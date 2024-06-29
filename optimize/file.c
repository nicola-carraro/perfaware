#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

#pragma warning(push, 0)

#include "windows.h"

#pragma warning(pop)

#include "intrin.h"
#include "assert.h"
#include "stdint.h"

#define BUFFER_SIZE 1024 * 1024 * 1024

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

int main(void) {
    u64 rdtscFrequency = measureRdtscFrequency();

    rdtscFrequency;

    char *buffer = VirtualAlloc(0, BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    assert(buffer);

    HANDLE file = CreateFile(FILE_NAME, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    assert(file);

    u32 readSize = BUFFER_SIZE;
    DWORD bytesRead = 0;
    u64 totalBytesRead = 0;

    do {
        BOOL ok = ReadFile(file, buffer, readSize, &bytesRead, 0);
        assert(ok);
        totalBytesRead += bytesRead;
    } while (bytesRead == readSize);
}
