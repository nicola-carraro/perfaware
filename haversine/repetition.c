#include "common.c"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "profiler.c"
#include "float.h"
#include "intsafe.h"

#pragma warning(push, 0)

#include "windows.h"

#pragma warning(pop)

#include "psapi.h"

#pragma

#include "io.h"
#include "sys\stat.h"
#include "fcntl.h"

void movAllBytes(size_t size, void *buffer);

void nopAllBytes(size_t size, void *buffer);

void cmpAllBytes(size_t size, void *buffer);

void decAllBytes(size_t size);

void incAllBytes(size_t size);

typedef enum {
    Alloc_None,
    Alloc_Malloc,
    Alloc_VirtualAlloc,
    Alloc_Large
} Alloc;

#define MAKE_TEST(f, n, a) {\
    .minSeconds = FLT_MAX, .minPageFaults = UINT64_MAX, .function = f, .name = n, .alloc = a\
}

typedef struct {
    uint64_t ticks;
    uint64_t bytes;
    uint64_t pageFaults;
    uint64_t startTicks;
    uint64_t startPageFaults;
} Iteration;

typedef struct {
    float minSeconds;
    float maxSeconds;
    float sumSeconds;
    uint64_t minPageFaults;
    uint64_t maxPageFaults;
    uint64_t sumPageFaults;
    size_t executionCount;
    void (*function) (Iteration *iteration, Arena *arena, Alloc alloc, HANDLE process, uint64_t largePageMinimum);
    char *name;
    float maxGbPerSecond;
    Alloc alloc;
} Test;

char *allocate(Alloc alloc, Arena *arena, size_t size, uint64_t largePageMinimum) {
    char *result = 0;
    switch (alloc) {
        case Alloc_None : {
            result = arenaAllocate(arena, size);
        }
        break;
        case Alloc_Malloc : {
            result = malloc(size);
        }
        break;
        case Alloc_VirtualAlloc : {
            result = VirtualAlloc(
                0,
                size,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE
            );
        }
        break;
        case Alloc_Large : {
            size_t remainder = size % largePageMinimum;

            if (remainder != 0) {
                size += (largePageMinimum - remainder);
            }

            result = VirtualAlloc(
                0,
                size,
                MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES,
                PAGE_READWRITE
            );
        }
        break;
        default : {
            assert(false);
        }
    }

    return result;
}

void freeAllocation(Alloc alloc, Arena *arena, char *buffer) {
    switch (alloc) {
        case Alloc_None : {
            arenaFreeAll(arena);
        }
        break;
        case Alloc_Malloc : {
            free(buffer);
        }
        break;
        case Alloc_VirtualAlloc : case Alloc_Large : {
            VirtualFree(buffer, 0, MEM_RELEASE);
        }
        break;
        default : {
            assert(false);
        }
    }
}

void iterationStartCounters(Iteration *iteration, HANDLE process, uint64_t bytes) {
    iteration->startPageFaults = getPageFaultCount(process);
    iteration->startTicks = __rdtsc();
    iteration->bytes = bytes;
}

void iterationStopCounters(Iteration *iteration, HANDLE process) {
    uint64_t ticks = __rdtsc();
    uint64_t pageFaults = getPageFaultCount(process);
    iteration->pageFaults = pageFaults - iteration->startPageFaults;
    iteration->ticks = ticks - iteration->startTicks;
}

void repeatTest(
    Test *test,
    uint64_t rdtscFrequency,
    Arena *arena,
    HANDLE process,
    uint64_t largePageMinimum
) {
    uint64_t ticksSinceLastReset = __rdtsc();

    float secondsSinceLastReset = 0.0f;

    printf("%s:\n", test->name);

    while (true) {
        Iteration iteration = {0};

        test->function(&iteration, arena, test->alloc, process, largePageMinimum);

        float secondsForFunction = (float) iteration.ticks / (float) rdtscFrequency;
        uint64_t pageFaults = iteration.pageFaults;

        if (secondsForFunction < test->minSeconds) {
            test->minSeconds = secondsForFunction;

            secondsSinceLastReset = 0.0f;

            ticksSinceLastReset = __rdtsc();
        }

        if (secondsForFunction > test->maxSeconds) {
            test->maxSeconds = secondsForFunction;
        }

        if (pageFaults < test->minPageFaults) {
            test->minPageFaults = pageFaults;
        }

        if (pageFaults > test->maxPageFaults) {
            test->maxPageFaults = pageFaults;
        }

        test->executionCount++;
        test->sumSeconds += secondsForFunction;
        test->sumPageFaults += pageFaults;

        float averageSeconds = test->sumSeconds / (float) test->executionCount;
        float averagePageFaults = (float) test->sumPageFaults / (float) test->executionCount;

        float gb = ((float) iteration.bytes) / (1024.0f * 1024.0f * 1024.0f);
        float kb = ((float) iteration.bytes) / 1024.0f;

        float minGbPerSecond = gb / test->maxSeconds;
        float maxGbPerSecond = gb / test->minSeconds;
        float averageGbPerSecond = gb / averageSeconds;

        test->maxGbPerSecond = maxGbPerSecond;

        float minKbPerFault = kb / (float) test->maxPageFaults;
        float maxKbPerFault = kb / (float) test->minPageFaults;
        float averageKbPerFault = kb / averagePageFaults;

        printf(
            "Best : %f s, Throughput: %f Gb/s, PF: %f (%f kb/fault)\n",
            test->minSeconds,
            maxGbPerSecond,
            (float) test->minPageFaults,
            maxKbPerFault
        );
        printf(
            "Worst: %f s, Throughput: %f Gb/s, PF: %f (%f kb/fault)\n",
            test->maxSeconds,
            minGbPerSecond,
            (float) test->maxPageFaults,
            minKbPerFault
        );
        printf(
            "Avg. : %f s, Throughput: %f Gb/s, PF: %f (%f kb/fault)\n",
            averageSeconds,
            averageGbPerSecond,
            averagePageFaults,
            averageKbPerFault
        );

        uint64_t ticks = __rdtsc();
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

void readWithFread(
    Iteration *iteration,
    Arena *arena,
    Alloc alloc,
    HANDLE process,
    uint64_t largePageMinimum
) {
    FILE *file = fopen(JSON_PATH, "rb");

    if (file != NULL) {
        size_t size = getFileSize(file, JSON_PATH);

        char *buffer = 0;

        buffer = allocate(alloc, arena, size, largePageMinimum);

        iterationStartCounters(iteration, process, size);

        size_t read = fread(buffer, 1, size, file);

        iterationStopCounters(iteration, process);

        fclose(file);

        freeAllocation(alloc, arena, buffer);

        if (read < 1) {
            die(__FILE__, __LINE__, errno, "Read failed");
        }
    }
}

void readWith_read(
    Iteration *iteration,
    Arena *arena,
    Alloc alloc,
    HANDLE process,
    uint64_t largePageMinimum
) {
    int fd = _open(JSON_PATH, _O_RDONLY | _O_BINARY);

    if (fd != 0) {
        struct __stat64 fileStat = {0};
        if (_stat64(JSON_PATH, &fileStat) == 0) {
            size_t size = fileStat.st_size;
            char *buffer = 0;
            buffer = allocate(alloc, arena, size, largePageMinimum);
            char *cursor = buffer;
            size_t remainingBytes = size;
            iterationStartCounters(iteration, process, size);
            while (remainingBytes > 0) {
                int bytesToRead = 0;
                if (size > INT_MAX) {
                    bytesToRead = INT_MAX;
                } else {
                    bytesToRead = (int) size;
                } int read = _read(fd, cursor, bytesToRead);
                remainingBytes -= read;
                cursor += read;
                if (read < size) {
                    die(__FILE__, __LINE__, errno, "Read failed");
                } } iterationStopCounters(iteration, process);
            _close(fd);
            freeAllocation(alloc, arena, buffer);
        }
    }
}

void readWithReadFile(
    Iteration *iteration,
    Arena *arena,
    Alloc alloc,
    HANDLE process,
    uint64_t largePageMinimum
) {
    HANDLE file = CreateFile(
        JSON_PATH,
        GENERIC_READ,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER size = {0};
        if (GetFileSizeEx(file, &size)) {
            char *buffer = 0;

            buffer = allocate(alloc, arena, size.QuadPart, largePageMinimum);

            char *cursor = buffer;

            uint64_t remainingBytes = (uint64_t) size.QuadPart;

            iterationStartCounters(iteration, process, size.QuadPart);

            while (remainingBytes > 0) {
                DWORD bytesToRead = 0;
                if (remainingBytes > DWORD_MAX) {
                    bytesToRead = DWORD_MAX;
                }
                else {
                    bytesToRead = (DWORD) remainingBytes;
                }

                DWORD read = 0;

                BOOL success = ReadFile(file, cursor, bytesToRead, &read, NULL);

                cursor += read;
                remainingBytes -= read;
                if (!success || bytesToRead != read) {
                    break;
                }
            }
            iterationStopCounters(iteration, process);

            if (remainingBytes > 0) {
                die(__FILE__, __LINE__, 0, "Error while reading file");
            }

            freeAllocation(alloc, arena, buffer);
        }

        CloseHandle(file);
    }
}

void movBuffer(
    Iteration *iteration,
    Arena *arena,
    Alloc alloc,
    HANDLE process,
    uint64_t largePageMinimum
) {
    FILE *file = fopen(JSON_PATH, "rb");

    if (file != NULL) {
        size_t size = getFileSize(file, JSON_PATH);
        char *buffer = 0;

        buffer = allocate(alloc, arena, size, largePageMinimum);

        iterationStartCounters(iteration, process, size);

        movAllBytes(size, buffer);

        iterationStopCounters(iteration, process);

        freeAllocation(alloc, arena, buffer);

        fclose(file);
    }
}

void nopBuffer(
    Iteration *iteration,
    Arena *arena,
    Alloc alloc,
    HANDLE process,
    uint64_t largePageMinimum
) {
    FILE *file = fopen(JSON_PATH, "rb");

    if (file != NULL) {
        size_t size = getFileSize(file, JSON_PATH);
        char *buffer = 0;

        buffer = allocate(alloc, arena, size, largePageMinimum);

        iterationStartCounters(iteration, process, size);

        nopAllBytes(size, buffer);

        iterationStopCounters(iteration, process);

        freeAllocation(alloc, arena, buffer);

        fclose(file);
    }
}

void cmpBuffer(
    Iteration *iteration,
    Arena *arena,
    Alloc alloc,
    HANDLE process,
    uint64_t largePageMinimum
) {
    FILE *file = fopen(JSON_PATH, "rb");

    if (file != NULL) {
        size_t size = getFileSize(file, JSON_PATH);
        char *buffer = 0;

        buffer = allocate(alloc, arena, size, largePageMinimum);

        iterationStartCounters(iteration, process, size);

        cmpAllBytes(size, buffer);

        iterationStopCounters(iteration, process);

        freeAllocation(alloc, arena, buffer);

        fclose(file);
    }
}

void dec(
    Iteration *iteration,
    Arena *arena,
    Alloc alloc,
    HANDLE process,
    uint64_t largePageMinimum
) {
    UNREFERENCED_PARAMETER(arena);
    UNREFERENCED_PARAMETER(alloc);
    UNREFERENCED_PARAMETER(arena);
    UNREFERENCED_PARAMETER(largePageMinimum);

    FILE *file = fopen(JSON_PATH, "rb");

    if (file != NULL) {
        size_t size = getFileSize(file, JSON_PATH);

        iterationStartCounters(iteration, process, size);

        decAllBytes(size);

        iterationStopCounters(iteration, process);

        fclose(file);
    }
}

void writeBuffer(
    Iteration *iteration,
    Arena *arena,
    Alloc alloc,
    HANDLE process,
    uint64_t largePageMinimum
) {
    FILE *file = fopen(JSON_PATH, "rb");

    if (file != NULL) {
        size_t size = getFileSize(file, JSON_PATH);
        char *buffer = 0;

        buffer = allocate(alloc, arena, size, largePageMinimum);

        iterationStartCounters(iteration, process, size);

        for (uint64_t i = 0; i < size; i++) {
            buffer[i] = (char) i;
        }

        iterationStopCounters(iteration, process);

        freeAllocation(alloc, arena, buffer);

        fclose(file);
    }
}

uint64_t enableLargePages() {
    HANDLE token = INVALID_HANDLE_VALUE;

    uint64_t result = 0;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
        TOKEN_PRIVILEGES privileges = {
            .PrivilegeCount = 1,
            .Privileges = {
                {
                    .Attributes = SE_PRIVILEGE_ENABLED
                }
            }
        };

        if (LookupPrivilegeValue(
            NULL,
            SE_LOCK_MEMORY_NAME,
            &privileges.Privileges[0] .Luid
        )) {
            AdjustTokenPrivileges(token, FALSE, &privileges, 0, NULL, NULL);

            DWORD error = GetLastError();
            if (error == ERROR_SUCCESS) {
                result = GetLargePageMinimum();
            }
            else {
                printf("Could not adjust privileges, error = %d\n", error);
            }
        }
        else {
            printf("Could not lookup privileges\n");
        }
    }

    else {
        printf("Could not open process token\n");
    }

    return result;
}

int main(void) {
    uint64_t rdtscFrequency = estimateRdtscFrequency();

    Arena arena = arenaInit();

    HANDLE process = GetCurrentProcess();

    uint64_t largePageMinimum = enableLargePages();

    Test tests[] = {
        MAKE_TEST(dec, "dec", Alloc_None),
        // MAKE_TEST(writeBuffer, "writeBuffer", Alloc_None),
        // MAKE_TEST(movBuffer, "movBuffer", Alloc_None),
        // MAKE_TEST(cmpBuffer, "cmpBuffer", Alloc_None),
        // MAKE_TEST(nopBuffer, "nopBuffer", Alloc_None),
        // MAKE_TEST(writeBuffer, "malloc + writeBuffer", Alloc_Malloc),
        // MAKE_TEST(readWith_read, "VirtualAlloc + writeBuffer", Alloc_VirtualAlloc),
        // MAKE_TEST(readWith_read, "VirtualAlloc (large) + writeBuffer", Alloc_Large),
        // MAKE_TEST(readWith_read, "_read", Alloc_None),
        // MAKE_TEST(readWith_read, "malloc + _read", Alloc_Malloc),
        // MAKE_TEST(readWith_read, "VirtualAlloc + _read", Alloc_VirtualAlloc),
        // MAKE_TEST(readWith_read, "VirtualAlloc (large) + _read", Alloc_Large),
        // MAKE_TEST(readWithFread, "fread", Alloc_None),
        // MAKE_TEST(readWithFread, "malloc + fread", Alloc_Malloc),
        // MAKE_TEST(readWithFread, "VirtualAlloc + fread", Alloc_VirtualAlloc),
        // MAKE_TEST(readWithFread, "VirtualAlloc (large) + fread", Alloc_Large),
        // MAKE_TEST(readWithReadFile, "ReadFile", Alloc_None),
        // MAKE_TEST(readWithReadFile, "malloc + ReadFile", Alloc_Malloc),
        // MAKE_TEST(readWithReadFile, "VirtualAlloc + ReadFile", Alloc_VirtualAlloc),
        // MAKE_TEST(readWithReadFile, "VirtualAlloc (large) + ReadFile", Alloc_Large),
    };

    while (true) {
        for (size_t i = 0; i < ARRAYSIZE(tests); i++) {
            Test *test = tests + i;


            if (test->alloc != Alloc_Large || largePageMinimum != 0) {
                repeatTest(
                    test,
                    rdtscFrequency,
                    &arena,
                    process,
                    largePageMinimum
                );
            }
        }

        float maxGbPerSecond = 0;

        char *bestTest = "";

        for (size_t i = 0; i < ARRAYSIZE(tests); i++) {
            Test test = tests[i];
            if (test.maxGbPerSecond > maxGbPerSecond) {
                maxGbPerSecond = test.maxGbPerSecond;
                bestTest = test.name;
            }
        }

        printf("BEST THROUGHPUT: %s, %f Gb/s\n\n", bestTest, maxGbPerSecond);
    }

    return 0;
}
