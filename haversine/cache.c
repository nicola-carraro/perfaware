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
    .counter = r,\
    .buffer = b,\
    .cacheSize = s\
}

#define KB(b) (1024LL *b)

#define MB(b) (KB(KB(b)))

#define GB(b) (KB(MB(b)))

typedef struct {
    float minSeconds;
    float maxSeconds;
    float sumSeconds;
    size_t executionCount;
    void (*function) (int64_t counter, void *buffer, int64_t cacheSize);
    char *name;
    float maxThroughput;
    int64_t counter;
    int64_t cacheSize;
    void *buffer;
} Test;

void testCache(int64_t counter, void *buffer, int64_t cacheSize);

void repeatTest(Test *test, uint64_t rdtscFrequency) {
    uint64_t ticksSinceLastReset = __rdtsc();

    float secondsSinceLastReset = 0.0f;

    printf("%s:\n", test->name);

    while (true) {
        uint64_t start = __rdtsc();
        test->function(test->counter, test->buffer, test->cacheSize);
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
        float minThroughput = (float) test->counter / test->maxSeconds;
        float maxThroughput = (float) test->counter / test->minSeconds;
        float avgThroughput = (float) test->counter / averageSeconds;

        test->maxThroughput = maxThroughput;

        printf("Best : %f s, Throughput: %f,\n", test->minSeconds, maxThroughput);
        printf("Worst: %f s, Throughput: %f\n", test->maxSeconds, minThroughput);
        printf("Avg. : %f s, Throughput: %f\n", averageSeconds, avgThroughput);

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

    int64_t counter = GB(4);

    const int32_t l1size = KB(32);
    const int32_t l2size = KB(512);
    const int32_t l3size = MB(4);

    char *buffer = malloc(counter);
    assert(buffer);

    Test tests[] = {
        MAKE_TEST(testCache, "L1", counter, buffer, l1size),
        MAKE_TEST(testCache, "L2", counter, buffer, l2size),
        MAKE_TEST(testCache, "L3", counter, buffer, l3size),
        MAKE_TEST(testCache, "main", counter, buffer, counter)
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

        printf("BEST THROUGHPUT: %s, %f\n\n", bestTest, maxThroughput);
    }

    return 0;
}
