#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "assert.h"
#include "math.h"
#include "stdint.h"
#include "assert.h"
#include "profiler.c"
#include "windows.h"

#ifdef _WIN32

#pragma warning(push, 0)

#include "windows.h"
#include "psapi.h"

#pragma warning(pop)

#include "intrin.h"

#endif

#ifndef COMMON_C

#define COMMON_C

#define JSON_PATH "data/pairs.json"

#define ANSWERS_PATH "data/answers"

#define ARENA_SIZE 8 * 1024ll * 1024ll * 1024l

#define EARTH_RADIUS 6371

typedef struct {
    union {
        char *signedData;
        uint8_t *unsignedData;
    } data;
    size_t size;
} String;

typedef struct {
    void *memory;
    size_t size;
    size_t currentOffset;
    size_t previousOffset;
} Arena;

void die(const char *file, const size_t line, int errorNumber, const char *message, ...) {
    printf("ERROR (%s:%zu): ", file, line);

    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    if (errorNumber != 0) {
        char *cError = strerror(errorNumber);
        printf(" (%s)", cError);
    }

    exit(EXIT_FAILURE);
}

void writeTextToFile(FILE *file, const char *path, const char *format, ...) {
    va_list varargsList;

    va_start(varargsList, format);

    if (vfprintf(file, format, varargsList) < 0) {
        int error = errno;
        fclose(file);
        die(__FILE__, __LINE__, error, "could not write to %s", path);
    }
    va_end(varargsList);
}

void writeBinaryToFile(FILE *file, const char *path, void *data, size_t size, size_t count) {
    if (fwrite(data, size, count, file) != count) {
        int error = errno;
        fclose(file);
        die(__FILE__, __LINE__, error, "could not write to %s", path);
    }
}

size_t getFileSize(HANDLE *file, char *path) {
    TIME_FUNCTION;

    DWORD high = 0;
    DWORD low = GetFileSize(file, &high);

    if (low == INVALID_FILE_SIZE) {
        die(__FILE__, __LINE__, 0, "could not query the size of %s", path);
    }

    LARGE_INTEGER result = {.LowPart = low, .HighPart = high};

    STOP_COUNTER;
    return result.QuadPart;
}

Arena arenaInit() {
    TIME_FUNCTION Arena arena = {0};

    arena.memory = VirtualAlloc(0, ARENA_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!arena.memory) {
        die(__FILE__, __LINE__, errno, "could not initialize arena");
    }

    arena.size = ARENA_SIZE;
    arena.currentOffset = 0;
    arena.previousOffset = 0;

    STOP_COUNTER return arena;
}

void arenaFreeAll(Arena *arena) {
    arena->currentOffset = 0;
    arena->previousOffset = 0;
}

void *arenaAllocate(Arena *arena, size_t size) {
    assert(arena != NULL);

    assert(arena->currentOffset + size < arena->size);

    void *result = (char *) arena->memory + arena->currentOffset;
    arena->previousOffset = arena->currentOffset;
    arena->currentOffset += size;

    return result;
}

void freeLastAllocation(Arena *arena) {
    arena->currentOffset = arena->previousOffset;
}

String readFileToString(char *path, Arena *arena) {
    HANDLE file = CreateFile(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (file == INVALID_HANDLE_VALUE) {
        die(__FILE__, __LINE__, errno, "could not open %s", path);
    }
    String result = {0};
    result.size = getFileSize(file, path);
    result.data.signedData = arenaAllocate(arena, result.size);

    MEASURE_THROUGHPUT("fread", result.size);

    size_t totalBytesRead = 0;

    char *buffer = result.data.signedData;

    BOOL ok = TRUE;

    while (totalBytesRead < result.size) {
        DWORD bytesToRead = result.size <= MAXDWORD ? (DWORD) result.size : MAXDWORD;

        DWORD bytesRead = 0;

        ok = ReadFile(file, buffer, bytesToRead, &bytesRead, 0);
        totalBytesRead += bytesRead;
        buffer += bytesRead;
    }

    if (!ok) {
        die(__FILE__, __LINE__, errno, "could not read %s", path);
    }

    ok = CloseHandle(file);

    if (!ok) {
        die(__FILE__, __LINE__, errno, "could not close %s", path);
    }

    STOP_COUNTER;

    return result;
}

char *winErrorMessage() {
    char *result = NULL;

    DWORD error = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
        (LPTSTR) &result,
        0,
        NULL
    );

    return result;
}

uint64_t getPageFaultCount(HANDLE process) {
    uint64_t result = 0;
    PROCESS_MEMORY_COUNTERS memoryCounters = {0};

    if (GetProcessMemoryInfo(process, &memoryCounters, sizeof(memoryCounters))) {
        result = memoryCounters.PageFaultCount;
    }
    else {
        char *message = winErrorMessage();

        die(__FILE__, __LINE__, 0, "Failed to get page fault count: %s\n", message);
    }

    return result;
}

double degreesToRadians(double degrees) {
    return degrees * 0.01745329251994329577;
}

double square(double n) {
    return n *n;
}

double haversine(double x1Degrees, double y1Degrees, double x2Degrees, double y2Degrees, double radius) {
    double x1Radians = degreesToRadians(x1Degrees);
    double y1Radians = degreesToRadians(y1Degrees);
    double x2Radians = degreesToRadians(x2Degrees);
    double y2Radians = degreesToRadians(y2Degrees);

    double rootTerm = square(sin((y2Radians - y1Radians) / 2.0))
        + cos(y1Radians) * cos(y2Radians) * square(sin((x2Radians - x1Radians) / 2.0));

    double result = 2.0 * radius * asin(sqrt(rootTerm));

    return result;
}

#endif
