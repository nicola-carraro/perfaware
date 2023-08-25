#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "profiler.c"
#include "common.c"
#include "limits.h"

int main(void)
{
  uint64_t frequency = estimateCpuCounterFrequency();

  uint64_t ticks = _rdtsc();

  float timeSinceLastReset = 0.0f;

  float min = FLT_MAX;

  float max = 0.0f;

  float average = 0.0f;

  while (timeSinceLastReset < 10.0f)
  {
    FILE *file = fopen(JSON_PATH, "r");

    if (file != NULL)
    {
      size_t size = getFileSize(file, JSON_PATH);

      uint64_t start = __rdtsc();
    }
  }

  return 0;
}