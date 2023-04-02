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
    di,
    none
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
    union
    {
        MemoryLocation memory;
        RegisterLocation reg;
    } location;
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
    {di, "di", false}};

const struct
{
    RegisterLocation w0Reg;
    RegisterLocation w1Reg;
} RegFieldInfo[] = {
    {{a, l}, {a, x}},
    {{c, l}, {c, x}},
    {{d, l}, {d, x}},
    {{b, l}, {b, x}},
    {{a, h}, {sp}},
    {{c, h}, {bp}},
    {{d, h}, {si}},
    {{b, h}, {di}}};

const struct
{
    RegisterLocation mod3W0Reg;
    RegisterLocation mod3w1Reg;

    MemoryLocation memoryLocation;
} RmFieldInfo[] = {
    {{a, l}, {a, x}, {{b, x}, {si}, 2}},
    {{c, l}, {c, x}, {{b, x}, {di}, 2}},
    {{d, l}, {d, x}, {{bp}, {si}, 2}},
    {{b, l}, {b, x}, {{bp}, {di}, 2}},
    {{a, h}, {sp}, {{si}, {none}, 1}},
    {{c, h}, {bp}, {{di}, {none}, 1}},
    {{d, h}, {si}, {{bp}, {none}, 1}},
    {{b, h}, {di}, {{b, x}, {none}, 1}},
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

void printError(const char *file, const size_t line, const char *format, va_list args)
{
    char errorMessage[256];
    vsprintf(errorMessage, format, args);
    va_end(args);

    printf("ERROR(%s:%zu): ", file, line);
    perror(errorMessage);
}

void error(const char *file, const size_t line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printError(file, line, format, args);

    exit(EXIT_FAILURE);
}

char *readFile(const char *filename, size_t *len)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        error(__FILE__, __LINE__, " Could not open % s ", filename);
    }

    char errorMessage[] = "Could not determine size of %s";
    if (fseek(file, 0, SEEK_END) < 0)
    {
        error(__FILE__, __LINE__, errorMessage, filename);
    }

    *len = ftell(file);
    if (fseek(file, 0, SEEK_SET) < 0)
    {
        error(__FILE__, __LINE__, errorMessage, filename);
    }

    char *bytes = malloc(*len);

    if (!bytes)
    {
        error(__FILE__, __LINE__, "Allocation failed");
    }

    if (fread(bytes, *len, 1, file) != 1)
    {
        error(__FILE__, __LINE__, " Could not read from % s ", filename);
    }

    return bytes;
}

int main(int argc, char *argv[])
{
    if (argc < 1)
    {
        error(__FILE__, __LINE__, "Usage: %s <filename>", argv[0]);
    }

    const char *filename = argv[1];

    size_t len;

    char *bytes = readFile(filename, &len);

    if (bytes != NULL)
    {
        free(bytes);
    }

    printPressEnterToContinue();

    return 0;
}
