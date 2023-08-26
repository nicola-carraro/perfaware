#ifndef PROFILE_C

#define PROFILE_C

#include "stdint.h"

#include "windows.h"

#include "assert.h"

#define COUNTER_NAME_CAPACITY 50

typedef struct
{
    uint64_t start;
    size_t id;
    uint64_t initialTicksInRoot;

} Counter;

#ifdef PROFILE
#define MAX_COUNTERS 4096
typedef struct
{

    uint64_t totalTicks;
    uint64_t ticksInRoot;
    uint64_t childrenTicks;
    size_t calls;
    const char *name;
    size_t bytes;
} TimedBlock;

#endif

typedef struct
{
    uint64_t start;
    uint64_t end;
    uint64_t cpuCounterFrequency;
#ifdef PROFILE
    TimedBlock timedBlocks[MAX_COUNTERS];
    size_t blocksCount;
    Counter stack[MAX_COUNTERS];
    size_t stackSize;
#endif
} Counters;

#ifdef PROFILE

void pushCounter(Counters *counters, size_t id, const char *name, size_t bytes)
{
    assert(id != 0);
    size_t count = __rdtsc();

    TimedBlock *timedBlock = &(counters->timedBlocks[id]);
    timedBlock->bytes = bytes;
    if (counters->blocksCount <= id)
    {
        counters->blocksCount = id + 1;
    }

    if (timedBlock->name == 0)
    {
        timedBlock->name = name;
    }

    timedBlock->calls++;

    counters->stack[counters->stackSize].id = id;
    counters->stack[counters->stackSize].start = count;
    counters->stack[counters->stackSize].initialTicksInRoot = timedBlock->ticksInRoot;
    counters->stackSize++;
}

void popCounter(Counters *counters)
{
    size_t endTicks = __rdtsc();

    assert(counters->stackSize > 0);

    size_t startTicks = counters->stack[counters->stackSize - 1].start;
    size_t id = counters->stack[counters->stackSize - 1].id;
    uint64_t initialTicksInRoot = counters->stack[counters->stackSize - 1].initialTicksInRoot;

    size_t elapsed = endTicks - startTicks;
    TimedBlock *timedBlock = &(counters->timedBlocks[id]);
    timedBlock->totalTicks += elapsed;
    timedBlock->ticksInRoot = initialTicksInRoot + elapsed;

    counters->stackSize--;

    if (counters->stackSize > 0)
    {
        size_t parentId = counters->stack[counters->stackSize - 1].id;
        TimedBlock *parentBlock = &(counters->timedBlocks[parentId]);
        parentBlock->childrenTicks += elapsed;
    }
}

int compareTimedBlocks(const void *left, const void *right)
{
    TimedBlock *leftTimedBlock = (TimedBlock *)(left);

    TimedBlock *rightTimedBlock = (TimedBlock *)(right);

    uint64_t leftTicksInRoot = leftTimedBlock->ticksInRoot;
    uint64_t rightTicksInRoot = rightTimedBlock->ticksInRoot;

    if (leftTicksInRoot > rightTicksInRoot)
    {
        return -1;
    }
    else if (leftTicksInRoot < rightTicksInRoot)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void printTimedBlocksStats(Counters *counters, size_t totalCount)
{
    assert(counters->stackSize == 0);

    float totalPercentage = 0.0f;

    char format[] = "%-25s (called %10zu times): \t\t with children: %20.10f (%14.10f %%) \t\t without children:  %20.10f (%14.10f %%) , Throughput: %4.10f 5 gB/S\n";

    qsort((void *)(counters->timedBlocks), counters->blocksCount, sizeof(*counters->timedBlocks), compareTimedBlocks);
    for (size_t counterIndex = 1; counterIndex < counters->blocksCount; counterIndex++)
    {
        TimedBlock *timedBlock = &(counters->timedBlocks[counterIndex]);
        if (timedBlock->name != NULL)
        {
            uint64_t totalTicks = timedBlock->totalTicks;
            uint64_t ticksInRoot = timedBlock->ticksInRoot;
            uint64_t childrenTicks = timedBlock->childrenTicks;
            uint64_t ticksWithoutChildren = totalTicks - childrenTicks;
            float secondsWithChildren = ((float)(ticksInRoot)) / ((float)(counters->cpuCounterFrequency));
            float secondsWithoutChildren = ((float)(ticksWithoutChildren)) / ((float)(counters->cpuCounterFrequency));
            float percentageWithoutChildren = (((float)ticksWithoutChildren) / ((float)(totalCount))) * 100.0f;
            float percentageWithChildren = (((float)ticksInRoot) / ((float)(totalCount))) * 100.0f;
            float througput = 0.0f;

            if (timedBlock->bytes > 0)
            {
                througput = ((float)timedBlock->bytes / secondsWithChildren) / (1024 * 1024 * 1024);
            }

            totalPercentage += percentageWithoutChildren;
            printf(format, timedBlock->name, timedBlock->calls, secondsWithChildren, percentageWithChildren, secondsWithoutChildren, percentageWithoutChildren, througput);
        }
    }
    printf("Total percentage: %14.10f\n", totalPercentage);
}

#define MEASURE_THROUGHPUT(NAME, BYTES)                         \
    {                                                           \
        pushCounter(&COUNTERS, (__COUNTER__ + 1), NAME, BYTES); \
    }

#define MEASURE_FUNCTION_THROUGHPUT(BYTES)                          \
    {                                                               \
        pushCounter(&COUNTERS, (__COUNTER__ + 1), __func__, BYTES); \
    }

#define TIME_BLOCK(NAME)             \
    {                                \
                                     \
        MEASURE_THROUGHPUT(NAME, 0); \
    }

#define TIME_FUNCTION         \
    {                         \
                              \
        TIME_BLOCK(__func__); \
    }

#define STOP_COUNTER           \
    {                          \
        popCounter(&COUNTERS); \
    }
#else

#define TIME_BLOCK \
    {              \
    }

#define MEASURE_THROUGHPUT(NAME, BYTES) \
    {                                   \
    }

#define MEASURE_FUNCTION_THROUGHPUT(BYTES) \
    {                                      \
    }

#define TIME_FUNCTION \
    {                 \
    }
#define STOP_COUNTER \
    {                \
    }
#endif

static Counters COUNTERS = {0};

void startCounters(Counters *counters)
{
    size_t count = __rdtsc();

    counters->start = count;
}

void stopCounters(Counters *counters)
{
    size_t count = __rdtsc();

    counters->end = count;
}

void printPerformanceReport(Counters *counters)
{

    size_t totalCount = counters->end - counters->start;

#ifdef PROFILE
    printTimedBlocksStats(counters, totalCount);
#endif

    float totalSeconds = ((float)(totalCount)) / ((float)(counters->cpuCounterFrequency));
    printf("\n");

    printf("Total time:       %14.10f\n", totalSeconds);
}

uint64_t getOsTimeFrequency()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    return frequency.QuadPart;
}

uint64_t getOsTimeStamp()
{
    LARGE_INTEGER timestamp;

    QueryPerformanceCounter(&timestamp);

    return timestamp.QuadPart;
}

uint64_t estimateRdtscFrequency()
{
    uint64_t frequency = getOsTimeFrequency();

    uint64_t osTicks;
    uint64_t cpuStart = __rdtsc();
    uint64_t osStart = getOsTimeStamp();

    do
    {
        osTicks = getOsTimeStamp();
    } while ((osTicks - osStart) < frequency);
    uint64_t cpuTicks = __rdtsc() - cpuStart;

    /* printf("Os frequency %llu\n", frequency);
     printf("Os ticks %llu\n", osTicks - osStart);
     printf("Cpu frequency %llu\n", cpuTicks);*/

    return cpuTicks;
}

#endif