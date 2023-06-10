#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "assert.h"
#include "math.h"

#ifndef COMMON_C

#define COMMON_C

#define JSON_PATH "data/pairs.json"
#define ANSWERS_PATH "data/answers"

#define ARENA_SIZE 1024 * 1024 * 1024

#define EARTH_RADIUS 6371

typedef struct
{
    char *data;
    size_t size;
} String;

typedef struct
{
    void *memory;
    size_t size;
    size_t offset;
} Arena;

void die(const char *file, const size_t line, int errorNumber, const char *message, ...)
{
    printf("ERROR (%s:%zu): ", file, line);

    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    if (errorNumber != 0)
    {
        char *cError = strerror(errorNumber);
        printf(" (%s)", cError);
    }

    exit(EXIT_FAILURE);
}

void writeTextToFile(FILE *file, const char *path, const char *format, ...)
{
    va_list varargsList;

    va_start(varargsList, format);

    if (vfprintf(file, format, varargsList) < 0)
    {
        int error = errno;
        fclose(file);
        die(__FILE__, __LINE__, error, "could not write to %s", path);
    }
    va_end(varargsList);
}



void writeBinaryToFile(FILE *file, const char *path, void *data, size_t size, size_t count)
{
    if (fwrite(data, size, count, file) != count)
    {
        int error = errno;
        fclose(file);
        die(__FILE__, __LINE__, error, "could not write to %s", path);
    }
}

size_t getFileSize(FILE *file, char *path)
{
    size_t result = 0;

    const char *errorMessage = "could not query the size of %s";

    if (fseek(file, 0, SEEK_END) != 0)
    {
        int errorNumber = errno;
        fclose(file);
        die(__FILE__, __LINE__, errorNumber, errorMessage, path);
    }
    else
    {
        long cursorPosition = ftell(file);

        if (cursorPosition >= 0)
        {
            result = (size_t)cursorPosition;

            if (fseek(file, 0, SEEK_SET) != 0)
            {
                int errorNumber = errno;
                fclose(file);
                die(__FILE__, __LINE__, errorNumber, "could not restore cursor position of %s", path);
            }
        }
        else
        {
            int errorNumber = errno;
            fclose(file);
            die(__FILE__, __LINE__, errorNumber, errorMessage, path);
        }
    }

    return result;
}

Arena arenaInit()
{

    Arena arena = {0};

    arena.memory = malloc(ARENA_SIZE);

    if (!arena.memory)
    {
        die(__FILE__, __LINE__, errno, "could not initialize arena");
    }

    arena.size = ARENA_SIZE;
    arena.offset = 0;

    return arena;
}

void *arenaAllocate(Arena *arena, size_t size)
{
    assert(arena != NULL);

    void *result = NULL;

    if (arena->offset + size >= arena->size)
    {
        die(__FILE__, __LINE__, errno, "could not allocate from arena");
    }

    result = (char *)arena->memory + arena->offset;
    arena->offset += size;

    return result;
}

String readFileToString(char *path, Arena *arena)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL)
    {
        die(__FILE__, __LINE__, errno, "could not open %s", path);
    }
    String result = {0};
    result.size = getFileSize(file, path);
    result.data = arenaAllocate(arena, result.size);

    size_t read = fread(result.data, 1, result.size, file);

    if (read < result.size)
    {
        die(__FILE__, __LINE__, errno, "could not read %s, read %zu", path, read);
    }

    return result;
}

double degreesToRadians(double degrees)
{
    return degrees * 0.01745329251994329577;
}

double square(double n)
{
    return n * n;
}

double haversine(double x1Degrees, double y1Degrees, double x2Degrees, double y2Degrees, double radius)
{

    double x1Radians = degreesToRadians(x1Degrees);
    double y1Radians = degreesToRadians(y1Degrees);
    double x2Radians = degreesToRadians(x2Degrees);
    double y2Radians = degreesToRadians(y2Degrees);

    double rootTerm = square(sin((y2Radians - y1Radians) / 2.0)) + cos(y1Radians) * cos(y2Radians) * square(sin((x2Radians - x1Radians) / 2.0));

    double result = 2.0 * radius * asin(sqrt(rootTerm));

    return result;
}

#endif