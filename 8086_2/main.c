#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"

void printPressEnterToContinue()
{
    printf("; Press enter to continue...");

    char byte;
    if (fread(&byte, 1, 1, stdin) != 1)
    {
        perror("Could not read from standard input");
    }
}

void error(char *format, ...)
{
    char errorMessage[256];

    va_list args;
    va_start(args, format);
    vsprintf(errorMessage, format, args);
    va_end(args);
    perror(errorMessage);

    exit(EXIT_FAILURE);
}

void readFile(char *filename, char **bytes, size_t *len)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        error(" Could not open % s ", filename);
    }

    char errorMessage[] = "Could not determine size of %s";
    if (fseek(file, 0, SEEK_END) < 0)
    {
        error(errorMessage, filename);
    }

    *len = ftell(file);
    if (fseek(file, 0, SEEK_SET) < 0)
    {
        error(errorMessage, filename);
    }

    *bytes = malloc(*len);

    if (!bytes)
    {
        error("Allocation failed");
    }

    if (fread(bytes, *len, 1, file) != 1)
    {
        error(" Could not read from % s ", filename);
    }
}

int main(void)
{
    char filename[] = "data/listing37";

    char *bytes;
    size_t len;

    readFile(filename, &bytes, &len);

    if (bytes != NULL)
    {
        free(bytes);
    }

    printPressEnterToContinue();

    return 0;
}
