#include "common.c"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "profiler.c"
#include "float.h"

void cmpAllBytes(uint64_t counter, void *buffer);

void decSlow(uint64_t counter, void *buffer);

void nop3x1(uint64_t counter, void *buffer);

void nop1x3(uint64_t counter, void *buffer);

void nop9(uint64_t counter, void *buffer);

void jumps(uint64_t counter, void *buffer);

void align64(uint64_t counter, void *buffer);

void align1(uint64_t counter, void *buffer);

void align15(uint64_t counter, void *buffer);

void align62(uint64_t counter, void *buffer);

void align63(uint64_t counter, void *buffer);

void read1(uint64_t counter, void *buffer);

void read2(uint64_t counter, void *buffer);

void read3(uint64_t counter, void *buffer);

void read4(uint64_t counter, void *buffer);

void write1(uint64_t counter, void *buffer);

void write2(uint64_t counter, void *buffer);

void write3(uint64_t counter, void *buffer);

void write4(uint64_t counter, void *buffer);

void read2x4(uint64_t counter, void *buffer);

void read2x8(uint64_t counter, void *buffer);

void read2x16(uint64_t counter, void *buffer);

void read2x32(uint64_t counter, void *buffer);

void read2x64(uint64_t counter, void *buffer);


#define MAKE_TEST(f, n, r, b) {\
    .minSeconds = FLT_MAX,\
    .function = f,\
    .name = n,\
    .counter = r,\
    .buffer = b\
}

typedef struct {
    float minSeconds;
    float maxSeconds;
    float sumSeconds;
    size_t executionCount;
    void (*function) (uint64_t counter, void *buffer);
    char *name;
    float maxThroughput;
    uint64_t counter;
    void *buffer;
} Test;

void repeatTest(Test *test, uint64_t rdtscFrequency) {
    uint64_t ticksSinceLastReset = __rdtsc();

    float secondsSinceLastReset = 0.0f;

    printf("%s:\n", test->name);

    while (true) {
        uint64_t start = __rdtsc();
        test->function(test->counter, test->buffer);
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

    uint64_t counter = 4LL * 1024LL * 1024LL * 1024LL;

    char buffer[64] = {0};

    // char *allZeros = malloc(counter);
    // memset(allZeros, 1, counter);
    // char *allOnes = malloc(counter);
    // memset(allOnes, 1, counter);
    // char *oneEveryTwo = malloc(counter);
    // for (int i = 0; i < counter; i++) {
    // oneEveryTwo[i] = (i % 2) == 0;
    // }
    // char *random = malloc(counter);
    // for (int i = 0; i < counter; i++) {
    // random[i] = (rand() % 2) == 0;
    // }
    Test tests[] = {
        // MAKE_TEST(decSlow, "decSlow", counter, 0),
        // MAKE_TEST(cmpAllBytes, "cmpAllBytes", counter, 0),
        // MAKE_TEST(align64, "align64", counter, 0),
        // MAKE_TEST(align1, "align1", counter, 0),
        // MAKE_TEST(align15, "align15", counter, 0),
        // MAKE_TEST(align62, "align62", counter, 0),
        // MAKE_TEST(align63, "align63", counter, 0),
        // MAKE_TEST(read1, "read1", counter, buffer),
        // MAKE_TEST(read2, "read2", counter, buffer),
        // MAKE_TEST(read3, "read3", counter, buffer),
        // MAKE_TEST(read4, "read4", counter, buffer),
        // MAKE_TEST(write1, "write1", counter, buffer),
        // MAKE_TEST(write2, "write2", counter, buffer),
        // MAKE_TEST(write3, "write3", counter, buffer),
        // MAKE_TEST(write4, "write4", counter, buffer),
        // MAKE_TEST(jumps, "allOnes", counter, allOnes),
        // MAKE_TEST(jumps, "allZeros", counter, allZeros),
        // MAKE_TEST(jumps, "oneEveryTwo", counter, oneEveryTwo),
        // MAKE_TEST(jumps, "random", counter, random),
        // MAKE_TEST(nop1x3, "nop1x3", counter, NULL),
        // MAKE_TEST(nop3x1, "nop3x1", counter, NULL),
        // MAKE_TEST(nop9, "nop9", counter, NULL),
        MAKE_TEST(read2x4, "read2x4", counter, buffer),
        MAKE_TEST(read2x8, "read2x8", counter, buffer),
        MAKE_TEST(read2x16, "read2x16", counter, buffer),
        MAKE_TEST(read2x32, "read2x32", counter, buffer),
    MAKE_TEST(read2x32, "read2x64", counter, buffer),
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
