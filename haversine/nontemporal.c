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
    .rangeSizeOrMask = s\
}

#define KB(b) (1024LL * b)

#define MB(b) (KB(KB(b)))

#define GB(b) (KB(MB(b)))

typedef struct {
    float minSeconds;
    float maxSeconds;
    float sumSeconds;
    size_t executionCount;
    void (*function) (int64_t bytes, void *source, void *destination);
    char name[256];
    float maxThroughput;
    int64_t bytes;
    int64_t cacheLineBits;
    int64_t cacheSetBits;
    void *source;
    void *destination;
} Test;

void testCache(int64_t bytes, void *buffer, int64_t rangeSize);

void testCacheAnd(int64_t bytes, void *buffer, int64_t mask);

void testCacheUnaligned(int64_t bytes, void *buffer, int64_t mask, int64_t offset);

void testCacheUnalignedNonContiguous(int64_t bytes, void *buffer, int64_t mask, int64_t offset);

void testCacheSet(int64_t bytes, void *buffer);

void cacheSetComparison(int64_t bytes, void *buffer);

void testTemporal(int64_t bytes, void *source, void *destination);

void testNonTemporal(int64_t bytes, void *source, void *destination);

void repeatTest(Test *test, uint64_t rdtscFrequency) {
    uint64_t ticksSinceLastReset = __rdtsc();

    float secondsSinceLastReset = 0.0f;

    printf("%s\n", test->name);

    while (true) {
        uint64_t start = __rdtsc();
        test->function(test->bytes, test->source, test->destination);
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
    char *destination = malloc(bytes * 2);
    assert(destination);

    int64_t rangeSize = KB(1);

    char *source = malloc(rangeSize);
    assert(source);

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
    int32_t numTests = 2;
    Test tests[2] = {0};

    char name[256] = {0};

    sprintf(name, "%d/%d (%lld: %s)", 0, numTests, rangeSize, "testTemporal");
    Test *test = tests;
    test->minSeconds = FLT_MAX;
    test->function = testTemporal;
    strncpy(test->name, name, sizeof(test->name));
    test->name[ARRAYSIZE(test->name) - 1] = '\0';
    test->bytes = bytes;
    test->source = source;
    test->destination = destination;

    sprintf(name, "%d/%d (%lld: %s)", 1, numTests, rangeSize, "testNonTemporal");
    test = tests + 1;
    test->minSeconds = FLT_MAX;
    test->function = testNonTemporal;
    strncpy(test->name, name, sizeof(test->name));
    test->name[ARRAYSIZE(test->name) - 1] = '\0';
    test->bytes = bytes;
    test->source = source;
    test->destination = destination;

    while (true) {
        for (size_t i = 0; i < numTests; i++) {
            test = tests + i;

            repeatTest(test, rdtscFrequency);
        }

        float maxThroughput = 0;

        char bestTest[256] = "";

        for (size_t i = 0; i < numTests; i++) {
            test = tests + i;
            if (test->maxThroughput > maxThroughput) {
                maxThroughput = test->maxThroughput;
                strncpy(bestTest, test->name, sizeof(bestTest));
            }
        }

        printf("BEST THROUGHPUT: %s, %f g/s\n\n", bestTest, maxThroughput / (1024.0f * 1024.0f * 1024.0f));

        /*
       FILE *f = fopen("cache.csv", "wb");
    
       assert(f);
    
       fprintf(f, "Size;");
    
       for (int32_t i = 0; i < offsetsPerSize; i++) {
       fprintf(f, "Offset %d; ", i);
       }
    
       fprintf(f, "\n");
    
       for (int32_t i = 0; i < sizesCount; i++) {
       Test test = tests[i * sizesCount];
    
       fprintf(f, "%lld; ", test.rangeSizeOrMask);
    
       for (int32_t j = 0; j < offsetsPerSize; j++) {
       test = tests[i * sizesCount + j];
       fprintf(f, "%f; ", test.maxThroughput / (1024.0f * 1024.0f * 1024.0f));
       }
    
       fprintf(f, "\n");
       }
    
       fclose(f);
    */
    }

    return 0;
}
