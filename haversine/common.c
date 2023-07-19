#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "assert.h"
#include "math.h"
#include "stdint.h"
#include "assert.h"
#ifdef _WIN32
#include "windows.h"
#include "intrin.h"
#endif
#ifndef COMMON_C

#define COMMON_C

#define JSON_PATH "data/pairs.json"
#define ANSWERS_PATH "data/answers"

#define ARENA_SIZE 8 * 1024ll * 1024ll * 1024l

#define EARTH_RADIUS 6371

#define MAX_COUNTERS 4096

#define COUNTER_NAME_CAPACITY 50

#define TIME_FUNCTION                                        \
    {                                                        \
        pushCounter(&COUNTERS, (__COUNTER__ + 1), __func__); \
    }

#define STOP_COUNTER           \
    {                          \
        popCounter(&COUNTERS); \
    }

typedef struct
{
    union
    {
        char *signedData;
        uint8_t *unsignedData;
    } data;

    size_t size;
} String;

typedef struct
{
    uint64_t start;
    size_t id;
    uint64_t initialTicksInRoot;

} Counter;

typedef struct
{

    uint64_t totalTicks;
    uint64_t ticksInRoot;
    uint64_t childrenTicks;
    const char *name;
} TimedBlock;

typedef struct
{
    uint64_t start;
    uint64_t end;
    TimedBlock timedBlocks[MAX_COUNTERS];
    size_t blocksCount;
    uint64_t cpuCounterFrequency;
    Counter stack[MAX_COUNTERS];
    size_t stackSize;
} Counters;

typedef struct
{
    void *memory;
    size_t size;
    size_t currentOffset;
    size_t previousOffset;
} Arena;

void pushCounter(Counters *counters, size_t id, const char *name);

void popCounter(Counters *counters);

static Counters COUNTERS = {0};

void die(const char *file, const size_t line, int errorNumber, const char *message, ...)
{
    printf("ERROR (%s:%zu): ", file, line);

    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    if (errorNumber != 0)
    {
        char *cError = strerror(errorNumber);
        printf(" (%s)", cError);
    }

    exit(EXIT_FAILURE);
}

void writeTextToFile(FILE *file, const char *path, const char *format, ...)
{
    va_list varargsList;

    va_start(varargsList, format);

    if (vfprintf(file, format, varargsList) < 0)
    {
        int error = errno;
        fclose(file);
        die(__FILE__, __LINE__, error, "could not write to %s", path);
    }
    va_end(varargsList);
}

void writeBinaryToFile(FILE *file, const char *path, void *data, size_t size, size_t count)
{
    if (fwrite(data, size, count, file) != count)
    {
        int error = errno;
        fclose(file);
        die(__FILE__, __LINE__, error, "could not write to %s", path);
    }
}

size_t getFileSize(FILE *file, char *path)
{
    TIME_FUNCTION

    size_t result = 0;

    const char *errorMessage = "could not query the size of %s";

    if (fseek(file, 0, SEEK_END) != 0)
    {
        int errorNumber = errno;
        fclose(file);
        die(__FILE__, __LINE__, errorNumber, errorMessage, path);
    }
    else
    {
        long cursorPosition = ftell(file);

        if (cursorPosition >= 0)
        {
            result = (size_t)cursorPosition;

            if (fseek(file, 0, SEEK_SET) != 0)
            {
                int errorNumber = errno;
                fclose(file);
                die(__FILE__, __LINE__, errorNumber, "could not restore cursor position of %s", path);
            }
        }
        else
        {
            int errorNumber = errno;
            fclose(file);
            die(__FILE__, __LINE__, errorNumber, errorMessage, path);
        }
    }

    STOP_COUNTER

    return result;
}

Arena arenaInit()
{
    TIME_FUNCTION
    Arena arena = {0};

    arena.memory = malloc(ARENA_SIZE);

    if (!arena.memory)
    {
        die(__FILE__, __LINE__, errno, "could not initialize arena");
    }

    arena.size = ARENA_SIZE;
    arena.currentOffset = 0;
    arena.previousOffset = 0;

    STOP_COUNTER

    return arena;
}

void *arenaAllocate(Arena *arena, size_t size)
{
    assert(arena != NULL);

    void *result = NULL;

    if (arena->currentOffset + size >= arena->size)
    {
        die(__FILE__, __LINE__, errno, "could not allocate from arena");
    }

    result = (char *)arena->memory + arena->currentOffset;
    arena->previousOffset = arena->currentOffset;
    arena->currentOffset += size;

    return result;
}

void freeLastAllocation(Arena *arena)
{
    arena->currentOffset = arena->previousOffset;
}

String readFileToString(char *path, Arena *arena)
{
    TIME_FUNCTION
    FILE *file = fopen(path, "rb");

    if (file == NULL)
    {
        die(__FILE__, __LINE__, errno, "could not open %s", path);
    }
    String result = {0};
    result.size = getFileSize(file, path);
    result.data.signedData = arenaAllocate(arena, result.size);

    size_t read = fread(result.data.signedData, 1, result.size, file);

    if (read < result.size)
    {
        die(__FILE__, __LINE__, errno, "could not read %s, read %zu", path, read);
    }

    STOP_COUNTER

    return result;
}

double degreesToRadians(double degrees)
{
    return degrees * 0.01745329251994329577;
}

double square(double n)
{
    return n * n;
}

double haversine(double x1Degrees, double y1Degrees, double x2Degrees, double y2Degrees, double radius)
{

    double x1Radians = degreesToRadians(x1Degrees);
    double y1Radians = degreesToRadians(y1Degrees);
    double x2Radians = degreesToRadians(x2Degrees);
    double y2Radians = degreesToRadians(y2Degrees);

    double rootTerm = square(sin((y2Radians - y1Radians) / 2.0)) + cos(y1Radians) * cos(y2Radians) * square(sin((x2Radians - x1Radians) / 2.0));

    double result = 2.0 * radius * asin(sqrt(rootTerm));

    return result;
}

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

void printPerformanceReport(Counters *counters)
{

    size_t totalCount = counters->end - counters->start;

    assert(counters->stackSize == 0);

    float totalPercentage = 0.0f;
    char format[] = "%-25s: \t\t with children: %20.10f (%14.10f %%) \t\t without children:  %20.10f (%14.10f %%) \n";

    qsort((void *)(counters->timedBlocks), counters->blocksCount, sizeof(*counters->timedBlocks), compareTimedBlocks);
    for (size_t counterIndex = 1; counterIndex < counters->blocksCount; counterIndex++)
    {
        TimedBlock *timedBlock = &(counters->timedBlocks[counterIndex]);
        uint64_t totalTicks = timedBlock->totalTicks;
        uint64_t ticksInRoot = timedBlock->ticksInRoot;
        uint64_t childrenTicks = timedBlock->childrenTicks;
        uint64_t ticksWithoutChildren = totalTicks - childrenTicks;
        float secondsWithChildren = ((float)(ticksInRoot)) / ((float)(counters->cpuCounterFrequency));
        float secondsWithoutChildren = ((float)(ticksWithoutChildren)) / ((float)(counters->cpuCounterFrequency));
        float percentageWithoutChildren = (((float)ticksWithoutChildren) / ((float)(totalCount))) * 100.0f;
        float percentageWithChildren = (((float)ticksInRoot) / ((float)(totalCount))) * 100.0f;
        totalPercentage += percentageWithoutChildren;
        printf(format, timedBlock->name, secondsWithChildren, percentageWithChildren, secondsWithoutChildren, percentageWithoutChildren);
    }

    float totalSeconds = ((float)(totalCount)) / ((float)(counters->cpuCounterFrequency));
    printf("\n");

    char totalFormat[] = "%-25s: \t\t %10.10f                        (%14.10f %%)";
    printf(totalFormat, "Total", totalSeconds, totalPercentage);
}

#endif