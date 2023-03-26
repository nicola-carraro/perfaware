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

uint8_t readUnsignedByte(FILE *input)
{
    uint8_t result = 0;
    if (fread(&result, sizeof(result), 1, input) != 1)
    {
        assert(false && "Could not read");
    }

    return result;
}

void decodeRegisterMemoryToFromMemory(FILE *input, uint8_t secondByte, bool dBit, bool wBit)
{
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

void decodeJump(int8_t secondByte)
{

    printf("$%+d", secondByte + 2);
}

void decodeDirectAddressing(FILE *input, char *buffer)
{
    uint8_t byte = readUnsignedByte(input);
    int16_t constant = byte;

    extractHighBits(&constant, input);
    sprintf(buffer, "[%d]", constant);
}

void printAccumulator(bool wBit)
{
    if (wBit)
    {
        printf("ax, ");
    }
    else
    {
        printf("al, ");
    }
}

void decodeSegmentRegister(uint8_t regField)
{
    if (regField == 0x00)
    {
        printf("es");
    }
    else if (regField == 0x01)
    {
        printf("cs");
    }
    else if (regField == 0x02)
    {
        printf("ss");
    }
    else if (regField == 0x03)
    {
        printf("ds");
    }
    else
    {
        assert(false && "Invalid segment register");
    }
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

uint8_t extractLowBits(uint8_t byte, uint8_t bitsToExtract)
{
    assert(bitsToExtract <= 8);
    uint8_t mask = 0x00;

    switch (bitsToExtract)
    {
    case 0:
    {
        mask = 0x00;
    }
    break;
    case 1:
    {
        mask = 0x01;
    }
    break;
    case 2:
    {
        mask = 0x03;
    }
    break;
    case 3:
    {
        mask = 0x07;
    }
    break;
    case 4:
    {
        mask = 0x0f;
    }
    break;
    case 5:
    {
        mask = 0xf1;
    }
    break;
    case 6:
    {
        mask = 0xf3;
    }
    break;
    case 7:
    {
        mask = 0xf7;
    }
    break;
    case 8:
    {
        mask = 0xff;
    }
    break;
    default:
    {
        assert(false && "Unreachable");
    }
    }

    return byte & mask;
}

uint8_t extractBits(uint8_t byte, uint8_t firstBit, uint8_t onePastLastBit)
{
    assert(firstBit <= onePastLastBit);
    assert(firstBit <= 7);
    assert(onePastLastBit <= 8);

    uint8_t bitCount = onePastLastBit - firstBit;
    uint8_t shifted = (byte >> firstBit);
    uint8_t result = extractLowBits(shifted, bitCount);

    return result;
}

bool extractBit(uint8_t byte, uint8_t index)
{
    assert(index <= 7);
    uint8_t extracted = extractBits(byte, index, index + 1);
    assert(extracted <= 1);

    return extracted == 1;
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
            if (firstByte == 0xc5)
            {
                printf("lds ");
                uint8_t secondByte = readUnsignedByte(input);
                decodeRegisterMemoryToFromMemory(input, secondByte, true, true);
            }
            else if (firstByte == 0xc4)
            {
                printf("les ");
                uint8_t secondByte = readUnsignedByte(input);
                decodeRegisterMemoryToFromMemory(input, secondByte, true, true);
            }
            else if (firstByte == 0x37)
            {
                printf("aaa");
            }
            else if (firstByte == 0x27)
            {
                printf("daa");
            }
            else if (firstByte == 0x3f)
            {
                printf("aas");
            }
            else if (firstByte == 0x2f)
            {
                printf("das");
            }
            else if (firstByte == 0xc4)
            {
                printf("les ");
                uint8_t secondByte = readUnsignedByte(input);
                decodeRegisterMemoryToFromMemory(input, secondByte, true, true);
            }
            else if (firstByte == 0x9f)
            {
                printf("lahf ");
            }
            else if (firstByte == 0x9e)
            {
                printf("sahf ");
            }
            else if (firstByte == 0x9c)
            {
                printf("pushf ");
            }
            else if (firstByte == 0x9d)
            {
                printf("popf ");
            }
            else if (opcode == 0x22)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("mov ");
                bool dBit = extractDOrSBit(firstByte);
                bool wBit = extractWBit(firstByte);
                decodeRegisterMemoryToFromMemory(input, secondByte, dBit, wBit);
            }
            else if (opcode == 0x00)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("add ");
                bool dBit = extractDOrSBit(firstByte);
                bool wBit = extractWBit(firstByte);
                decodeRegisterMemoryToFromMemory(input, secondByte, dBit, wBit);
            }
            else if (opcode == 0x04)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("adc ");
                bool dBit = extractDOrSBit(firstByte);
                bool wBit = extractWBit(firstByte);
                decodeRegisterMemoryToFromMemory(input, secondByte, dBit, wBit);
            }
            else if (opcode == 0x0e)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("cmp ");
                bool dBit = extractDOrSBit(firstByte);
                bool wBit = extractWBit(firstByte);
                decodeRegisterMemoryToFromMemory(input, secondByte, dBit, wBit);
            }
            else if ((firstByte >> 1) == 0x1e)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("cmp ");
                decodeImmediateToAccumulator(input, firstByte, secondByte);
            }
            else if ((firstByte >> 1) == 0x2)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("add ");
                decodeImmediateToAccumulator(input, firstByte, secondByte);
            }
            else if ((firstByte >> 1) == 0x0a)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("adc ");
                decodeImmediateToAccumulator(input, firstByte, secondByte);
            }
            else if (firstByte >= 0x2c && firstByte <= 0x2d)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("sub ");
                decodeImmediateToAccumulator(input, firstByte, secondByte);
            }
            else if (firstByte >= 0x1c && firstByte <= 0x1d)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("sbb ");
                decodeImmediateToAccumulator(input, firstByte, secondByte);
            }
            else if ((firstByte >> 1) == 0x50)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("mov "); // Memory to accumulator

                printf("ax, ");
                uint8_t thirdByte = readUnsignedByte(input);

                int16_t address = secondByte | (thirdByte << 8);

                printf("[%d]", address);
            }
            else if ((firstByte >> 1) == 0x51)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("mov "); // Accumulator to memory
                uint8_t thirdByte = readUnsignedByte(input);
                int16_t address = secondByte | (thirdByte << 8);

                printf("[%d], ", address);
                printf("ax");
            }
            else if (firstByte >= 0x28 && firstByte <= 0x2b)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("sub ");
                bool dBit = extractDOrSBit(firstByte);
                bool wBit = extractWBit(firstByte);
                decodeRegisterMemoryToFromMemory(input, secondByte, dBit, wBit);
            }
            else if (firstByte >= 0x18 && firstByte <= 0x1b)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("sbb ");
                bool dBit = extractDOrSBit(firstByte);
                bool wBit = extractWBit(firstByte);
                decodeRegisterMemoryToFromMemory(input, secondByte, dBit, wBit);
            }
            else if (firstByte >= 0xfd && firstByte <= 0xff)
            {

                uint8_t secondByte = readUnsignedByte(input);
                uint8_t rem = extractBits(secondByte, 3, 6);
                if (firstByte == 0xff && rem == 0x06)
                {
                    printf("push ");
                }
                else if (rem == 0x00)
                {
                    printf("inc ");
                }
                else if (rem == 0x01)
                {
                    printf("dec ");
                }
                else
                {
                    assert(false && "Unimplemented");
                }
                decodeRegisterMemory(input, firstByte, secondByte);
            }
            else if (firstByte >= 0xf6 && firstByte <= 0xf7)
            {
                uint8_t secondByte = readUnsignedByte(input);
                uint8_t rem = extractBits(secondByte, 3, 6);

                if (rem == 0x03)
                {
                    printf("neg ");
                }
                else if (rem == 0x04)
                {
                    printf("mul ");
                }
                else if (rem == 0x05)
                {
                    printf("imul ");
                }
                else
                {
                    assert(false && "Unimplemented");
                }

                decodeRegisterMemory(input, firstByte, secondByte);
            }
            else if (firstByte >= 0x40 && firstByte <= 0x47)
            {
                printf("inc ");
                uint8_t rem = extractLowBits(firstByte, 3);
                char *regExpression = getRegisterName(rem, 1);
                printf(regExpression);
            }
            else if (firstByte >= 0x48 && firstByte <= 0x4f)
            {
                printf("dec ");
                uint8_t rem = extractLowBits(firstByte, 3);
                char *regExpression = getRegisterName(rem, 1);
                printf(regExpression);
            }
            else if (opcode == 0x20)
            {
                uint8_t secondByte = readUnsignedByte(input);
                uint8_t mod = extractMod(secondByte);
                uint8_t instructionExtension = (secondByte >> 3) & 0x07;
                switch (instructionExtension)
                {
                case 0:
                {
                    printf("add ");
                }
                break;
                case 2:
                {
                    printf("adc ");
                }
                break;
                case 3:
                {
                    printf("sbb ");
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
                uint8_t secondByte = readUnsignedByte(input);
                uint8_t mod = extractMod(secondByte);
                printf("mov ");
                decodeImmediateToRegisterOrMemory(input, mod, firstByte, secondByte, false);
            }
            else if (firstByte >> 4 == 0xb)
            {
                uint8_t secondByte = readUnsignedByte(input);
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
            else if (firstByte == 0x70)
            {
                uint8_t secondByte = readUnsignedByte(input);
                printf("jo ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x71)
            {
                uint8_t secondByte = readUnsignedByte(input);

                printf("jno ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x72)
            {
                uint8_t secondByte = readUnsignedByte(input);

                printf("jb ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x73)
            {
                uint8_t secondByte = readUnsignedByte(input);

                printf("jnb ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x74)
            {
                uint8_t secondByte = readUnsignedByte(input);

                printf("je ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x75)
            {
                int8_t secondByte = readSignedByte(input);

                printf("jnz ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x76)
            {
                int8_t secondByte = readSignedByte(input);

                printf("jbe ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x77)
            {
                int8_t secondByte = readSignedByte(input);

                printf("ja ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x78)
            {
                int8_t secondByte = readSignedByte(input);

                printf("js ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x79)
            {
                int8_t secondByte = readSignedByte(input);

                printf("jns ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7a)
            {
                int8_t secondByte = readUnsignedByte(input);

                printf("jp ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7b)
            {
                int8_t secondByte = readUnsignedByte(input);

                printf("jnp ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7c)
            {
                int8_t secondByte = readUnsignedByte(input);

                printf("jl ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7d)
            {
                int8_t secondByte = readUnsignedByte(input);

                printf("jnl ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7e)
            {
                int8_t secondByte = readUnsignedByte(input);

                printf("jle ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0x7f)
            {
                int8_t secondByte = readSignedByte(input);

                printf("jg ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0xe0)
            {
                int8_t secondByte = readSignedByte(input);

                printf("loopnz ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0xe1)
            {
                int8_t secondByte = readSignedByte(input);

                printf("loopz ");
                decodeJump(secondByte);
            }
            else if (firstByte == 0xe2)
            {
                int8_t secondByte = readSignedByte(input);

                printf("loop ");
                decodeJump(secondByte);
            }

            else if (firstByte == 0xe3)
            {
                int8_t secondByte = readSignedByte(input);

                printf("jcxz ");
                decodeJump(secondByte);
            }

            else if ((firstByte >> 3) == 0xa)
            {
                printf("push ");

                uint8_t regField = firstByte & 0x07;
                char *regFieldRegName = getRegisterName(regField, 1);

                printf(regFieldRegName);
            }
            else if (((firstByte >> 6) == 0) && ((firstByte & 0x07) == 0x06))
            {
                printf("push ");
                uint8_t regField = (firstByte >> 3) & 0x03;

                decodeSegmentRegister(regField);
            }

            else if (firstByte == 0x8f)
            {

                printf("pop ");

                uint8_t secondByte = readUnsignedByte(input);
                uint8_t secondByteInstructionPattern = ((secondByte >> 3) & 0x07);
                assert(secondByteInstructionPattern == 0x00 && "Unimplemented instruction pattern");
                decodeRegisterMemory(input, firstByte, secondByte);
            }

            else if ((firstByte >> 3) == 0x0b)
            {

                printf("pop ");

                uint8_t regField = firstByte & 0x07;
                char *regFieldRegName = getRegisterName(regField, 1);

                printf(regFieldRegName);
            }
            else if (((firstByte >> 6) == 0) && ((firstByte & 0x07) == 0x07))
            {
                printf("pop ");
                uint8_t regField = (firstByte >> 3) & 0x03;

                decodeSegmentRegister(regField);
            }
            else if ((firstByte >> 1) == 0x43)
            {
                printf("xchg ");
                uint8_t secondByte = readUnsignedByte(input);
                bool dBit = extractDOrSBit(firstByte);
                bool wBit = extractWBit(firstByte);
                decodeRegisterMemoryToFromMemory(input, secondByte, dBit, wBit);
            }
            else if ((firstByte >> 3) == 0x12)
            {
                // Register with accumulator
                printf("xchg ");
                printf("ax, ");
                uint8_t regField = firstByte & 0x07;
                char *regFieldRegName = getRegisterName(regField, 1);
                printf(regFieldRegName);
            }
            else if ((firstByte >> 1) == 0x72)
            {
                printf("in ");
                bool wBit = extractWBit(firstByte);
                uint8_t secondByte = readSignedByte(input);

                int16_t fixedPort = secondByte;
                bool sign = secondByte >> 7;

                if (wBit && sign)
                {
                    fixedPort = fixedPort | (0xff << 8);
                }

                printAccumulator(wBit);

                printf("%d", fixedPort);
            }
            else if ((firstByte >> 1) == 0x76)
            {
                printf("in ");

                bool wBit = extractWBit(firstByte);

                printAccumulator(wBit);

                printf("dx");
            }
            else if ((firstByte >> 1) == 0x73)
            {
                printf("out ");
                bool wBit = extractWBit(firstByte);
                uint8_t secondByte = readSignedByte(input);

                int16_t fixedPort = secondByte;
                bool sign = secondByte >> 7;

                if (wBit && sign)
                {
                    fixedPort = fixedPort | (0xff << 8);
                }

                printf("%d, ", fixedPort);
                if (wBit && sign)
                {
                    fixedPort = fixedPort | (0xff << 8);
                }

                if (wBit)
                {
                    printf("ax");
                }
                else
                {
                    printf("al");
                }
            }

            else if ((firstByte >> 1) == 0x77)
            {
                printf("out ");
                printf("dx, ");

                bool wBit = extractWBit(firstByte);

                if (wBit)
                {
                    printf("ax");
                }
                else
                {
                    printf("al");
                }
            }
            else if (firstByte == 0xd7)
            {
                printf("xlat");
            }
            else if (firstByte == 0x8d)
            {
                printf("lea ");
                uint8_t secondByte = readUnsignedByte(input);
                decodeRegisterMemoryToFromMemory(input, secondByte, true, true);
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