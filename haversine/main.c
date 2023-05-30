
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

#include "common.c"

int main(void)
{

    FILE *jsonFile = fopen(JSON_PATH, "r");

    size_t size = getFileSize(jsonFile, JSON_PATH);

    printf("size: %zu", size);

    return 0;
}