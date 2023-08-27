#include "common.c"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "profiler.c"
#include "float.h"
#pragma warning(push, 0)
#include "windows.h"
#pragma warning(pop)
#pragma
#include "io.h"
#include "sys\stat.h"
#include "fcntl.h"

typedef struct
{
  uint64_t ticksForFunction;
  uint64_t bytes;
  bool success;
} Iteration;

typedef struct
{

  float minSeconds;

  float maxSeconds;

  float sumSeconds;

  float averageSeconds;

  size_t executionCount;

  Iteration (*function)(Arena *);

  char *name;
} Test;

void repeatTest(Test *test, uint64_t rdtscFrequency, Arena *arena)
{

  uint64_t ticksSinceLastReset = __rdtsc();

  float secondsSinceLastReset = 0.0f;

  printf("%s:\n", test->name);

  while (true)
  {

    Iteration iteration = test->function(arena);
    arenaFreeAll(arena);

    if (iteration.success)
    {
      float secondsForFunction = (float)iteration.ticksForFunction / (float)rdtscFrequency;

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

      test->executionCount++;
      test->sumSeconds += secondsForFunction;

      test->averageSeconds = test->sumSeconds / (float)test->executionCount;

      float gb = ((float)iteration.bytes) / (1024.0f * 1024.0f * 1024.0f);

      float maxGbPerSecond = gb / test->maxSeconds;
      float minGbPerSecond = gb / test->minSeconds;
      float averageGbPerSecond = gb / test->averageSeconds;

      printf("Min: %f s, Throughput: %f Gb/s\n", test->minSeconds, minGbPerSecond);
      printf("Max: %f s, Throughput: %f Gb/s\n", test->maxSeconds, maxGbPerSecond);
      printf("Avg: %f s, Throughput: %f Gb/s\n", test->averageSeconds, averageGbPerSecond);

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
    else
    {
      break;
    }
  }

  printf("\n");
}

Iteration readWithFread(Arena *arena)
{
  Iteration iteration = {0};
  FILE *file = fopen(JSON_PATH, "rb");

  if (file != NULL)
  {
    size_t size = getFileSize(file, JSON_PATH);

    iteration.bytes = size;

    char *buffer = arenaAllocate(arena, size);

    uint64_t start = __rdtsc();

    size_t read = fread(buffer, 1, size, file);

    uint64_t stop = __rdtsc();
    fclose(file);

    if (read < 1)
    {
      perror("Read failed");
    }
    else
    {
      iteration.success = true;
      iteration.ticksForFunction = stop - start;
    }
  }

  return iteration;
}

Iteration readWith_read(Arena *arena)
{
  Iteration iteration = {0};
  int fd = _open(JSON_PATH, _O_RDONLY | _O_BINARY);

  if (fd != 0)
  {
    struct __stat64 fileStat = {0};
    if (_stat64(JSON_PATH, &fileStat) == 0)
    {
      size_t size = fileStat.st_size;
      iteration.bytes = size;

      char *buffer = arenaAllocate(arena, size);

      char *cursor = buffer;

      size_t remainingBytes = size;

      uint64_t start = __rdtsc();

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
          perror("Read failed");
          break;
        }
      }
      uint64_t stop = __rdtsc();

      _close(fd);

      if (remainingBytes == 0)
      {
        iteration.success = true;
        iteration.ticksForFunction = stop - start;
      }
    }
  }

  return iteration;
}

Iteration readWithReadFile(Arena *arena)
{
  Iteration result = {0};
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
    DWORD fileSizeHigh = 0;
    DWORD fileSizeLow = GetFileSize(file, &fileSizeHigh);

    LARGE_INTEGER fileSize = {.LowPart = fileSizeLow, .HighPart = fileSizeHigh};

    if (fileSizeLow != INVALID_FILE_SIZE)
    {

      char *buffer = arenaAllocate(arena, fileSize.QuadPart);

      char *cursor = buffer;

      uint64_t remainingBytes = (uint64_t)fileSize.QuadPart;

      uint64_t start = __rdtsc();
      while (remainingBytes > 0)
      {
        DWORD bytesToRead = 0;
        if (remainingBytes > ULONG_MAX)
        {
          bytesToRead = ULONG_MAX;
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
      uint64_t end = __rdtsc();

      if (remainingBytes > 0)
      {
        fprintf(stderr, "Error while reading file");
      }
      else
      {
        result.success = true;
        result.bytes = fileSize.QuadPart;
        result.ticksForFunction = end - start;
      }
    }
    CloseHandle(file);
  }

  return result;
}

int main(void)
{
  uint64_t rdtscFrequency = estimateRdtscFrequency();

  Arena arena = arenaInit();

  Test tests[] = {
      {
          .minSeconds = FLT_MAX,
          .function = readWithFread,
          .name = "fread",
      },
      {.minSeconds = FLT_MAX,
       .function = readWith_read,
       .name = "_read"},
      {.minSeconds = FLT_MAX,
       .function = readWithReadFile,
       .name = "ReadFile"},
  };

  while (true)
  {

    for (size_t i = 0; i < ARRAYSIZE(tests); i++)
    {
      repeatTest(tests + i, rdtscFrequency, &arena);
    }
  }

  return 0;
}