#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

#pragma warning(push, 0)

#include "windows.h"

#pragma warning(pop)

#include "intrin.h"
#include "assert.h"
#include "stdint.h"

typedef int64_t i64;

typedef uint64_t u64;

u64 measureRdtscFrequency() {
    LARGE_INTEGER performanceFrequency = {0};
    BOOL ok = QueryPerformanceFrequency(&performanceFrequency);
    assert(ok);
    printf("%llu\n", performanceFrequency.QuadPart);

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

    printf("%llu\n", rdtscFrequency);
}
