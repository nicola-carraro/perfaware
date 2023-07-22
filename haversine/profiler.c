#ifndef PROFILE_C

#define PROFILE_C

#include "stdint.h"

#define MAX_COUNTERS 4096

#define COUNTER_NAME_CAPACITY 50

typedef struct
{
    uint64_t start;
    size_t id;
    uint64_t initialTicksInRoot;

} Counter;

#ifdef PROFILE
typedef struct
{

    uint64_t totalTicks;
    uint64_t ticksInRoot;
    uint64_t childrenTicks;
    size_t calls;
    const char *name;
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

void pushCounter(Counters *counters, size_t id, const char *name)
{
    assert(id != 0);
    size_t count = __rdtsc();

    TimedBlock *timedBlock = &(counters->timedBlocks[id]);
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
    char format[] = "%-25s (called %10zu times): \t\t with children: %20.10f (%14.10f %%) \t\t without children:  %20.10f (%14.10f %%) \n";

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
            totalPercentage += percentageWithoutChildren;
            printf(format, timedBlock->name, timedBlock->calls, secondsWithChildren, percentageWithChildren, secondsWithoutChildren, percentageWithoutChildren);
        }
    }
    printf("Total percentage: %14.10f\n", totalPercentage);
}

#define TIME_BLOCK(NAME)                                 \
    {                                                    \
                                                         \
        pushCounter(&COUNTERS, (__COUNTER__ + 1), NAME); \
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

#endif