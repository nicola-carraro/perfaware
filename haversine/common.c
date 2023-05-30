#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"

#ifndef COMMON_C

#define COMMON_C

#define JSON_PATH "data/pairs.json"
#define ANSWERS_PATH "data/answers"

typedef struct {
   char *data;
   size_t size;
} String;

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

#endif