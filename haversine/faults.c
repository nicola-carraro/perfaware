#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "common.c"

#define PAGE_SIZE (4 *1024)

int main(void) {
    FILE *csv = fopen("faults.csv", "wb");

    assert(csv);

    const size_t totalPages = 4096;

    size_t size = totalPages * PAGE_SIZE;

    HANDLE process = GetCurrentProcess();

    fprintf(csv, "%s; %s; %s; %s\n", "Pages", "Faults", "Size", "Extra faults");

    for (size_t pages = 1; pages <= totalPages; pages++) {
        uint8_t *data = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        size_t touchSize = pages * PAGE_SIZE;

        uint64_t startFaults = getPageFaultCount(process);

        for (size_t byte = 0; byte < touchSize; byte++) {
            data[byte] = (uint8_t) byte;
        }

        uint64_t endFaults = getPageFaultCount(process);

        VirtualFree(data, 0, MEM_RELEASE);

        uint64_t faults = endFaults - startFaults;

        uint64_t extraFaults = faults - pages;

        fprintf(csv, "%zu; %llu; %llu; %llu\n", pages, faults, touchSize, extraFaults);
    }

    fclose(csv);

    return 0;
}
