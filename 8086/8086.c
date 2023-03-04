#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdint.h"
#include "assert.h"
#include "stdbool.h"

#define ARR_COUNT(a) (sizeof(a) / sizeof(*a))

typedef struct
{
    uint8_t registerIndex;
    char w0RegName[3];
    char w1RegName[3];
} RegisterName;

RegisterName registerNames[] = {
    {0, "al", "ax"},
    {1, "cl", "cx"},
    {2, "dl", "dx"},
    {3, "bl", "bx"},
    {4, "ah", "sp"},
    {5, "ch", "bp"},
    {6, "dh", "si"},
    {7, "bh", "di"},
};

char *getRegisterName(uint8_t registerIndex, bool wBit)
{

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

            uint8_t instructionType = firstByte >> 2;

            if (instructionType == 0x22)
            {
                printf("mov ");

                bool dBit = (firstByte & 0x02) >> 1;
                bool wBit = firstByte & 0x01;

                uint8_t mod = secondByte >> 6;

                assert(mod == 3 && "Unknown mod field");

                uint8_t regField = (secondByte & 0x3f) >> 3;

                uint8_t rmField = (secondByte & 0x07);

                char *regFieldRegName = getRegisterName(regField, wBit);

                char *rmFieldRegName = getRegisterName(rmField, wBit);

                if (dBit)
                {
                    printf("%s, ", regFieldRegName);
                    printf("%s\n", rmFieldRegName);
                }
                else
                {
                    printf("%s, ", rmFieldRegName);
                    printf("%s\n", regFieldRegName);
                }
            }
            else
            {
                assert(false && "Unknown instruction");
            }
        }
    }
    else
    {
        perror("ERROR: Could not open input file");
    }
    return 0;
}