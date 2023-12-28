#include "common.c"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "profiler.c"
#include "float.h"

void cmpAllBytes(int64_t repetitions, void *buffer);

void decSlow(int64_t repetitions, void *buffer);

void nop3x1(int64_t repetitions, void *buffer);

void nop1x3(int64_t repetitions, void *buffer);

void nop9(int64_t repetitions, void *buffer);

void jumps(int64_t repetitions, void *buffer);

void align64(int64_t repetitions, void *buffer);

void align1(int64_t repetitions, void *buffer);

void align15(int64_t repetitions, void *buffer);

void align62(int64_t repetitions, void *buffer);

void align63(int64_t repetitions, void *buffer);

#define MAKE_TEST(f, n, r, b) {\
    .minSeconds = FLT_MAX,\
    .function = f,\
    .name = n,\
    .repetitions = r,\
    .buffer = b\
}

typedef struct {
    float minSeconds;
    float maxSeconds;
    float sumSeconds;
    size_t executionCount;
    void (*function) (int64_t repetitions, void *buffer);
    char *name;
    float maxThroughput;
    int64_t repetitions;
    void *buffer;
} Test;

void repeatTest(Test *test, uint64_t rdtscFrequency) {
    uint64_t ticksSinceLastReset = __rdtsc();

    float secondsSinceLastReset = 0.0f;

    printf("%s:\n", test->name);

    while (true) {
        uint64_t start = __rdtsc();
        test->function(test->repetitions, test->buffer);
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

    // char *allZeros = malloc(repetitions);
    // memset(allZeros, 1, repetitions);
    // char *allOnes = malloc(repetitions);
    // memset(allOnes, 1, repetitions);
    // char *oneEveryTwo = malloc(repetitions);
    // for (int i = 0; i < repetitions; i++) {
    // oneEveryTwo[i] = (i % 2) == 0;
    // }
    // char *random = malloc(repetitions);
    // for (int i = 0; i < repetitions; i++) {
    // random[i] = (rand() % 2) == 0;
    // }
    Test tests[] = {
        // MAKE_TEST(decSlow, "decSlow", repetitions, 0),
        // MAKE_TEST(cmpAllBytes, "cmpAllBytes", repetitions, 0),
        MAKE_TEST(align64, "align64", repetitions, 0),
        MAKE_TEST(align1, "align1", repetitions, 0),
        MAKE_TEST(align15, "align15", repetitions, 0),
        MAKE_TEST(align62, "align62", repetitions, 0),
        MAKE_TEST(align63, "align63", repetitions, 0),
        // MAKE_TEST(jumps, "allOnes", repetitions, allOnes),
        // MAKE_TEST(jumps, "allZeros", repetitions, allZeros),
        // MAKE_TEST(jumps, "oneEveryTwo", repetitions, oneEveryTwo),
        // MAKE_TEST(jumps, "random", repetitions, random),
        // MAKE_TEST(nop1x3, "nop1x3", repetitions, NULL),
        // MAKE_TEST(nop3x1, "nop3x1", repetitions, NULL),
        // MAKE_TEST(nop9, "nop9", repetitions, NULL),
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
