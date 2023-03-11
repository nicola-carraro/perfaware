#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdint.h"
#include "assert.h"
#include "stdbool.h"
#include "string.h"

char *getRegisterName(uint8_t registerIndex, bool wBit);

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
    uint8_t highBits;
    if (fread(&highBits, sizeof(highBits), 1, input) == 1)
    {
        *dest |= (((uint16_t)highBits) << 8);
    }
}

void decodeDisplacement(uint8_t mod, uint8_t wBit, FILE *input, char *displacementExpression)
{
    if (mod == 0)
    {
        displacementExpression[0] = 0;
        // if (rmField == 6)
        // { // direct addressing
        //     uint16_t constant = secondByte;
        //     uint8_t thirdByte;
        //     if (fread(&thirdByte, sizeof(thirdByte), 1, input) == 1)
        //     {
        //         constant |= (((uint16_t)thirdByte) << 8);
        //     }
        //     sprintf(templateExpression, "[%u]", constant);
        // }
    }
    else
    {
        uint8_t thirdByte;

        if (fread(&thirdByte, sizeof(thirdByte), 1, input))
        {
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

void registerMemoryToFromMemory(FILE *input, uint8_t firstByte, uint8_t secondByte)
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
        sprintf(memoryExpression, templateExpression, displacementExpression);

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

        uint16_t instruction;

        while (fread(&instruction, sizeof(instruction), 1, input) == 1)
        {

            uint8_t firstByte = instruction & 0xff;

            uint8_t secondByte = (instruction >> 8);

            uint8_t opcode = firstByte >> 2;

            uint8_t mod = extractMod(secondByte);

            if (opcode == 0x22)
            {
                printf("mov ");
                registerMemoryToFromMemory(input, firstByte, secondByte);
            }
            else if (opcode == 0x00)
            {
                printf("add ");
                registerMemoryToFromMemory(input, firstByte, secondByte);
            }
            else if ((firstByte >> 1) == 0x2)
            { // immediate to accumulator
                printf("add ");
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
            else if (opcode == 0x20)
            {

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
                bool sBit = extractDOrSBit(firstByte);
                bool wBit = extractWBit(firstByte);

                uint8_t rmField = extractRmField(secondByte);

                char displacementExpression[256] = {0};

                if (mod == 1 || mod == 2)
                {
                    decodeDisplacement(mod, wBit, input, displacementExpression);
                    char dest[256];
                    uint8_t byte;
                    if (fread(&byte, sizeof(byte), 1, input) == 1)
                    {
                        int16_t immediate = byte;
                        if (mod == 2)
                        {
                            if (sBit == 0 && wBit == 1)
                            {
                                extractHighBits(&immediate, input);
                            }

                            printf("word ");
                        }
                        else
                        {
                            printf("byte ");
                        }
                        sprintf(dest, registerNames[rmField].rmExpression, displacementExpression);
                        printf("%s, ", dest);
                        printf("%d", immediate);
                    }
                }

                else
                {
                    char dest[256];
                    if (mod == 0)
                    {
                        sprintf(dest, registerNames[rmField].rmExpression, "");
                    }
                    else
                    {
                        memcpy(dest, getRegisterName(rmField, wBit), 3);
                    }

                    uint8_t byte;
                    if (fread(&byte, sizeof(byte), 1, input) == 1)
                    {
                        int16_t immediate = byte;

                        bool isWord = false;
                        if (sBit == 0 && wBit == 1)
                        {
                            isWord = true;
                            extractHighBits(&immediate, input);
                        }

                        if (mod == 0)
                        {
                            if (isWord)
                            {
                                printf("word ");
                            }
                            else
                            {
                                printf("byte ");
                            }
                        }

                        printf("%s, ", dest);

                        printf("%u", immediate);
                    }
                }
            }
            else if (firstByte >> 4 == 0xb)
            {
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