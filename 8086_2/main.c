#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "stdbool.h"
#include "stdint.h"

#define REG_COUNT 8

typedef enum
{
    a,
    b,
    c,
    d,
    sp,
    bp,
    si,
    di
} Register;

typedef enum
{
    x,
    l,
    h
} RegisterPortion;

typedef struct
{
    Register reg;
    RegisterPortion portion;
} RegisterLocation;

typedef struct
{
    RegisterLocation reg0;
    RegisterLocation reg1;
    uint8_t regCount;
    int16_t displacement;
} MemoryLocation;

typedef enum
{
    memoryOperand,
    registerOperand,
    immediateOperand
} OperandType;

typedef struct
{
    OperandType type;
    union location
    {
        MemoryLocation memory;
        RegisterLocation reg;
    };
} Operand;

const struct
{
    const Register reg;
    const char name[3];
    const bool isPartiallyAdressable;
} RegisterInfos[] = {
    {a, "a", true},
    {b, "b", true},
    {c, "c", true},
    {d, "d", true},
    {sp, "sp", false},
    {bp, "bp", false},
    {si, "si", false},
    {di, "di", false}
};

void printPressEnterToContinue()
{
    printf("; Press enter to continue...");

    char byte;
    if (fread(&byte, 1, 1, stdin) != 1)
    {
        perror("Could not read from standard input");
    }
}

void printError(const char *format, va_list args)
{
    char errorMessage[256];
    vsprintf(errorMessage, format, args);
    va_end(args);
    perror(errorMessage);
}

void error(const char *format, ...)
{

    va_list args;
    va_start(args, format);
    printError(format, args);

    exit(EXIT_FAILURE);
}

char *readFile(const char *filename, size_t *len)
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

    char *bytes = malloc(*len);

    if (!bytes)
    {
        error("Allocation failed");
    }

    if (fread(bytes, *len, 1, file) != 1)
    {
        error(" Could not read from % s ", filename);
    }

    return bytes;
}

int main(void)
{
    char filename[] = "data/listing37f";

    size_t len;

    char *bytes = readFile(filename, &len);

    if (bytes != NULL)
    {
        free(bytes);
    }

    printPressEnterToContinue();

    return 0;
}
