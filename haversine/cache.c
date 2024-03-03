#include "common.c"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "profiler.c"
#include "float.h"

#define MAKE_TEST(f, n, r, b, s) {\
    .minSeconds = FLT_MAX,\
    .function = f,\
    .name = n,\
    .bytes = r,\
    .buffer = b,\
    .cacheSizeOrMask = s\
}

#define KB(b) (1024LL *b)

#define MB(b) (KB(KB(b)))

#define GB(b) (KB(MB(b)))

typedef struct {
    float minSeconds;
    float maxSeconds;
    float sumSeconds;
    size_t executionCount;
    void (*function) (int64_t bytes, void *buffer, int64_t cacheSize);
    char *name;
    float maxThroughput;
    int64_t bytes;
    int64_t cacheSizeOrMask;
    void *buffer;
} Test;

void testCache(int64_t bytes, void *buffer, int64_t cacheSize);

void testCacheAnd(int64_t bytes, void *buffer, int64_t cacheSize);

void repeatTest(Test *test, uint64_t rdtscFrequency) {
    uint64_t ticksSinceLastReset = __rdtsc();

    float secondsSinceLastReset = 0.0f;

    printf("%s:\n", test->name);

    while (true) {
        uint64_t start = __rdtsc();
        test->function(test->bytes, test->buffer, test->cacheSizeOrMask);
        uint64_t end = __rdtsc();

        uint64_t ticks = end - start;

        float secondsForFunction = (float) ticks / (float) rdtscFrequency;

        if (secondsForFunction < test->minSeconds) {
            test->minSeconds = secondsForFunction;

            secondsSinceLastReset = 0.0f;

            ticksSinceLastReset = __rdtsc();
        }

        if (secondsForFunction > test->maxSeconds) {
            test->maxSeconds = secondsForFunction;
        }

        test->executionCount++;
        test->sumSeconds += secondsForFunction;

        float averageSeconds = test->sumSeconds / (float) test->executionCount;
        float minThroughput = (float) test->bytes / test->maxSeconds;
        float maxThroughput = (float) test->bytes / test->minSeconds;
        float avgThroughput = (float) test->bytes / averageSeconds;

        test->maxThroughput = maxThroughput;

        printf(
            "Best : %f s, Throughput: %f gb/s,\n",
            test->minSeconds,
            maxThroughput / (1024.0f *1024.0f *1024.0f)
        );
        printf(
            "Worst: %f s, Throughput: %f gb/s\n",
            test->maxSeconds,
            minThroughput / (1024.0f *1024.0f *1024.0f)
        );
        printf(
            "Avg. : %f s, Throughput: %f gb/s\n",
            averageSeconds,
            avgThroughput / (1024.0f *1024.0f *1024.0f)
        );

        ticks = __rdtsc();
        secondsSinceLastReset = ((float) (ticks - ticksSinceLastReset)) / ((float) rdtscFrequency);

        if (secondsSinceLastReset > 10.0f) {
            break;
        }

        for (size_t i = 0; i < 3; i++) {
            printf("\033[F");
        }
    }

    printf("\n");
}

int main(void) {
    uint64_t rdtscFrequency = estimateRdtscFrequency();

    int64_t bytes = GB(4);

    const int32_t l1size = KB(32);
    const int32_t l2size = KB(512);
    const int32_t l3size = MB(4);

    char *buffer = malloc(bytes);
    assert(buffer);

    Test tests[] = {
        MAKE_TEST(testCache, "L1", bytes, buffer, l1size),
        MAKE_TEST(testCache, "L2", bytes, buffer, l2size),
        MAKE_TEST(testCache, "L3", bytes, buffer, l3size),
        MAKE_TEST(testCache, "main", bytes, buffer, bytes),
        MAKE_TEST(testCacheAnd, "L1 and", bytes, buffer, 0x7fff),
        MAKE_TEST(testCacheAnd, "L2 and", bytes, buffer, 0x3ffff),
        MAKE_TEST(testCacheAnd, "L3 and", bytes, buffer, 0x3fffff),
        MAKE_TEST(testCacheAnd, "main and", bytes, buffer, INT64_MAX),
    };

    while (true) {
        for (size_t i = 0; i < ARRAYSIZE(tests); i++) {
            Test *test = tests + i;

            repeatTest(test, rdtscFrequency);
        }

        float maxThroughput = 0;

        char *bestTest = "";

        for (size_t i = 0; i < ARRAYSIZE(tests); i++) {
            Test test = tests[i];
            if (test.maxThroughput > maxThroughput) {
                maxThroughput = test.maxThroughput;
                bestTest = test.name;
            }
        }

        printf("BEST THROUGHPUT: %s, %f g/s\n\n", bestTest, maxThroughput / (1024.0f *1024.0f *1024.0f));
    }

    return 0;
}
