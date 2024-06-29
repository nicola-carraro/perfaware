#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

#pragma warning(push, 0)

#include "windows.h"

#pragma warning(pop)

#include "intrin.h"
#include "assert.h"
#include "stdint.h"

#define GB (1024 * 1024 * 1024)

#define FILE_NAME "data/pairs.json"

typedef int32_t i32;

typedef int64_t i64;

typedef uint32_t u32;

typedef uint64_t u64;

u64 measureRdtscFrequency() {
    LARGE_INTEGER performanceFrequency = {0};
    BOOL ok = QueryPerformanceFrequency(&performanceFrequency);
    assert(ok);

    LARGE_INTEGER performanceCounterStart = {0};
    ok = QueryPerformanceCounter(&performanceCounterStart);
    assert(ok);

    LARGE_INTEGER performanceCount = {0};

    u64 rdtscStart = __rdtsc();
    do {
        ok = QueryPerformanceCounter(&performanceCount);
    } while (performanceCount.QuadPart - performanceCounterStart.QuadPart < performanceFrequency.QuadPart);
    u64 rdtscEnd = __rdtsc();

    assert(ok);

    u64 rdtscFrequency = rdtscEnd - rdtscStart;

    return rdtscFrequency;
}

u64 readFromFile(u32 bufferSize) {
    DWORD bytesRead = 0;
    u64 totalBytesRead = 0;

    char *buffer = VirtualAlloc(0, bufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    assert(buffer);

    HANDLE file = CreateFile(FILE_NAME, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    assert(file);

    BOOL ok = 0;
    do {
        ok = ReadFile(file, buffer, bufferSize, &bytesRead, 0);
        assert(ok);
        totalBytesRead += bytesRead;
    } while (bytesRead == bufferSize);

    ok = CloseHandle(file);
    assert(ok);

    ok = VirtualFree(buffer, 0, MEM_RELEASE);
    assert(ok);

    return totalBytesRead;
}

float measureThroughput(u32 bufferSize, u64 rdtscFrequency) {
    u64 best = UINT64_MAX;

    u64 secondsSinceReset = 0;

    u64 reset = __rdtsc();

    float bestGbPerSecond = 0;

    while (secondsSinceReset < 10) {
        u64 start = __rdtsc();
        u64 bytes = readFromFile(bufferSize);
        u64 end = __rdtsc();

        u64 elapsed = end - start;

        u64 ticksSinceReset = end - reset;
        secondsSinceReset = ticksSinceReset / rdtscFrequency;

        if (elapsed < best) {
            best = elapsed;
            secondsSinceReset = 0;

            float seconds = (float) (elapsed) / (float) rdtscFrequency;
            float gb = (float) bytes / (float) GB;

            float gbPerSecond = gb / seconds;

            bestGbPerSecond = gbPerSecond;

            printf("Buffer size %I32u: %f sec, %f gb/sec\n", bufferSize, seconds, gbPerSecond);
        }
    }

    return bestGbPerSecond;
}

int main(void) {
    u64 rdtscFrequency = measureRdtscFrequency();
    u32 bufferSizes[] = {
        1024 * 256,
        1024 * 512,
        1024 * 1024,
        1024 * 1024 * 2,
        1024 * 1024 * 4,
        1024 * 1024 * 8,
        1024 * 1024 * 16,
        1024 * 1024 * 32,
        1024 * 1024 * 512,
        1024 * 1024 * 1024,
    };

    float throughputs[ARRAYSIZE(bufferSizes)] = {0};

    for (i32 testIndex = 0; testIndex < ARRAYSIZE(bufferSizes); testIndex++) {
        throughputs[testIndex] = measureThroughput(bufferSizes[testIndex], rdtscFrequency);
    }

    for (i32 testIndex = 0; testIndex < ARRAYSIZE(bufferSizes); testIndex++) {
        printf("size: %u, throughput: %f\n", bufferSizes[testIndex], throughputs[testIndex]);
    }
}
