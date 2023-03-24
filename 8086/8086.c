#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdint.h"
#include "assert.h"
#include "stdbool.h"
#include "string.h"

char *getRegisterName(uint8_t registerIndex, bool wBit);

void decodeDirectAddressing(FILE *input, char *dest);

uint8_t readUnsignedByte(FILE *input);

#define ARR_COUNT(a) (sizeof(a) / sizeof(*a))

typedef struct
{
    uint8_t registerIndex;
    char w0RegName[3];
    char w1RegName[3];
    char rmExpression[20];
} RegisterName;

RegisterName registerNames[] = {
    {0, "al", "ax", "[bx + si%s]"},
    {1, "cl", "cx", "[bx + di%s]"},
    {2, "dl", "dx", "[bp + si%s]"},
    {3, "bl", "bx", "[bp + di%s]"},
    {4, "ah", "sp", "[si%s]"},
    {5, "ch", "bp", "[di%s]"},
    {6, "dh", "si", "[bp%s]"},
    {7, "bh", "di", "[bx%s]"}};

bool extractWBit(uint8_t firstByte)
{
    bool result = firstByte & 0x01;

    return result;
}

bool extractDOrSBit(uint8_t firstByte)
{
    bool result = (firstByte & 0x02) >> 1;

    return result;
}

void extractHighBits(int16_t *dest, FILE *input)
{
    uint8_t highBits = readUnsignedByte(input);
    *dest |= (((uint16_t)highBits) << 8);
}

void decodeDisplacement(uint8_t mod, uint8_t wBit, FILE *input, char *displacementExpression)
{
    if (mod == 0)
    {
        displacementExpression[0] = 0;
    }
    else
    {
        uint8_t thirdByte = readUnsignedByte(input);
        bool sign = thirdByte >> 7;
        int16_t displacement;

        bool isSigned = false;
        displacement = thirdByte;
        if (sign && wBit && mod == 1)
        {
            displacement = displacement | (0xff << 8);
            isSigned = true;
        }

        if (mod == 2)
        {
            extractHighBits(&displacement, input);
        }
        sprintf(displacementExpression, " %s %d", isSigned ? "" : "+", displacement);
    }
}

uint8_t extractMod(uint8_t secondByte)
{
    uint8_t result = secondByte >> 6;

    return result;
}

uint8_t extractRmField(uint8_t byte)
{
    uint8_t result = (byte & 0x07);

    return result;
}

void decodeImmediateToAccumulator(FILE *input, uint8_t firstByte, uint8_t secondByte)
{
    int16_t immediate = secondByte;
    bool wBit = extractWBit(firstByte);
    if (wBit)
    {
        extractHighBits(&immediate, input);
        printf("ax, ");
    }
    else
    {
        bool sign = secondByte >> 7;

        if (sign)
        {
            immediate = immediate | (0xff << 8);
        }
        printf("al, ");
    }

    printf("%d", immediate);
}

void decodeRegisterMemory(FILE *input, uint8_t firstByte, uint8_t secondByte)
{

    uint8_t rmField = extractRmField(secondByte);
    uint8_t mod = extractMod(secondByte);
    bool dBit = extractDOrSBit(firstByte);
    bool wBit = extractWBit(firstByte);

    if (wBit)
    {
        printf("word ");
    }
    else
    {
        printf("byte ");
    }

    if (mod == 3) // Register
    {
        char *rmFieldRegName = getRegisterName(rmField, wBit);

        if (dBit)
        {
            printf("%s", rmFieldRegName);
        }
        else
        {
            printf("%s, ", rmFieldRegName);
        }
    }
    else if (mod < 3) // Memory
    {
        char templateExpression[256];
        strcpy(templateExpression, registerNames[rmField].rmExpression);

        char displacementExpression[256];
        decodeDisplacement(mod, wBit, input, displacementExpression);
        char memoryExpression[256];

        if (mod == 0 && rmField == 6)
        {

            decodeDirectAddressing(input, memoryExpression);
        }
        else
        {
            sprintf(memoryExpression, templateExpression, displacementExpression);
        }

        if (dBit)
        {
            printf("%s", memoryExpression);
        }
        else
        {
            printf("%s, ", memoryExpression);
        }
    }
    else
    {
        assert(false && "Unknown mod field");
    }
}

uint8_t readByte(FILE *input)
{
    uint8_t result = 0;
    if (fread(&result, sizeof(result), 1, input) != 1)
    {
        assert(false && "Could not read");
    }

    return result;
}

void decodeRegisterMemoryToFromMemory(FILE *input, uint8_t firstByte, uint8_t secondByte)
{
    bool dBit = extractDOrSBit(firstByte);
    bool wBit = extractWBit(firstByte);

    uint8_t mod = extractMod(secondByte);

    uint8_t regField = (secondByte & 0x3f) >> 3;
    uint8_t rmField = extractRmField(secondByte);

    char *regFieldRegName = getRegisterName(regField, wBit);

    if (mod == 3) // Register to register
    {
        char *rmFieldRegName = getRegisterName(rmField, wBit);

        if (dBit)
        {
            printf("%s, ", regFieldRegName);
            printf("%s", rmFieldRegName);
        }
        else
        {
            printf("%s, ", rmFieldRegName);
            printf("%s", regFieldRegName);
        }
    }
    else if (mod < 3) // Between memory and register
    {
        char templateExpression[256];
        strcpy(templateExpression, registerNames[rmField].rmExpression);

        char displacementExpression[256];
        decodeDisplacement(mod, wBit, input, displacementExpression);
        char memoryExpression[256];

        if (mod == 0 && rmField == 6)
        {

            decodeDirectAddressing(input, memoryExpression);
        }
        else
        {
            sprintf(memoryExpression, templateExpression, displacementExpression);
        }

        if (dBit)
        {
            printf("%s, ", regFieldRegName);
            printf("%s", memoryExpression);
        }
        else
        {
            printf("%s, ", memoryExpression);
            printf("%s", regFieldRegName);
        }
    }
    else
    {
        assert(false && "Unknown mod field");
    }
}

char *getRegisterName(uint8_t registerIndex, bool wBit)
{

    assert(registerIndex < ARR_COUNT(registerNames) && "Register index out of range");

    RegisterName *registerName = registerNames + registerIndex;

    assert(registerIndex == registerName->registerIndex && "Wrong register index");

    if (registerName->registerIndex == registerIndex)
    {
        if (wBit)
        {
            return registerName->w1RegName;
        }
        else
        {
            return registerName->w0RegName;
        }
    }

    assert(false && "Invalid register");

    return 0;
}

uint8_t readUnsignedByte(FILE *input)
{
    uint8_t result;
    if (fread(&result, sizeof(result), 1, input) == 1)
    {
        return result;
    }
    else
    {
        assert(false && "Error while reading from file");
        return 0;
    }
}

int8_t readSignedByte(FILE *input)
{
    int8_t result;
    if (fread(&result, sizeof(result), 1, input) == 1)
    {
        return result;
    }
    else
    {
        assert(false && "Error while reading from file");
        return 0;
    }
}

void decodeJump(uint8_t secondByte)
{

    printf("%d", secondByte);
}

void decodeDirectAddressing(FILE *input, char *buffer)
{
    uint8_t byte = readUnsignedByte(input);
    int16_t constant = byte;

    extractHighBits(&constant, input);
    sprintf(buffer, "[%d]", constant);
}

void decodeImmediateToRegisterOrMemory(FILE *input, uint8_t mod, uint8_t firstByte, uint8_t secondByte, bool hasSignBit)
{
    bool sBit = extractDOrSBit(firstByte);
    bool wBit = extractWBit(firstByte);

    uint8_t rmField = extractRmField(secondByte);

    char displacementExpression[256] = {0};

    if (mod == 1 || mod == 2)
    {
        decodeDisplacement(mod, wBit, input, displacementExpression);
        char dest[256];
        uint8_t byte = readUnsignedByte(input);

        int16_t immediate = byte;
        if (mod == 2)
        {

            if (wBit && !(hasSignBit && sBit))
            {
                extractHighBits(&immediate, input);
            }
        }

        if (mod == 1 || mod == 2)
        {
            if (wBit)
            {
                printf("word ");
            }
            else
            {
                printf("byte ");
            }
        }

        sprintf(dest, registerNames[rmField].rmExpression, displacementExpression);
        printf("%s, ", dest);
        printf("%d", immediate);
    }

    else
    {
        char dest[256];
        if (mod == 0)
        {
            if (wBit)
            {
                printf("word ");
            }
            else
            {
                printf("byte ");
            }

            if (rmField == 6)
            { // direct addressing
                decodeDirectAddressing(input, dest);
            }
            else
            {
                sprintf(dest, registerNames[rmField].rmExpression, "");
            }
        }
        else
        {
            memcpy(dest, getRegisterName(rmField, wBit), 3);
        }

        uint8_t byte = readUnsignedByte(input);

        int16_t immediate = byte;

        bool isWord = false;
        if (sBit == 0 && wBit == 1)
        {
            isWord = true;
            extractHighBits(&immediate, input);
        }

        printf("%s, ", dest);

        printf("%u", immediate);
    }
}

int main(int argc, char *argv[])
{

    char *programName = argv[0];

    if (argc < 2)
    {
        printf("Usage: %s <input file>\n", programName);

        return 1;
    }

    char *inputName = argv[1];

    FILE *input = fopen(inputName, "r");

    if (input != NULL)
    {

        printf(";%s\n", inputName);
        printf("bits 16\n");

        uint8_t firstByte;

        while (fread(&firstByte, sizeof(firstByte), 1, input) == 1)
        {

            firstByte = firstByte & 0xff;

            uint8_t opcode = firstByte >> 2;

            if (opcode == 0x22)
            {
                uint8_t secondByte = readByte(input);
                printf("mov ");
                decodeRegisterMemoryToFromMemory(input, firstByte, secondByte);
            }
            else if (opcode == 0x00)
            {
                uint8_t secondByte = readByte(input);
                printf("add ");
                decodeRegisterMemoryToFromMemory(input, firstByte, secondByte);
            }
            else if (opcode == 0x0e)
            {
                uint8_t secondByte = readByte(input);
                printf("cmp ");
                decodeRegisterMemoryToFromMemory(input, firstByte, secondByte);
            }
            else if ((firstByte >> 1) == 0x1e)
            {
                uint8_t secondByte = readByte(input);
                printf("cmp ");
                decodeImmediateToAccumulator(input, firstByte, secondByte);
            }
            else if ((firstByte >> 1) == 0x2)
            {
                uint8_t secondByte = readByte(input);
                printf("add ");
                decodeImmediateToAccumulator(input, firstByte, secondByte);
            }
            else if ((firstByte >> 1) == 0x16)
            {
                uint8_t secondByte = readByte(input);
                printf("sub ");
                decodeImmediateToAccumulator(input, firstByte, secondByte);
            }
            else if ((firstByte >> 1) == 0x50)
            {
                uint8_t secondByte = readByte(input);
                printf("mov "); // Memory to accumulator

                printf("ax, ");
                uint8_t thirdByte = readUnsignedByte(input);

                int16_t address = secondByte | (thirdByte << 8);

                printf("[%d]", address);
            }
            else if ((firstByte >> 1) == 0x51)
            {
                uint8_t secondByte = readByte(input);
                printf("mov "); // Accumulator to memory
                uint8_t thirdByte = readUnsignedByte(input);
                int16_t address = secondByte | (thirdByte << 8);

                printf("[%d], ", address);
                printf("ax");
            }
            else if (opcode == 0x0a)
            {
                uint8_t secondByte = readByte(input);
                printf("sub ");
                decodeRegisterMemoryToFromMemory(input, firstByte, secondByte);
            }
            else if (opcode == 0x20)
            {
                uint8_t secondByte = readByte(input);
                uint8_t mod = extractMod(secondByte);
                uint8_t instructionExtension = (secondByte >> 3) & 0x07;
                switch (instructionExtension)
                {
                case 0:
                {
                    printf("add ");
                }
                break;
                case 5:
                {
                    printf("sub ");
                }
                break;
                case 7:
                {
                    printf("cmp ");
                }
                break;
                default:
                {
                    assert(false && "Unimplemented");
                }
                }
                decodeImmediateToRegisterOrMemory(input, mod, firstByte, secondByte, true);
            }
            else if (opcode == 0x31)
            {
                uint8_t secondByte = readByte(input);
                uint8_t mod = extractMod(secondByte);
                printf("mov ");
                decodeImmediateToRegisterOrMemory(input, mod, firstByte, secondByte, false);
            }
            else if (firstByte >> 4 == 0xb)
            {
                uint8_t secondByte = readByte(input);
                // Immediate-to-register mov
                printf("mov ");
                bool wBit = (firstByte & 0x08) >> 3;

                uint8_t regField = firstByte & 0x07;
                char *regFieldRegName = getRegisterName(regField, wBit);

                int16_t immediate = secondByte;

                if (wBit)
                {
                    extractHighBits(&immediate, input);
                }

                printf("%s, ", regFieldRegName);
                printf("%u", immediate);
            }
            else if ((firstByte >> 3) == 0xa)
            {
                printf("push ");

                uint8_t regField = firstByte & 0x07;
                char *regFieldRegName = getRegisterName(regField, 1);

                printf(regFieldRegName);
            }
            else if (firstByte == 0x70)
            {
                uint8_t secondByte = readByte(input);
                printf("jo ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x71)
            {
                uint8_t secondByte = readByte(input);

                printf("jno ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x72)
            {
                uint8_t secondByte = readByte(input);

                printf("jb ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x73)
            {
                uint8_t secondByte = readByte(input);

                printf("jnb ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x74)
            {
                uint8_t secondByte = readByte(input);

                printf("je ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x75)
            {
                uint8_t secondByte = readByte(input);

                printf("jne ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x76)
            {
                uint8_t secondByte = readByte(input);

                printf("jbe ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x77)
            {
                uint8_t secondByte = readByte(input);

                printf("ja ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x78)
            {
                uint8_t secondByte = readByte(input);

                printf("js ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x79)
            {
                uint8_t secondByte = readByte(input);

                printf("jns ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7a)
            {
                uint8_t secondByte = readByte(input);

                printf("jp ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7b)
            {
                uint8_t secondByte = readByte(input);

                printf("jnp ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7c)
            {
                uint8_t secondByte = readByte(input);

                printf("jl ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7d)
            {
                uint8_t secondByte = readByte(input);

                printf("jnl ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7e)
            {
                uint8_t secondByte = readByte(input);

                printf("jle ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7f)
            {
                uint8_t secondByte = readByte(input);

                printf("jg ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0xe0)
            {
                uint8_t secondByte = readByte(input);

                printf("loopnz ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0xe1)
            {
                uint8_t secondByte = readByte(input);

                printf("loopz ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0xe2)
            {
                uint8_t secondByte = readByte(input);

                printf("loop ");
                decodeJump(secondByte);
            }

            else if (firstByte == 0xe3)
            {
                uint8_t secondByte = readByte(input);

                printf("jcxz ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0xff)
            {
                uint8_t secondByte = readByte(input);

                printf("push ");
                uint8_t secondByteInstructionPattern = ((secondByte >> 3) & 0x07);
                assert(secondByteInstructionPattern == 0x06 && "Unimplemented instruction pattern");
                decodeRegisterMemory(input, firstByte, secondByte);
            }
            else
            {
                assert(false && "Unknown instruction");
            }

            printf("\n");
        }
    }
    else
    {
        perror("Error: Could not open input file");
    }

    printf("; Press enter to continue...");

    char byte;
    fread(&byte, sizeof(byte), 1, stdin);

    return 0;
}