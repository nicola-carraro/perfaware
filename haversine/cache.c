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

#define KB(b) (1024LL * b)

#define MB(b) (KB(KB(b)))

#define GB(b) (KB(MB(b)))

typedef struct {
    float minSeconds;
    float maxSeconds;
    float sumSeconds;
    size_t executionCount;
    void (*function) (int64_t bytes, void *buffer, int64_t cacheSize);
    char name[256];
    float maxThroughput;
    int64_t bytes;
    int64_t cacheSizeOrMask;
    void *buffer;
} Test;

void testCache(int64_t bytes, void *buffer, int64_t cacheSize);

void testCacheAnd(int64_t bytes, void *buffer, int64_t mask);

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
            maxThroughput / (1024.0f * 1024.0f * 1024.0f)
        );
        printf(
            "Worst: %f s, Throughput: %f gb/s\n",
            test->maxSeconds,
            minThroughput / (1024.0f * 1024.0f * 1024.0f)
        );
        printf(
            "Avg. : %f s, Throughput: %f gb/s\n",
            averageSeconds,
            avgThroughput / (1024.0f * 1024.0f * 1024.0f)
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

int32_t mask(uint8_t bitsToKeep) {
    int32_t result = 0;
    for (int i = 0; i < bitsToKeep; i++) {
        result |= (1 << i);
    }

    return result;
}

int main(void) {
    uint64_t rdtscFrequency = estimateRdtscFrequency();

    int64_t bytes = GB(4);

    // const int32_t l1size = KB(32);
    // const int32_t l2size = KB(512);
    // const int32_t l3size = MB(4);
    char *buffer = malloc(bytes);
    assert(buffer);

    // Test tests[] = {
    // MAKE_TEST(testCacheAnd, "10", bytes, buffer, mask(10)),
    // MAKE_TEST(testCacheAnd, "11", bytes, buffer, mask(11)),
    // MAKE_TEST(testCacheAnd, "12", bytes, buffer, mask(12)),
    // MAKE_TEST(testCacheAnd, "13", bytes, buffer, mask(13)),
    // MAKE_TEST(testCacheAnd, "14", bytes, buffer, mask(14)),
    // MAKE_TEST(testCacheAnd, "15", bytes, buffer, mask(15)),
    // MAKE_TEST(testCacheAnd, "16", bytes, buffer, mask(16)),
    // MAKE_TEST(testCacheAnd, "17", bytes, buffer, mask(17)),
    // MAKE_TEST(testCacheAnd, "18", bytes, buffer, mask(18)),
    // MAKE_TEST(testCacheAnd, "19", bytes, buffer, mask(19)),
    // MAKE_TEST(testCacheAnd, "20", bytes, buffer, mask(20)),
    // MAKE_TEST(testCacheAnd, "21", bytes, buffer, mask(21)),
    // MAKE_TEST(testCacheAnd, "22", bytes, buffer, mask(22)),
    // MAKE_TEST(testCacheAnd, "23", bytes, buffer, mask(23)),
    // MAKE_TEST(testCacheAnd, "24", bytes, buffer, mask(24)),
    // MAKE_TEST(testCacheAnd, "25", bytes, buffer, mask(25)),
    // MAKE_TEST(testCacheAnd, "26", bytes, buffer, mask(26)),
    // MAKE_TEST(testCacheAnd, "27", bytes, buffer, mask(27)),
    // MAKE_TEST(testCacheAnd, "28", bytes, buffer, mask(28)),
    // MAKE_TEST(testCacheAnd, "29", bytes, buffer, mask(29)),
    // MAKE_TEST(testCacheAnd, "30", bytes, buffer, mask(30)),
    // MAKE_TEST(testCacheAnd, "31", bytes, buffer, mask(31)),
    // };
    const int32_t numTests = 200;

    const int32_t testsSize = sizeof(Test) * numTests;
    Test *tests = malloc(testsSize);
    memset(tests, 0, testsSize);
    int32_t increment = 512;
    int32_t size = 1024;
    for (int i = 0; i < numTests; i++) {
        if (i % 20 == 0) {
            increment *= 2;
        }
        char name[256] = {0};
        int64_t testBytes = (bytes / size) * size;

        sprintf(name, "%d", size);
        Test *test = &tests[i];
        test->minSeconds = FLT_MAX;
        test->function = testCache;
        strncpy(test->name, name, sizeof(test->name));
        test->name[ARRAYSIZE(test->name) - 1] = '\0';
        test->bytes = testBytes;
        test->buffer = buffer;
        test->cacheSizeOrMask = size;
        size += increment;
    }

    while (true) {
        for (size_t i = 0; i < numTests; i++) {
            Test *test = tests + i;

            repeatTest(test, rdtscFrequency);
        }

        float maxThroughput = 0;

        char *bestTest = "";

        for (size_t i = 0; i < numTests; i++) {
            Test test = tests[i];
            if (test.maxThroughput > maxThroughput) {
                maxThroughput = test.maxThroughput;
                bestTest = test.name;
            }
        }

        printf("BEST THROUGHPUT: %s, %f g/s\n\n", bestTest, maxThroughput / (1024.0f * 1024.0f * 1024.0f));

        FILE *f = fopen("cache.csv", "wb");

        assert(f);

        fprintf(f, "Size; Throughput\n");
        for (size_t i = 0; i < numTests; i++) {
            Test test = tests[i];
            fprintf(
                f,
                "%lld; %f\n",
                test.cacheSizeOrMask,
                test.maxThroughput / (1024.0f * 1024.0f * 1024.0f)
            );
        }

        fclose(f);
    }

    return 0;
}
