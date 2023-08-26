#include "common.c"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "profiler.c"
#include "float.h"
#include "windows.h"
#include "io.h"
#include "sys\stat.h"
#include "fcntl.h"

typedef struct
{
  uint64_t ticksForFunction;
  float gb;
  bool success;
} Iteration;

void repeatTest(Iteration (*test)(Arena *), char *testName, uint64_t rdtscFrequency, Arena *arena)
{

  uint64_t ticksSinceLastReset = __rdtsc();

  float secondsSinceLastReset = 0.0f;

  float minSeconds = FLT_MAX;

  float maxSeconds = 0.0f;

  float sumSeconds = 0.0f;

  float averageSeconds = 0.0f;

  size_t executionCount = 0;

  printf("%s:\n", testName);

  while (true)
  {

    Iteration iteration = test(arena);
    arenaFreeAll(arena);

    if (iteration.success)
    {
      float secondsForFunction = (float)iteration.ticksForFunction / (float)rdtscFrequency;

      if (secondsForFunction < minSeconds)
      {
        minSeconds = secondsForFunction;

        secondsSinceLastReset = 0.0f;

        ticksSinceLastReset = __rdtsc();
      }

      if (secondsForFunction > maxSeconds)
      {
        maxSeconds = secondsForFunction;
      }

      executionCount++;
      sumSeconds += secondsForFunction;

      averageSeconds = sumSeconds / (float)executionCount;

      float gb = iteration.gb;

      float maxGbPerSecond = gb / maxSeconds;
      float minGbPerSecond = gb / minSeconds;
      float averageGbPerSecond = gb / averageSeconds;

      printf("Min: %f s, Throughput: %f Gb/s\n", minSeconds, minGbPerSecond);
      printf("Max: %f s, Throughput: %f Gb/s\n", maxSeconds, maxGbPerSecond);
      printf("Avg: %f s, Throughput: %f Gb/s\n", averageSeconds, averageGbPerSecond);

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

    iteration.gb = (float)size / (1024.0f * 1024.0f * 1024.0f);

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
  int fd = _open(JSON_PATH, _O_RDONLY | _O_BINARY, _SH_DENYRW);

  if (fd != 0)
  {
    struct __stat64 fileStat = {0};
    if (_stat64(JSON_PATH, &fileStat) == 0)
    {
      size_t size = fileStat.st_size;
      iteration.gb = (float)size / (1024.0f * 1024.0f * 1024.0f);

      char *buffer = arenaAllocate(arena, size);

      uint64_t start = __rdtsc();

      size_t remainingBytes = size;

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

        int read = _read(fd, buffer, bytesToRead);

        remainingBytes -= read;

        uint64_t stop = __rdtsc();
        _close(fd);

        if (read < size)
        {
          perror("Read failed");
        }
        else
        {
          iteration.success = true;
          iteration.ticksForFunction = stop - start;
        }
      }
    }
  }

  return iteration;
}

int main(void)
{
  uint64_t rdtscFrequency = estimateRdtscFrequency();

  Arena arena = arenaInit();

  repeatTest(readWithFread, "fread", rdtscFrequency, &arena);

  repeatTest(readWith_read, "_read", rdtscFrequency, &arena);

  return 0;
}