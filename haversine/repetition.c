#include "common.c"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "profiler.c"
#include "float.h"
#include "intsafe.h"
#pragma warning(push, 0)
#include "windows.h"
#pragma warning(pop)
#include "psapi.h"
#pragma
#include "io.h"
#include "sys\stat.h"
#include "fcntl.h"

#define MAKE_TEST(f, n, u)                                                                       \
  {                                                                                              \
    .minSeconds = FLT_MAX, .minPageFaults = UINT64_MAX, .function = f, .name = n, .useMalloc = u \
  }

typedef struct
{
  uint64_t ticks;
  uint64_t bytes;
  uint64_t pageFaults;
  uint64_t startTicks;
  uint64_t startPageFaults;
} Iteration;

typedef struct
{

  float minSeconds;

  float maxSeconds;

  float sumSeconds;

  uint64_t minPageFaults;

  uint64_t maxPageFaults;

  uint64_t sumPageFaults;

  size_t executionCount;

  void (*function)(Iteration *iteration, Arena *arena, bool useMalloc, HANDLE process);

  char *name;

  bool useMalloc;
} Test;

char *winErrorMessage()
{

  char *result = NULL;

  DWORD error = GetLastError();
  FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
          FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      error,
      MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
      (LPTSTR)&result,
      0,
      NULL);

  return result;
}

uint64_t getPageFaultCount(HANDLE process)
{
  uint64_t result = 0;
  PROCESS_MEMORY_COUNTERS memoryCounters = {0};

  if (GetProcessMemoryInfo(process, &memoryCounters, sizeof(memoryCounters)))
  {
    result = memoryCounters.PageFaultCount;
  }
  else
  {
    char *message = winErrorMessage();

    die(__FILE__, __LINE__, 0, "Failed to get page fault count: %s\n", message);
  }

  return result;
}

void iterationStartCounters(Iteration *iteration, HANDLE process, uint64_t bytes)
{
  iteration->startPageFaults = getPageFaultCount(process);
  iteration->startTicks = __rdtsc();
  iteration->bytes = bytes;
}

void iterationStopCounters(Iteration *iteration, HANDLE process)
{
  uint64_t ticks = __rdtsc();
  uint64_t pageFaults = getPageFaultCount(process);
  iteration->pageFaults = pageFaults - iteration->startPageFaults;
  iteration->ticks = ticks - iteration->startTicks;
}

void repeatTest(Test *test, uint64_t rdtscFrequency, Arena *arena, HANDLE process)
{

  uint64_t ticksSinceLastReset = __rdtsc();

  float secondsSinceLastReset = 0.0f;

  printf("%s:\n", test->name);

  while (true)
  {

    Iteration iteration = {0};

    test->function(&iteration, arena, test->useMalloc, process);

    float secondsForFunction = (float)iteration.ticks / (float)rdtscFrequency;
    uint64_t pageFaults = iteration.pageFaults;

    if (secondsForFunction < test->minSeconds)
    {
      test->minSeconds = secondsForFunction;

      secondsSinceLastReset = 0.0f;

      ticksSinceLastReset = __rdtsc();
    }

    if (secondsForFunction > test->maxSeconds)
    {
      test->maxSeconds = secondsForFunction;
    }

    if (pageFaults < test->minPageFaults)
    {
      test->minPageFaults = pageFaults;
    }

    if (pageFaults > test->maxPageFaults)
    {
      test->maxPageFaults = pageFaults;
    }

    test->executionCount++;
    test->sumSeconds += secondsForFunction;
    test->sumPageFaults += pageFaults;

    float averageSeconds = test->sumSeconds / (float)test->executionCount;
    float averagePageFaults = (float)test->sumPageFaults / (float)test->executionCount;

    float gb = ((float)iteration.bytes) / (1024.0f * 1024.0f * 1024.0f);
    float kb = ((float)iteration.bytes) / 1024.0f;

    float minGbPerSecond = gb / test->maxSeconds;
    float maxGbPerSecond = gb / test->minSeconds;
    float averageGbPerSecond = gb / averageSeconds;

    float minKbPerFault = kb / (float)test->maxPageFaults;
    float maxKbPerFault = kb / (float)test->minPageFaults;
    float averageKbPerFault = kb / averagePageFaults;

    printf("Best : %f s, Throughput: %f Gb/s, PF: %f (%f kb/fault)\n", test->minSeconds, maxGbPerSecond, (float)test->minPageFaults, maxKbPerFault);
    printf("Worst: %f s, Throughput: %f Gb/s, PF: %f (%f kb/fault)\n", test->maxSeconds, minGbPerSecond, (float)test->maxPageFaults, minKbPerFault);
    printf("Avg. : %f s, Throughput: %f Gb/s, PF: %f (%f kb/fault)\n", averageSeconds, averageGbPerSecond, averagePageFaults, averageKbPerFault);

    uint64_t ticks = __rdtsc();
    secondsSinceLastReset = ((float)(ticks - ticksSinceLastReset)) / ((float)rdtscFrequency);

    if (secondsSinceLastReset > 10.0f)
    {
      break;
    }

    for (size_t i = 0; i < 3; i++)
    {
      printf("\033[F");
    }
  }

  printf("\n");
}

void readWithFread(Iteration *iteration, Arena *arena, bool useMalloc, HANDLE process)
{
  useMalloc;
  FILE *file = fopen(JSON_PATH, "rb");

  if (file != NULL)
  {
    size_t size = getFileSize(file, JSON_PATH);

    char *buffer = 0;

    if (useMalloc)
    {
      buffer = malloc(size);
    }
    else
    {
      buffer = arenaAllocate(arena, size);
    }

    iterationStartCounters(iteration, process, size);

    size_t read = fread(buffer, 1, size, file);

    iterationStopCounters(iteration, process);

    fclose(file);

    if (useMalloc)
    {
      free(buffer);
    }
    else
    {
      arenaFreeAll(arena);
    }

    if (read < 1)
    {
      die(__FILE__, __LINE__, errno, "Read failed");
    }
  }
}

void readWith_read(Iteration *iteration, Arena *arena, bool useMalloc, HANDLE process)
{
  useMalloc;
  int fd = _open(JSON_PATH, _O_RDONLY | _O_BINARY);

  if (fd != 0)
  {
    struct __stat64 fileStat = {0};
    if (_stat64(JSON_PATH, &fileStat) == 0)
    {
      size_t size = fileStat.st_size;

      char *buffer = 0;
      if (useMalloc)
      {
        buffer = malloc(size);
      }
      else
      {
        buffer = arenaAllocate(arena, size);
      }
      char *cursor = buffer;

      size_t remainingBytes = size;

      iterationStartCounters(iteration, process, size);

      while (remainingBytes > 0)
      {

        int bytesToRead = 0;

        if (size > INT_MAX)
        {
          bytesToRead = INT_MAX;
        }
        else
        {
          bytesToRead = (int)size;
        }

        int read = _read(fd, cursor, bytesToRead);

        remainingBytes -= read;

        cursor += read;

        if (read < size)
        {
          die(__FILE__, __LINE__, errno, "Read failed");
        }
      }
      iterationStopCounters(iteration, process);

      _close(fd);

      if (useMalloc)
      {
        free(buffer);
      }
      else
      {
        arenaFreeAll(arena);
      }
    }
  }
}

void readWithReadFile(Iteration *iteration, Arena *arena, bool useMalloc, HANDLE process)
{
  useMalloc;

  HANDLE file = CreateFile(
      JSON_PATH,
      GENERIC_READ,
      0,
      0,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL);

  if (file != INVALID_HANDLE_VALUE)
  {

    LARGE_INTEGER fileSize = {0};
    if (GetFileSizeEx(file, &fileSize))
    {

      char *buffer = 0;

      if (useMalloc)
      {
        buffer = malloc(fileSize.QuadPart);
      }
      else
      {
        buffer = arenaAllocate(arena, fileSize.QuadPart);
      }

      char *cursor = buffer;

      uint64_t remainingBytes = (uint64_t)fileSize.QuadPart;

      iterationStartCounters(iteration, process, fileSize.QuadPart);

      while (remainingBytes > 0)
      {
        DWORD bytesToRead = 0;
        if (remainingBytes > DWORD_MAX)
        {
          bytesToRead = DWORD_MAX;
        }
        else
        {
          bytesToRead = (DWORD)remainingBytes;
        }

        DWORD read = 0;

        BOOL success = ReadFile(
            file,
            cursor,
            bytesToRead,
            &read,
            NULL);

        cursor += read;
        remainingBytes -= read;
        if (!success || bytesToRead != read)
        {
          break;
        }
      }
      iterationStopCounters(iteration, process);

      if (remainingBytes > 0)
      {
        die(__FILE__, __LINE__, 0, "Error while reading file");
      }

      if (useMalloc)
      {
        free(buffer);
      }
      else
      {
        arenaFreeAll(arena);
      }
    }

    CloseHandle(file);
  }
}

void writeBuffer(Iteration *iteration, Arena *arena, bool useMalloc, HANDLE process)
{
  char *buffer = 0;

  uint64_t size = 947115053;

  if (useMalloc)
  {
    buffer = malloc(size);
  }
  else
  {
    buffer = arenaAllocate(arena, size);
  }

  iterationStartCounters(iteration, process, size);

  for (uint64_t i = 0; i < size; i++)
  {
    buffer[i] = (char)i;
  }

  if (useMalloc)
  {
    free(buffer);
  }
  else
  {
    arenaFreeAll(arena);
  }

  iterationStopCounters(iteration, process);
}

int main(void)
{
  uint64_t rdtscFrequency = estimateRdtscFrequency();

  Arena arena = arenaInit();

  HANDLE process = GetCurrentProcess();

  Test tests[] = {
      MAKE_TEST(writeBuffer, "malloc + writeBuffer", true),
      MAKE_TEST(readWith_read, "_read", false),
      MAKE_TEST(readWith_read, "malloc + _read", true),
      MAKE_TEST(readWithFread, "fread", false),
      MAKE_TEST(readWithFread, "malloc + fread", true),
      MAKE_TEST(readWithReadFile, "ReadFile", false),
      MAKE_TEST(readWithReadFile, "malloc + ReadFile", true),
  };

  while (true)
  {

    for (size_t i = 0; i < ARRAYSIZE(tests); i++)
    {
      repeatTest(tests + i, rdtscFrequency, &arena, process);
    }
  }

  return 0;
}