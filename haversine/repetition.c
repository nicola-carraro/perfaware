#include "common.c"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "profiler.c"
#include "float.h"

int main(void)
{
  uint64_t rdtscFrequency = estimateCpuCounterFrequency();

  uint64_t ticksSinceLastReset = __rdtsc();

  float secondsSinceLastReset = 0.0f;

  float minSeconds = FLT_MAX;

  float maxSeconds = 0.0f;

  float sumSeconds = 0.0f;

  float averageSeconds = 0.0f;

  size_t executionCount = 0;

  Arena arena = arenaInit();

  printf("fread:\n");

  while (true)
  {
    FILE *file = fopen(JSON_PATH, "r");

    if (file != NULL)
    {
      size_t size = getFileSize(file, JSON_PATH);

      float gb = (float)size / (1024.0f * 1024.0f * 1024.0f);

      char *buffer = arenaAllocate(&arena, size);

      uint64_t start = __rdtsc();

      size_t read = fread(buffer, 1, size, file);

      uint64_t stop = __rdtsc();

      if (read < 1)
      {
        perror("Read failed");
        break;
      }

      arenaFreeAll(&arena);

      uint64_t ticksForFunction = stop - start;

      float secondsForFunction = (float)ticksForFunction / (float)rdtscFrequency;

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

    fclose(file);
  }

  return 0;
}