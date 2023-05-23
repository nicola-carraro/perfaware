#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "ctype.h"
#include "stdbool.h"
#include "stdlib.h"
#include "assert.h"
#include "stdarg.h"

#define OUTPUT_PATH "data/input.json"

bool isDigits(char *cString)
{

    while (*cString != '\0')
    {
        if (!isdigit(*cString))
        {
            return false;
        }
        cString++;
    }

    return true;
}

float randomFloat(float min, float max)
{
    assert(min <= max);
    float range = max - min;
    int randomInt = rand();

    float result = min + ((range / (float)RAND_MAX) * randomInt);

    return result;
}

void die(const char *file, const size_t line, char *message, ...)
{
    va_list args;
    va_start(args, message);

    char errorMessage[256];
    printf(errorMessage, args);
    va_end(args);

    printf("ERROR(%s:%zu): %s\n", file, line, message);

    exit(EXIT_FAILURE);
}

void writeToFile(FILE *file, const char *fileName, const char *format, ...)
{

    if (fprintf(file, format) < 0)
    {
        die(__FILE__, __LINE__, "Error while writing to %s", fileName);
        return;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s pairs seed", argv[0]);
        return;
    }

    if (!isDigits(argv[1]))
    {
        printf("Error: pairs should be numeric, found: %s", argv[1]);
        return;
    }

    if (!isDigits(argv[2]))
    {
        printf("Error: seed should be numeric, found: %s", argv[2]);
        return;
    }

    size_t pairs = (size_t)atoi(argv[1]);

    int seed = atoi(argv[2]);

    printf("Pairs : %zu\n", pairs);
    printf("Seed  : %d\n", seed);

    srand(seed);

    FILE *file = fopen(OUTPUT_PATH, "w");

    if (!file)
    {
        printf("Error while opening %s", OUTPUT_PATH);
        return;
    }

    writeToFile(file, OUTPUT_PATH, "{\n\t\"pairs\":[\n");

    for (size_t pair = 0; pair < pairs; pair++)
    {
        float x1 = randomFloat(0.0f, 180.0f);
        float y1 = randomFloat(-90.0f, 90.0f);

        float x2 = randomFloat(0.0f, 180.0f);
        float y2 = randomFloat(-90.0f, 90.0f);

        writeToFile(
            file,
            OUTPUT_PATH,
            "\t\t{\"x1\":%f, \"y1\":%f, \"x2\":%f, \"y2\":%f}",
            x1,
            y1,
            x2,
            y2);

        if (pair < pairs - 1)
        {
            writeToFile(file, OUTPUT_PATH, ",\n");
        }
    }

    writeToFile(file, OUTPUT_PATH, "\n\t]\n}");
}
