#include "common.c"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "profiler.c"
#include "float.h"

void cmpAllBytes(int64_t repetitions);

void decSlow(int64_t repetitions);

#define MAKE_TEST(f, n, r) {\
    .minSeconds = FLT_MAX, .function = f, .name = n, .repetitions = r\
}

typedef struct {
    float minSeconds;
    float maxSeconds;
    float sumSeconds;
    size_t executionCount;
    void (*function) (int64_t repetitions);
    char *name;
    float maxThroughput;
    int64_t repetitions;
} Test;

void repeatTest(Test *test, uint64_t rdtscFrequency) {
    uint64_t ticksSinceLastReset = __rdtsc();

    float secondsSinceLastReset = 0.0f;

    printf("%s:\n", test->name);

    while (true) {
        uint64_t start = __rdtsc();
        test->function(test->repetitions);
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
        float minThroughput = (float) test->repetitions / test->maxSeconds;
        float maxThroughput = (float) test->repetitions / test->minSeconds;
        float avgThroughput = (float) test->repetitions / averageSeconds;

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

    int64_t repetitions = 1000000000;

    Test tests[] = {
        MAKE_TEST(decSlow, "decSlow", repetitions),
        MAKE_TEST(cmpAllBytes, "cmpAllBytes", repetitions),
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
