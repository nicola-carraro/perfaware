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

#define ARENA_SIZE 1024ll * 1024ll * 1024l

#define EARTH_RADIUS 6371

#define MAX_COUNTERS 100

#define COUNTER_NAME_CAPACITY 50

#define TIME_FUNCTION                                         \
    {                                                         \
        startCounter(&COUNTERS, (__COUNTER__ + 1), __func__); \
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
    uint64_t lastStart[MAX_COUNTERS];
    uint64_t totalTicks[MAX_COUNTERS];
    char names[COUNTER_NAME_CAPACITY][MAX_COUNTERS];
    size_t countersCount;
    uint64_t cpuCounterFrequency;
    size_t activeCounter;
} Counters;

typedef struct
{
    void *memory;
    size_t size;
    size_t currentOffset;
    size_t previousOffset;
} Arena;

void startCounter(Counters *counters, size_t id, char *name);

void stopCounter(Counters *counters);

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

    stopCounter(&COUNTERS);

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

    stopCounter(&COUNTERS);

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

void startCounter(Counters *counters, size_t id, char *name)
{
    assert(id != 0);
    size_t count = __rdtsc();

    assert(counters->activeCounter == 0);

    if (counters->countersCount <= id)
    {
        counters->countersCount = id + 1;
    }

    if (counters->names[id][0] == '\0')
    {
        strncpy(counters->names[id], name, COUNTER_NAME_CAPACITY - 1);
    }

    counters->lastStart[id] = count;
    counters->activeCounter = id;
}

void stopCounter(Counters *counters)
{
    size_t endTicks = __rdtsc();

    assert(counters->lastStart[counters->activeCounter] != 0);

    size_t elapsed = endTicks - counters->lastStart[counters->activeCounter];
    counters->totalTicks[counters->activeCounter] += elapsed;

    counters->lastStart[counters->activeCounter] = 0;
    counters->activeCounter = 0;
}

void printPerformanceReport(Counters *counters)
{

    size_t totalCount = 0;

    for (size_t counterIndex = 1; counterIndex < counters->countersCount; counterIndex++)
    {
        totalCount += counters->totalTicks[counterIndex];
    }

    float totalPercentage = 0.0f;
    char format[] = "%-25s: %20.10f (%14.10f %%)\n";

    for (size_t counterIndex = 1; counterIndex < counters->countersCount; counterIndex++)
    {
        assert(counters->lastStart[counterIndex] == 0);
        float seconds = ((float)(counters->totalTicks[counterIndex])) / ((float)(counters->cpuCounterFrequency));
        float percentage = (((float)counters->totalTicks[counterIndex]) / ((float)(totalCount))) * 100.0f;
        totalPercentage += percentage;
        printf(format, counters->names[counterIndex], seconds, percentage);
    }

    float totalSeconds = ((float)(totalCount)) / ((float)(counters->cpuCounterFrequency));
    printf("\n");
    printf(format, "Total", totalSeconds, totalPercentage);
}

#endif