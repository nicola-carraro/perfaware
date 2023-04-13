#include "8086.h"

void printPressEnterToContinue(bool isNoWait)
{
    if (isNoWait)
    {
        return;
    }
    printf("; Press enter to continue...");

    char byte;
    if (fread(&byte, 1, 1, stdin) != 1)
    {
        perror("Could not read from standard input");
    }
}

void printError(const char *file, const size_t line, const char *format, va_list args, bool isNoWait)
{
    char errorMessage[256];
    vsprintf(errorMessage, format, args);
    va_end(args);

    printf("ERROR(%s:%zu): \n", file, line);
    perror(errorMessage);
    printPressEnterToContinue(isNoWait);
}

void error(const char *file, const size_t line, bool isNoWait, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printError(file, line, format, args, isNoWait);

    exit(EXIT_FAILURE);
}

char *readFile(const char *filename, size_t *len, State *state)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        error(__FILE__, __LINE__, state->isNoWait, " Could not open % s ", filename);
    }

    char errorMessage[] = "Could not determine size of %s";
    if (fseek(file, 0, SEEK_END) < 0)
    {
        error(__FILE__, __LINE__, state->isNoWait, errorMessage, filename);
    }

    *len = ftell(file);

    if (fseek(file, 0, SEEK_SET) < 0)
    {
        error(__FILE__, __LINE__, state->isNoWait, errorMessage, filename);
    }

    char *bytes = malloc(*len);

    if (!bytes)
    {
        error(__FILE__, __LINE__, state->isNoWait, "Allocation failed", state->isNoWait);
    }

    if (fread(bytes, *len, 1, file) != 1)
    {
        error(__FILE__, __LINE__, state->isNoWait, " Could not read from % s ", filename);
    }

    if (file != NULL)
    {
        fclose(file);
    }

    return bytes;
}

int16_t consumeTwoBytesAsSigned(State *state)
{
    if (state->instructions.instructionPointer + 2 > state->instructions.size)
    {
        error(__FILE__, __LINE__, state->isNoWait, "reached end of instuctions stream");
    }

    int16_t result = *((int16_t *)(state->instructions.bytes + state->instructions.instructionPointer));

    state->instructions.instructionPointer += 2;

    return result;
}

uint8_t consumeByteAsUnsigned(State *state)
{
    if (state->instructions.instructionPointer >= state->instructions.size)
    {
        error(__FILE__, __LINE__, state->isNoWait, "reached end of instuctions stream");
    }

    uint8_t result = state->instructions.bytes[state->instructions.instructionPointer];

    state->instructions.instructionPointer++;

    return result;
}

int8_t consumeByteAsSigned(State *state)
{
    if (state->instructions.instructionPointer >= state->instructions.size)
    {
        error(__FILE__, __LINE__, state->isNoWait, "reached end of instuctions stream");
    }

    int8_t result = state->instructions.bytes[state->instructions.instructionPointer];

    state->instructions.instructionPointer++;

    return result;
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

void checkMod(const char *file, size_t line, uint8_t mod, State *state)
{
    if (mod > 3)
    {
        error(file, line, state->isNoWait, "Invalid mod %#X", mod);
    }
}

void checkRm(const char *file, size_t line, uint8_t rm, State *state)
{
    if (rm > 7)
    {
        error(file, line, state->isNoWait, "Invalid rm  %#X", rm);
    }
}

void checkReg(const char *file, size_t line, uint8_t reg, State *state)
{
    if (reg > 7)
    {
        error(file, line, state->isNoWait, "Invalid reg %#X", reg);
    }
}

Operand decodeRegOperand(bool wBit, uint8_t reg, State *state)
{
    checkReg(__FILE__, __LINE__, reg, state);

    Operand result = {0};
    result.type = operand_type_register;
    result.payload.reg = wBit ? RegFieldInfo[reg].w1Reg : RegFieldInfo[reg].w0Reg;

    return result;
}

Operand decodeRmOperand(bool wBit, uint8_t mod, uint8_t rm, State *state)
{
    checkMod(__FILE__, __LINE__, mod, state);
    checkRm(__FILE__, __LINE__, rm, state);
    assert(rm <= 7);

    Operand result = {0};

    bool immediateAddressing = (mod == 0 && rm == 6);
    if (immediateAddressing)
    {
        result.type = operand_type_memory;
        result.payload.memory.regCount = 0;
        result.payload.memory.displacement = consumeTwoBytesAsSigned(state);
    }
    else
    {
        if (mod == 3)
        {
            result.type = operand_type_register;
            result.payload.reg = wBit ? RmFieldInfo[rm].mod3w1Reg : RmFieldInfo[rm].mod3W0Reg;
        }
        else
        {
            result.type = operand_type_memory;
            result.payload.memory = RmFieldInfo[rm].memoryLocation;

            if (mod == 1)
            {
                int16_t displacement = consumeByteAsSigned(state);
                result.payload.memory.displacement = displacement;
            }
            else if (mod == 2)
            {
                result.payload.memory.displacement = consumeTwoBytesAsSigned(state);
            }
        }
    }

    return result;
}

void printRegister(RegisterLocation registerLocation)
{
    assert(RegisterInfos[registerLocation.reg].reg == registerLocation.reg);
    printf(RegisterInfos[registerLocation.reg].name);
    if (RegisterInfos[registerLocation.reg].isPartiallyAdressable)
    {
        assert(RegisterPortionInfos[registerLocation.portion].portion == registerLocation.portion);
        printf(RegisterPortionInfos[registerLocation.portion].name);
    }
}

void printOperand(Operand operand, uint16_t instructionByteCount, Register segmentRegister)
{

    printf(" ");
    switch (operand.type)
    {
    case operand_type_register:
    {

        printRegister(operand.payload.reg);
    }
    break;
    case operand_type_immediate:
    {
        if (operand.payload.immediate.isRelativeOffset)
        {
            printf("$%+d", operand.payload.immediate.value + instructionByteCount);
        }
        else if (operand.payload.immediate.isIntersegment)
        {
            printf("%u:%u", operand.payload.immediate.cs, operand.payload.immediate.ip);
        }
        else
        {
            printf("%d", operand.payload.immediate.value);
        }
    }
    break;
    case operand_type_memory:
    {
        if (segmentRegister != reg_none)
        {
            printf("%s:", RegisterInfos[segmentRegister].name);
        }
        printf("[");
        if (operand.payload.memory.regCount > 0)
        {
            printRegister(operand.payload.memory.reg0);
        }
        if (operand.payload.memory.regCount > 1)
        {
            printf(" + ");
            printRegister(operand.payload.memory.reg1);
        }

        if (operand.payload.memory.displacement > 0)
        {
            printf(" + ");
            printf("%d", operand.payload.memory.displacement);
        }
        else if (operand.payload.memory.displacement < 0)
        {
            printf(" - ");
            printf("%d", -operand.payload.memory.displacement);
        }
        else if (operand.payload.memory.regCount == 0)
        {
            printf("0");
        }

        printf("]");
    }
    break;
    default:
    {
        assert(false && "Unimplemented");
    }
    }
}

void printFlags(State *state)
{
    for (Flag flag = 0; flag < FLAG_COUNT; flag++)
    {
        if (state->flags[flag])
        {
            printf(FlagNames[flag].name);
        }
    }
}

void printInstruction(Instruction instruction, State *before, State *after)
{
    assert(instruction.type != instruction_none);
    assert(InstructionInfos[instruction.type].type == instruction.type);

    const char *mnemonic = InstructionInfos[instruction.type].name;
    printf(mnemonic);

    if (instruction.isInt3)
    {
        assert(instruction.type == instruction_int);
        printf("3");
    }

    if (InstructionInfos[instruction.type].needsWithSuffix)
    {
        printf("%s", instruction.isWide ? "w" : "b");
    }

    else
    {
        if (instruction.needsDecorator)
        {
            if (instruction.isWide)
            {
                printf(" word");
            }
            else
            {
                printf(" byte");
            }
        }
    }

    if (instruction.operandCount > 0)
    {
        printOperand(instruction.firstOperand, instruction.byteCount, instruction.segmentRegister);
    }

    if (instruction.operandCount > 1)
    {
        printf(",");
        printOperand(instruction.secondOperand, instruction.byteCount, instruction.segmentRegister);
    }

    printf("\t");
    if (after->execute)
    {
        for (Register reg = 0; reg < REG_COUNT; reg++)
        {
            if (before->registers[reg].x != after->registers[reg].x)
            {
                RegisterLocation location;
                location.reg = reg;
                location.portion = reg_portion_x;
                printRegister(location);
                printf("   %#x--->%#x", before->registers[reg].x, after->registers[reg].x);
            }
        }

        bool isFlagChange = false;
        for (Flag flag = 0; flag < FLAG_COUNT; flag++)
        {
            if (before->flags[flag] != after->flags[flag])
            {
                isFlagChange = true;
                break;
            }
        }
        if (isFlagChange)
        {
            printf(" Flags:");
            printFlags(before);
            printf("->");
            printFlags(after);
        }
        printf(" ip:%#x--->%#x", before->instructions.instructionPointer, after->instructions.instructionPointer);
    }

    if (instruction.type == instruction_rep || instruction.type == instruction_lock)
    {
        printf(" ");
    }
    else
    {
        printf("\n");
    }
}

Instruction decodeRegMemToFromRegMem(bool dBit, bool wBit, State *state)
{
    uint8_t secondByte = consumeByteAsUnsigned(state);
    uint8_t mod = extractBits(secondByte, 6, 8);
    uint8_t reg = extractBits(secondByte, 3, 6);
    uint8_t rm = extractLowBits(secondByte, 3);

    Instruction result = {0};

    result.operandCount = 2;
    Operand rmOperand = decodeRmOperand(wBit, mod, rm, state);
    Operand regOperand = decodeRegOperand(wBit, reg, state);
    result.isWide = wBit;

    if (dBit)
    {
        result.firstOperand = regOperand;
        result.secondOperand = rmOperand;
    }
    else
    {
        result.firstOperand = rmOperand;
        result.secondOperand = regOperand;
    }

    return result;
}

Operand decodeImmediateOperand(bool wBit, bool sBit, State *state)
{

    Operand result = {0};

    result.type = operand_type_immediate;
    result.payload.immediate.isRelativeOffset = false;
    if (wBit && !sBit)
    {

        result.payload.immediate.value = consumeTwoBytesAsSigned(state);
    }
    else
    {
        result.payload.immediate.value = consumeByteAsSigned(state);
    }
    return result;
}

Instruction decodeImmediateToRegister(bool wBit, uint8_t reg, State *state)
{
    Instruction result = {0};

    result.firstOperand = decodeRegOperand(wBit, reg, state);
    result.secondOperand = decodeImmediateOperand(wBit, false, state);
    result.operandCount = 2;
    result.isWide = wBit;

    return result;
}

Instruction decodeImmediateToRegisterMemory(uint8_t firstByte, bool sBit, uint8_t secondByte, State *state)
{
    bool wBit = extractBit(firstByte, 0);
    uint8_t mod = extractBits(secondByte, 6, 8);
    uint8_t rm = extractLowBits(secondByte, 3);

    Instruction result = {0};

    result.firstOperand = decodeRmOperand(wBit, mod, rm, state);
    result.secondOperand = decodeImmediateOperand(wBit, sBit, state);
    result.operandCount = 2;
    result.isWide = wBit;

    return result;
}

Instruction decodeImmediateFromAccumulator(bool wBit, State *state)
{
    Instruction result = {0};

    result.firstOperand.type = operand_type_register;
    result.firstOperand.payload.reg.reg = reg_a;
    result.firstOperand.payload.reg.portion = wBit ? reg_portion_x : reg_portion_l;

    result.secondOperand = decodeImmediateOperand(wBit, false, state);
    result.operandCount = 2;
    result.isWide = wBit;

    return result;
}

Instruction decodeMemoryToAccumulator(uint8_t firstByte, State *state)
{
    Instruction instruction = {0};
    instruction.operandCount = 2;
    bool wBit = extractBit(firstByte, 0);

    instruction.firstOperand.type = operand_type_register;
    instruction.firstOperand.payload.reg.reg = reg_a;
    if (wBit)
    {
        instruction.firstOperand.payload.reg.portion = reg_portion_x;
    }
    else
    {
        instruction.firstOperand.payload.reg.portion = reg_portion_l;
    }

    instruction.secondOperand.type = operand_type_memory;
    instruction.secondOperand.payload.memory.regCount = 0;
    instruction.secondOperand.payload.memory.displacement = consumeTwoBytesAsSigned(state);

    instruction.isWide = wBit;

    return instruction;
}

Instruction decodeAccumulatorToMemory(uint8_t firstByte, State *state)
{
    Instruction instruction = {0};
    instruction.operandCount = 2;
    bool wBit = extractBit(firstByte, 0);

    instruction.firstOperand.type = operand_type_memory;
    instruction.firstOperand.payload.memory.regCount = 0;
    instruction.firstOperand.payload.memory.displacement = consumeTwoBytesAsSigned(state);

    instruction.secondOperand.type = operand_type_register;
    instruction.secondOperand.payload.reg.reg = reg_a;
    if (wBit)
    {
        instruction.secondOperand.payload.reg.portion = reg_portion_x;
    }
    else
    {
        instruction.secondOperand.payload.reg.portion = reg_portion_l;
    }

    instruction.isWide = wBit;

    return instruction;
}

Instruction decodeRegisterMemory(uint8_t secondByte, bool isWide, State *state)
{
    Instruction result = {0};

    uint8_t rm = extractLowBits(secondByte, 3);
    uint8_t mod = extractBits(secondByte, 6, 8);
    result.operandCount = 1;
    result.firstOperand = decodeRmOperand(isWide, mod, rm, state);
    result.isWide = isWide;

    return result;
}

Instruction decodeRegister(uint8_t firstByte, bool wBit, State *state)
{
    Instruction result = {0};
    result.operandCount = 1;
    uint8_t reg = extractLowBits(firstByte, 3);
    result.firstOperand = decodeRegOperand(wBit, reg, state);
    result.isWide = true;
    return result;
}

Register decodeSrField(uint8_t reg)
{
    switch (reg)
    {
    case 0x0:
    {
        return reg_es;
    }
    break;
    case 0x1:
    {
        return reg_cs;
    }
    break;
    case 0x2:
    {
        return reg_ss;
    }
    break;
    case 0x3:
    {
        return reg_ds;
    }
    break;
    default:
    {
        assert(false);
        return reg_none;
    }
    }
}

Instruction decodeSegmentRegister(uint8_t firstByte)
{
    Instruction result = {0};
    result.operandCount = 1;
    result.firstOperand.type = operand_type_register;
    result.firstOperand.payload.reg.portion = reg_portion_x;
    uint8_t sr = extractBits(firstByte, 3, 5);
    result.firstOperand.payload.reg.reg = decodeSrField(sr);
    result.isWide = true;
    return result;
}

Instruction decodeRegisterWithAccumulator(uint8_t firstByte, State *state)
{
    Instruction result = {0};
    result.operandCount = 2;
    result.isWide = true;

    uint8_t reg = extractLowBits(firstByte, 3);
    result.firstOperand = decodeRegOperand(true, reg, state);

    result.secondOperand.type = operand_type_register;
    result.secondOperand.payload.reg.reg = reg_a;
    result.secondOperand.payload.reg.portion = reg_portion_x;

    return result;
}

Instruction
decodeJump(State *state)
{
    Instruction result = {0};
    result.operandCount = 1;
    result.firstOperand.type = operand_type_immediate;
    result.firstOperand.payload.immediate.value = consumeByteAsSigned(state);
    result.firstOperand.payload.immediate.isRelativeOffset = true;
    return result;
}

Instruction decodeIntersegmentImmediate(State *state)
{
    Instruction instruction = {0};

    instruction.operandCount = 1;
    instruction.firstOperand.type = operand_type_immediate;
    instruction.firstOperand.payload.immediate.isIntersegment = true;
    instruction.firstOperand.payload.immediate.ip = consumeTwoBytesAsSigned(state);
    instruction.firstOperand.payload.immediate.cs = consumeTwoBytesAsSigned(state);
    instruction.isWide = true;

    return instruction;
}

Instruction decodeFixedPort(bool isDest, uint8_t firstByte, State *state)
{
    Instruction result = {0};
    result.operandCount = 2;
    bool wBit = extractBit(firstByte, 0);
    result.isWide = wBit;

    Operand port = {0};
    port.type = operand_type_immediate;
    port.payload.immediate.value = consumeByteAsUnsigned(state);

    Operand other = {0};

    other.type = operand_type_register;
    other.payload.reg.reg = reg_a;
    other.payload.reg.portion = wBit ? reg_portion_x : reg_portion_l;

    if (isDest)
    {
        result.firstOperand = port;
        result.secondOperand = other;
    }
    else
    {
        result.firstOperand = other;
        result.secondOperand = port;
    }

    return result;
}

Instruction decodeVariablePort(bool isDest, uint8_t firstByte)
{
    Instruction result = {0};
    bool wBit = extractBit(firstByte, 0);
    result.isWide = wBit;
    result.operandCount = 2;

    Operand port = {0};
    port.type = operand_type_register;
    port.payload.reg.reg = reg_d;
    port.payload.reg.portion = reg_portion_x;

    Operand other = {0};
    other.type = operand_type_register;
    other.payload.reg.reg = reg_a;
    other.payload.reg.portion = wBit ? reg_portion_x : reg_portion_l;

    if (isDest)
    {
        result.firstOperand = port;
        result.secondOperand = other;
    }
    else
    {
        result.firstOperand = other;
        result.secondOperand = port;
    }

    return result;
}

Instruction decodeInstruction(State *state)
{

    uint16_t initialStackPointer = state->instructions.instructionPointer;

    uint8_t firstByte = consumeByteAsUnsigned(state);

    Instruction instruction = {0};

    Register segmentRegister = reg_none;

    if (firstByte >= 0x88 && firstByte <= 0x8b)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);

        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);
        instruction.type = instruction_mov;
    }
    if (firstByte == 0xc6 || firstByte == 0xc7)
    {
        assert(instruction.type == instruction_none);
        uint8_t secondByte = consumeByteAsUnsigned(state);

        instruction = decodeImmediateToRegisterMemory(firstByte, false, secondByte, state);

        uint8_t reg = extractBits(secondByte, 3, 6);
        switch (reg)
        {
        case 0x00:
        {
            instruction.type = instruction_mov;
        }
        break;
        default:
        {
            error(__FILE__, __LINE__, state->isNoWait, "Unknown instruction, first byte=%#X, reg=%#X", firstByte, reg);
        }
        }
    }
    if (firstByte >= 0xb0 && firstByte <= 0xbf)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 3);
        uint8_t reg = extractLowBits(firstByte, 3);

        instruction = decodeImmediateToRegister(wBit, reg, state);
        instruction.type = instruction_mov;
    }
    if (firstByte == 0xa0 || firstByte == 0xa1)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeMemoryToAccumulator(firstByte, state);
        instruction.type = instruction_mov;
    }
    if (firstByte == 0xa2 || firstByte == 0xa3)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeAccumulatorToMemory(firstByte, state);

        instruction.type = instruction_mov;
    }
    if (firstByte == 0x8c)
    {
        assert(instruction.type == instruction_none);
        uint8_t secondByte = consumeByteAsUnsigned(state);
        uint8_t mod = extractBits(secondByte, 6, 8);
        if (extractBit(secondByte, 5) != 0)
        {
            error(__FILE__, __LINE__, state->isNoWait, "Expected a 0 bit in the 5th bit of bite 2 for mov from segmente register");
        }
        uint8_t sr = extractBits(secondByte, 3, 5);
        uint8_t rm = extractLowBits(secondByte, 3);

        instruction.operandCount = 2;
        instruction.firstOperand = decodeRmOperand(true, mod, rm, state);
        instruction.secondOperand.type = operand_type_register;
        instruction.secondOperand.payload.reg.reg = decodeSrField(sr);
        instruction.secondOperand.payload.reg.portion = reg_portion_x;
        instruction.type = instruction_mov;
    }
    if (firstByte == 0xfe || firstByte == 0xff)
    {
        assert(instruction.type == instruction_none);
        uint8_t secondByte = consumeByteAsUnsigned(state);
        uint8_t reg = extractBits(secondByte, 3, 6);

        if (firstByte == 0xff && reg == 0x6)
        {

            instruction = decodeRegisterMemory(secondByte, true, state);

            instruction.type = instruction_push;
        }

        if (reg == 0x0)
        {
            bool wBit = extractBit(firstByte, 0);
            instruction = decodeRegisterMemory(secondByte, wBit, state);

            instruction.type = instruction_inc;
        }
        else if (reg == 0x1)
        {
            bool wBit = extractBit(firstByte, 0);
            instruction = decodeRegisterMemory(secondByte, wBit, state);

            instruction.type = instruction_dec;
        }
        else if (reg == 0x2)
        {
            bool wBit = extractBit(firstByte, 0);
            instruction = decodeRegisterMemory(secondByte, wBit, state);

            instruction.type = instruction_call;
        }
        else if (reg == 0x4)
        {
            bool wBit = extractBit(firstByte, 0);
            instruction = decodeRegisterMemory(secondByte, wBit, state);

            instruction.type = instruction_jmp;
        }
    }
    if (firstByte == 0x3c || firstByte == 0x3d)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        instruction = decodeImmediateFromAccumulator(wBit, state);

        instruction.type = instruction_cmp;
    }
    if (firstByte >= 0x38 && firstByte <= 0x3b)
    {
        assert(instruction.type == instruction_none);

        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);
        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);
        instruction.type = instruction_cmp;
    }
    if (firstByte >= 0x50 && firstByte <= 0x57)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeRegister(firstByte, true, state);
        instruction.type = instruction_push;
    }
    if (extractBits(firstByte, 5, 8) == 0x0 && extractLowBits(firstByte, 3) == 0x06)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeSegmentRegister(firstByte);
        instruction.type = instruction_push;
    }
    if (firstByte >= 0x58 && firstByte <= 0x5f)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeRegister(firstByte, true, state);
        instruction.type = instruction_pop;
    }
    if (extractBits(firstByte, 5, 8) == 0x0 && extractLowBits(firstByte, 3) == 0x7)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeSegmentRegister(firstByte);
        instruction.type = instruction_pop;
    }
    if (firstByte == 0x8f)
    {
        assert(instruction.type == instruction_none);
        uint8_t secondByte = consumeByteAsUnsigned(state);
        instruction = decodeRegisterMemory(secondByte, true, state);
        uint8_t reg = extractBits(secondByte, 3, 6);

        switch (reg)
        {
        case 0x00:
        {
            instruction.type = instruction_pop;
        }
        break;
        default:
        {
            error(__FILE__, __LINE__, state->isNoWait, "Unknown instruction, first byte=%#X, reg=%#X", firstByte, reg);
        }
        }
    }
    if (firstByte == 0x86 || firstByte == 0x87)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);
        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);
        instruction.type = instruction_xchg;
    }
    if (firstByte >= 0x90 && firstByte <= 0x97)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeRegisterWithAccumulator(firstByte, state);
        instruction.type = instruction_xchg;
    }
    if (firstByte == 0xe4 || firstByte == 0xe5)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeFixedPort(false, firstByte, state);
        instruction.type = instruction_in;
    }
    if (firstByte == 0xec || firstByte == 0xed)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeVariablePort(false, firstByte);
        instruction.type = instruction_in;
    }
    if (firstByte == 0xe6 || firstByte == 0xe7)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeFixedPort(true, firstByte, state);
        instruction.type = instruction_out;
    }
    if (firstByte == 0xee || firstByte == 0xef)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeVariablePort(true, firstByte);
        instruction.type = instruction_out;
    }
    if (firstByte == 0xd7)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_xlat;
    }
    if (firstByte == 0x8d)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeRegMemToFromRegMem(true, true, state);
        instruction.type = instruction_lea;
    }
    if (firstByte == 0xc5)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeRegMemToFromRegMem(true, true, state);
        instruction.type = instruction_lds;
    }
    if (firstByte == 0xc4)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeRegMemToFromRegMem(true, true, state);
        instruction.type = instruction_les;
    }
    if (firstByte == 0x9f)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_lahf;
    }
    if (firstByte == 0x9e)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_sahf;
    }
    if (firstByte == 0x9c)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_pushf;
    }
    if (firstByte == 0x9d)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_popf;
    }
    if (firstByte >= 0x00 && firstByte <= 0x03)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);
        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);

        instruction.type = instruction_add;
    }
    if (firstByte >= 0x80 && firstByte <= 0x83)
    {
        assert(instruction.type == instruction_none);
        uint8_t secondByte = consumeByteAsUnsigned(state);
        bool sBit = extractBit(firstByte, 1);

        instruction = decodeImmediateToRegisterMemory(firstByte, sBit, secondByte, state);

        uint8_t reg = extractBits(secondByte, 3, 6);
        switch (reg)
        {
        case 0x00:
        {
            instruction.type = instruction_add;
        }
        break;
        case 0x01:
        {
            instruction.type = instruction_or;
        }
        break;
        case 0x02:
        {
            instruction.type = instruction_adc;
        }
        break;
        case 0x03:
        {
            instruction.type = instruction_sbb;
        }
        break;
        case 0x04:
        {
            instruction.type = instruction_and;
        }
        break;
        case 0x05:
        {
            instruction.type = instruction_sub;
        }
        break;
        case 0x06:
        {
            instruction.type = instruction_xor;
        }
        break;
        case 0x07:
        {
            instruction.type = instruction_cmp;
        }
        break;
        default:
        {
            error(__FILE__, __LINE__, state->isNoWait, "Unknown instruction, first byte=%#X, reg=%#X", firstByte, reg);
        }
        }
    }
    if (firstByte == 0x04 || firstByte == 0x05)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction = decodeImmediateFromAccumulator(wBit, state);

        instruction.type = instruction_add;
    }
    if (firstByte >= 0x10 && firstByte <= 0x13)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);
        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);

        instruction.type = instruction_adc;
    }
    if (firstByte == 0x14 || firstByte == 0x15)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction = decodeImmediateFromAccumulator(wBit, state);

        instruction.type = instruction_adc;
    }
    if (firstByte >= 0x40 && firstByte <= 0x47)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeRegister(firstByte, true, state);
        instruction.type = instruction_inc;
    }
    if (firstByte == 0x37)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_aaa;
    }
    if (firstByte == 0x27)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_daa;
    }
    if (firstByte >= 0x28 && firstByte <= 0x2b)
    {
        assert(instruction.type == instruction_none);

        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);
        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);
        instruction.type = instruction_sub;
    }
    if (firstByte == 0x2c || firstByte == 0x2d)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        instruction = decodeImmediateFromAccumulator(wBit, state);

        instruction.type = instruction_sub;
    }

    if (firstByte >= 0x18 && firstByte <= 0x1b)
    {
        assert(instruction.type == instruction_none);

        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);
        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);
        instruction.type = instruction_sbb;
    }
    if (firstByte == 0x1c || firstByte == 0x1d)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        instruction = decodeImmediateFromAccumulator(wBit, state);

        instruction.type = instruction_sbb;
    }
    if (firstByte >= 0x48 && firstByte <= 0x4f)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeRegister(firstByte, true, state);
        instruction.type = instruction_dec;
    }
    if (firstByte == 0xf6 || firstByte == 0xf7)
    {
        assert(instruction.type == instruction_none);
        uint8_t secondByte = consumeByteAsUnsigned(state);
        uint8_t reg = extractBits(secondByte, 3, 6);
        bool wBit = extractBit(firstByte, 0);

        if (reg != 0)
        {
            instruction = decodeRegisterMemory(secondByte, wBit, state);
        }
        else
        {
            instruction = decodeImmediateToRegisterMemory(firstByte, 0, secondByte, state);
        }

        switch (reg)
        {
        case 0x0:
        {
            instruction.type = instruction_test;
        }
        break;
        case 0x2:
        {

            instruction.type = instruction_not;
        }
        break;
        case 0x3:
        {
            instruction.type = instruction_neg;
        }
        break;
        case 0x4:
        {
            instruction.type = instruction_mul;
        }
        break;
        case 0x5:
        {
            instruction.type = instruction_imul;
        }
        break;
        case 0x6:
        {
            instruction.type = instruction_div;
        }
        break;
        case 0x7:
        {
            instruction.type = instruction_idiv;
        }
        break;
        }
    }
    if (firstByte == 0x3f)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_aas;
    }
    if (firstByte == 0x2f)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_das;
    }
    if (firstByte == 0xd4)
    {
        assert(instruction.type == instruction_none);
        uint8_t secondByte = consumeByteAsUnsigned(state);
        if (secondByte == 0x0a)
        {
            instruction.type = instruction_aam;
        }
    }
    if (firstByte == 0xd5)
    {
        assert(instruction.type == instruction_none);
        uint8_t secondByte = consumeByteAsUnsigned(state);
        if (secondByte == 0x0a)
        {
            instruction.type = instruction_aad;
        }
    }
    if (firstByte == 0x98)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_cbw;
    }
    if (firstByte == 0x99)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_cwd;
    }

    if (firstByte >= 0xd0 && firstByte <= 0xd3)
    {
        assert(instruction.type == instruction_none);
        uint8_t secondByte = consumeByteAsUnsigned(state);
        uint8_t mod = extractBits(secondByte, 6, 8);
        uint8_t reg = extractBits(secondByte, 3, 6);
        uint8_t rm = extractLowBits(secondByte, 3);

        bool wBit = extractBit(firstByte, 0);
        bool vBit = extractBit(firstByte, 1);
        instruction.isWide = wBit;
        instruction.operandCount = 2;
        if (vBit)
        {
            instruction.secondOperand.type = operand_type_register;
            instruction.secondOperand.payload.reg.reg = reg_c;
            instruction.secondOperand.payload.reg.portion = reg_portion_l;
        }
        else
        {
            instruction.secondOperand.type = operand_type_immediate;
            instruction.secondOperand.payload.immediate.value = 1;
        }

        instruction.firstOperand = decodeRmOperand(wBit, mod, rm, state);

        if (instruction.firstOperand.type == operand_type_memory)
        {
            instruction.needsDecorator = true;
        }

        switch (reg)
        {
        case 0x0:
        {
            instruction.type = instruction_rol;
        }
        break;
        case 0x1:
        {
            instruction.type = instruction_ror;
        }
        break;
        case 0x2:
        {
            instruction.type = instruction_rcl;
        }
        break;
        case 0x3:
        {
            instruction.type = instruction_rcr;
        }
        break;
        case 0x4:
        {
            instruction.type = instruction_shl;
        }
        break;
        case 0x5:
        {
            instruction.type = instruction_shr;
        }
        break;
        case 0x7:
        {
            instruction.type = instruction_sar;
        }
        break;
        }
    }
    if (firstByte >= 0x20 && firstByte <= 0x23)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);
        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);

        instruction.type = instruction_and;
    }
    if (firstByte == 0x24 || firstByte == 0x25)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction = decodeImmediateFromAccumulator(wBit, state);

        instruction.type = instruction_and;
    }
    if (firstByte == 0x84 || firstByte == 0x85)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);
        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);

        instruction.type = instruction_test;
    }
    if (firstByte == 0xa8 || firstByte == 0xa9)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction = decodeImmediateFromAccumulator(wBit, state);

        instruction.type = instruction_test;
    }

    if (firstByte >= 0x08 && firstByte <= 0x0b)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);
        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);

        instruction.type = instruction_or;
    }
    if (firstByte == 0x0c || firstByte == 0x0d)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction = decodeImmediateFromAccumulator(wBit, state);

        instruction.type = instruction_or;
    }

    if (firstByte >= 0x30 && firstByte <= 0x33)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractBit(firstByte, 0);
        bool dBit = extractBit(firstByte, 1);
        instruction = decodeRegMemToFromRegMem(dBit, wBit, state);

        instruction.type = instruction_xor;
    }
    if (firstByte == 0x34 || firstByte == 0x35)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction = decodeImmediateFromAccumulator(wBit, state);

        instruction.type = instruction_xor;
    }
    if (firstByte == 0xf2 || firstByte == 0xf3)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_rep;
    }
    if (firstByte == 0xa4 || firstByte == 0xa5)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction.isWide = wBit;
        instruction.type = instruction_movs;
    }
    if (firstByte == 0xa6 || firstByte == 0xa7)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction.isWide = wBit;
        instruction.type = instruction_cmps;
    }
    if (firstByte == 0xae || firstByte == 0xaf)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction.isWide = wBit;
        instruction.type = instruction_scas;
    }
    if (firstByte == 0xac || firstByte == 0xad)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction.isWide = wBit;
        instruction.type = instruction_lods;
    }
    if (firstByte == 0xaa || firstByte == 0xab)
    {
        assert(instruction.type == instruction_none);
        bool wBit = extractLowBits(firstByte, 1);
        instruction.isWide = wBit;
        instruction.type = instruction_stos;
    }
    if (firstByte == 0xe8)
    {
        assert(instruction.type == instruction_none);
        instruction.operandCount = 1;
        instruction.firstOperand.type = operand_type_memory;
        instruction.firstOperand.payload.memory.displacement = consumeTwoBytesAsSigned(state);

        instruction.type = instruction_call;
    }
    if (firstByte == 0x9a)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeIntersegmentImmediate(state);
        instruction.type = instruction_call;
    }
    if (firstByte == 0xea)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeIntersegmentImmediate(state);
        instruction.type = instruction_jmp;
    }
    if (firstByte == 0xe9)
    {
        assert(instruction.type == instruction_none);
        instruction.operandCount = 1;
        instruction.isWide = true;
        instruction.firstOperand.type = operand_type_immediate;
        instruction.firstOperand.payload.immediate.value = consumeTwoBytesAsSigned(state);
        instruction.firstOperand.payload.immediate.isRelativeOffset = true;
        instruction.type = instruction_jmp;
    }
    if (firstByte == 0xc3)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_ret;
    }
    if (firstByte == 0xc2)
    {
        assert(instruction.type == instruction_none);
        instruction.operandCount = 1;
        instruction.firstOperand = decodeImmediateOperand(true, false, state);
        instruction.isWide = true;
        instruction.type = instruction_ret;
    }
    if (firstByte == 0x74)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_je;
    }
    if (firstByte == 0x7c)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jl;
    }
    if (firstByte == 0x7e)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jle;
    }
    if (firstByte == 0x72)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jb;
    }
    if (firstByte == 0x76)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jbe;
    }
    if (firstByte == 0x7a)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jp;
    }
    if (firstByte == 0x70)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeJump(state);
        instruction.type = instruction_jo;
    }
    if (firstByte == 0x78)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_js;
    }
    if (firstByte == 0x75)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jnz;
    }
    if (firstByte == 0x7d)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jnl;
    }
    if (firstByte == 0x7f)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jnle;
    }
    if (firstByte == 0x73)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jnb;
    }
    if (firstByte == 0x77)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_ja;
    }
    if (firstByte == 0x7b)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jnp;
    }
    if (firstByte == 0x71)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jno;
    }
    if (firstByte == 0x79)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jns;
    }
    if (firstByte == 0xe2)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_loop;
    }
    if (firstByte == 0xe1)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_loopz;
    }
    if (firstByte == 0xe0)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_loopnz;
    }
    if (firstByte == 0xe3)
    {
        assert(instruction.type == instruction_none);

        instruction = decodeJump(state);
        instruction.type = instruction_jcxz;
    }
    if (firstByte == 0xcd)
    {
        assert(instruction.type == instruction_none);
        instruction.operandCount = 1;
        instruction.firstOperand.type = operand_type_immediate;
        instruction.firstOperand.payload.immediate.value = consumeByteAsUnsigned(state);
        instruction.type = instruction_int;
    }
    if (firstByte == 0xcc)
    {
        assert(instruction.type == instruction_none);
        instruction.isInt3 = true;
        instruction.type = instruction_int;
    }
    if (firstByte == 0xce)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_into;
    }
    if (firstByte == 0xcf)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_iret;
    }
    if (firstByte == 0xf8)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_clc;
    }
    if (firstByte == 0xf5)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_cmc;
    }
    if (firstByte == 0xf9)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_stc;
    }
    if (firstByte == 0xfc)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_cld;
    }
    if (firstByte == 0xfd)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_std;
    }
    if (firstByte == 0xfa)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_cli;
    }
    if (firstByte == 0xfb)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_sti;
    }
    if (firstByte == 0xf4)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_hlt;
    }
    if (firstByte == 0x9b)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_wait;
    }
    if (firstByte == 0xf0)
    {
        assert(instruction.type == instruction_none);
        instruction.type = instruction_lock;
    }
    if (firstByte == 0x2e)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeInstruction(state);
        segmentRegister = reg_cs;
    }
    if (firstByte == 0x3e)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeInstruction(state);
        segmentRegister = reg_ds;
    }
    if (firstByte == 0x26)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeInstruction(state);
        segmentRegister = reg_es;
    }
    if (firstByte == 0x36)
    {
        assert(instruction.type == instruction_none);
        instruction = decodeInstruction(state);
        segmentRegister = reg_ss;
    }
    if (instruction.type == instruction_none)
    {
        error(__FILE__, __LINE__, state->isNoWait, "Unknown instruction, first byte=%#X", firstByte);
    }

    if (
        instruction.operandCount == 1 && instruction.firstOperand.type != operand_type_register && !(instruction.firstOperand.type == operand_type_immediate && instruction.firstOperand.payload.immediate.isRelativeOffset))
    {
        instruction.needsDecorator = true;
    }
    if (instruction.operandCount == 2 && instruction.firstOperand.type != operand_type_register && instruction.secondOperand.type != operand_type_register)
    {
        instruction.needsDecorator = true;
    }

    instruction.segmentRegister = segmentRegister;

    instruction.byteCount = state->instructions.instructionPointer - initialStackPointer;
    return instruction;
}

OpValue getRegisterValue(RegisterLocation registerLocation, State *state)
{
    OpValue result = {0};
    assert(registerLocation.reg != reg_none);
    if (registerLocation.portion == reg_portion_x)
    {
        result.isWide = true;
        result.value.signedWord = state->registers[registerLocation.reg].x;
    }
    else if (registerLocation.portion == reg_portion_l)
    {
        result.value.signedByte = state->registers[registerLocation.reg].lh.l;
    }
    else
    {
        result.value.signedByte = state->registers[registerLocation.reg].lh.h;
    }

    return result;
}

size_t getAddress(Operand source, State *state)
{
    assert(source.type == operand_type_memory);
    size_t address = 0;
    if (source.payload.memory.regCount == 1)
    {
        OpValue regValue = getRegisterValue(source.payload.memory.reg0, state);

        address = regValue.isWide ? regValue.value.signedWord : regValue.value.signedByte;
    }
    else if (source.payload.memory.regCount == 2)
    {
        OpValue reg0Value = getRegisterValue(source.payload.memory.reg0, state);

        OpValue reg1Value = getRegisterValue(source.payload.memory.reg1, state);

        OpValue sum = opValueAdd(reg0Value, reg1Value);

        address = sum.isWide ? sum.value.signedWord : sum.value.signedByte;
    }

    address += source.payload.memory.displacement;
    return address;
}

OpValue getOperandValue(Operand source, bool isWide, State *state)
{
    OpValue result = {0};

    result.isWide = isWide;

    if (source.type == operand_type_register)
    {
        result = getRegisterValue(source.payload.reg, state);
    }
    else if (source.type == operand_type_immediate)
    {
        if (isWide)
        {
            result.value.signedWord = source.payload.immediate.value;
        }
        else
        {
            result.value.signedByte = (int8_t)source.payload.immediate.value;
        }
    }
    else
    {
        size_t address = getAddress(source, state);

        if (isWide)
        {
            result.value.signedWord = *((int16_t *)(state->memory + address));
        }
        else
        {
            result.value.signedByte = state->memory[address];
        }
    }

    return result;
}

void setDestination(Operand destination, OpValue sourceValue, State *state)
{

    if (destination.type == operand_type_register)
    {

        assert(destination.payload.reg.reg != reg_none);
        if (destination.payload.reg.portion == reg_portion_x)
        {
            assert(sourceValue.isWide);
            state->registers[destination.payload.reg.reg].x = sourceValue.value.signedWord;
        }
        else if (destination.payload.reg.portion == reg_portion_l)
        {
            assert(!sourceValue.isWide);
            state->registers[destination.payload.reg.reg].lh.l = sourceValue.value.signedByte;
        }
        else
        {
            assert(!sourceValue.isWide);
            state->registers[destination.payload.reg.reg].lh.h = sourceValue.value.signedByte;
        }
    }
    else
    {
        size_t address = getAddress(destination, state);
        if (address >= MEMORY_SIZE)
        {
            error(
                __FILE__,
                __LINE__,
                state->isNoWait,
                "The requested address %zu exceeds the memory limit of %u bytes",
                address,
                MEMORY_SIZE);
        }

        if (sourceValue.isWide)
        {
            *((int16_t *)(state->memory + address)) = sourceValue.value.signedWord;
        }
        else
        {
            state->memory[address] = sourceValue.value.signedByte;
        }
    }
}

OpValue opValueSubtract(OpValue left, OpValue right)
{
    assert(left.isWide == right.isWide);

    OpValue result = {0};
    result.isWide = left.isWide;
    if (result.isWide)
    {
        result.value.signedWord = left.value.signedWord - right.value.signedWord;
    }
    else
    {
        result.value.signedByte = left.value.signedByte - right.value.signedByte;
    }

    if (result.isWide)
    {
        if (result.value.unsignedWord > left.value.unsignedWord)
        {
            result.isCarry = true;
        }
    }
    else
    {
        if (result.value.signedWord > left.value.signedWord)
        {
            result.isCarry = true;
        }
    }

    return result;
}

OpValue opValueAdd(OpValue left, OpValue right)
{
    assert(left.isWide == right.isWide);

    OpValue result = {0};
    result.isWide = left.isWide;
    if (result.isWide)
    {
        result.value.signedWord = left.value.signedWord + right.value.signedWord;
    }
    else
    {
        result.value.signedByte = left.value.signedByte + right.value.signedByte;
    }

    if (result.isWide)
    {
        if (result.value.unsignedWord < left.value.unsignedWord)
        {
            result.isCarry = true;
        }
    }
    else
    {
        if (result.value.signedWord < left.value.signedWord)
        {
            result.isCarry = true;
        }
    }

    return result;
}

bool isZero(OpValue value)
{

    if (value.isWide)
    {
        return value.value.signedWord == 0;
    }
    else
    {
        return value.value.signedByte == 0;
    }
}

bool isNegative(OpValue value)
{

    if (value.isWide)
    {
        return value.value.signedWord < 0;
    }
    else
    {
        return value.value.signedByte < 0;
    }
}

void updateZeroFlag(OpValue result, State *state)
{
    if (isZero(result))
    {
        state->flags[flag_zero] = true;
    }
    else
    {
        state->flags[flag_zero] = false;
    }
}

void updateSignFlag(OpValue result, State *state)
{
    if (isNegative(result))
    {
        state->flags[flag_sign] = true;
    }
    else
    {
        state->flags[flag_sign] = false;
    }
}

void updateParityFlag(OpValue result, State *state)
{
    size_t setCount = 0;
    for (uint8_t bitIndex = 0; bitIndex < 8; bitIndex++)
    {
        bool isSet = extractBit(result.value.signedByte, bitIndex);
        if (isSet)
        {
            setCount++;
        }
    }
    if (setCount % 2 == 0)
    {
        state->flags[flag_parity] = true;
    }
    else
    {
        state->flags[flag_parity] = false;
    }
}

void updateCarryFlag(OpValue result, State *state)
{
    if (result.isCarry)
    {
        state->flags[flag_carry] = true;
    }
    else
    {
        state->flags[flag_carry] = false;
    }
}

void executeInstruction(Instruction instruction, State *state)
{

    if (instruction.type == instruction_mov)
    {
        assert(instruction.operandCount == 2);

        OpValue sourceValue = getOperandValue(instruction.secondOperand, instruction.isWide, state);

        setDestination(instruction.firstOperand, sourceValue, state);
    }
    else if (instruction.type == instruction_sub)
    {
        OpValue sourceValue = getOperandValue(instruction.secondOperand, instruction.isWide, state);
        OpValue destinationValue = getOperandValue(instruction.firstOperand, instruction.isWide, state);

        OpValue result = opValueSubtract(destinationValue, sourceValue);

        setDestination(instruction.firstOperand, result, state);

        updateCarryFlag(result, state);
        updateZeroFlag(result, state);
        updateSignFlag(result, state);
        updateParityFlag(result, state);
    }
    else if (instruction.type == instruction_cmp)
    {
        OpValue left = getOperandValue(instruction.firstOperand, instruction.isWide, state);

        OpValue right = getOperandValue(instruction.secondOperand, instruction.isWide, state);

        OpValue result = opValueSubtract(left, right);

        updateZeroFlag(result, state);
        updateSignFlag(result, state);
        updateParityFlag(result, state);
    }
    else if (instruction.type == instruction_add)
    {
        OpValue left = getOperandValue(instruction.firstOperand, instruction.isWide, state);

        OpValue right = getOperandValue(instruction.secondOperand, instruction.isWide, state);

        OpValue result = opValueAdd(left, right);
        setDestination(instruction.firstOperand, result, state);

        updateCarryFlag(result, state);
        updateZeroFlag(result, state);
        updateSignFlag(result, state);
        updateParityFlag(result, state);
    }
    else if (instruction.type == instruction_jnz)
    {
        if (!state->flags[flag_zero])
        {
            state->instructions.instructionPointer += instruction.firstOperand.payload.immediate.value;
        }
    }
    else
    {
        assert(instruction.type == InstructionInfos[instruction.type].type);
        error(
            __FILE__,
            __LINE__,
            state->isNoWait,
            "Unimplemented instruction: %s",
            InstructionInfos[instruction.type].name);
    }
}

bool cStringsEqual(char *left, char *right)
{
    return strcmp(left, right) == 0;
}

int main(int argc, char *argv[])
{
    if (argc < 1)
    {
        error(__FILE__, __LINE__, "Usage: %s <filename>", argv[0]);
    }

    State state = {0};

    state.memory = malloc(MEMORY_SIZE);

    if (state.memory == NULL)
    {
        error(__FILE__, __LINE__, state.isNoWait, "Failed to allocate");
    }

    const char *fileName = NULL;

    for (size_t argumentIndex = 1; argumentIndex < argc; argumentIndex++)
    {
        char *argument = argv[argumentIndex];

        if (cStringsEqual(argument, "--nowait"))
        {
            state.isNoWait = true;
        }
        else if (cStringsEqual(argument, "--execute"))
        {
            state.execute = true;
        }
        else if (cStringsEqual(argument, "--dump"))
        {
            state.dump = true;
        }
        else if (cStringsEqual(argument, "--img"))
        {
            state.image = true;
        }
        else if (fileName == NULL)
        {
            fileName = argument;
        }
        else
        {
            error(__FILE__, __LINE__, state.isNoWait, "Invalid argument %s", argument);
        }
    }

    size_t fileSize;

    char *bytes = readFile(fileName, &fileSize, &state);
    state.instructions.bytes = bytes;
    state.instructions.size = fileSize;

    while (state.instructions.instructionPointer < state.instructions.size)
    {

        State before = state;

        Instruction instruction = decodeInstruction(&state);

        if (state.execute)
        {
            executeInstruction(instruction, &state);
        }
        State after = state;

        printInstruction(instruction, &before, &after);
    }

    if (state.execute)
    {
        for (size_t regIndex = 0; regIndex < REGISTER_COUNT; regIndex++)
        {
            printf("; %s = \t%u\n", RegisterInfos[regIndex].name, state.registers[regIndex].x);
        }

        printf("; ip = \t %u\n", state.instructions.instructionPointer);

        printf("Flags:");
        printFlags(&state);
    }

    if (state.dump)
    {
        FILE *outputFile = fopen(DUMP_PATH, "wb");

        if (outputFile != NULL)
        {
            if (fwrite(&state, sizeof(State), 1, outputFile) != 1)
            {
                fprintf(stderr, "Could not dump final state");
            }

            fclose(outputFile);
        }
    }

    if (state.image)
    {
        FILE *outputFile = fopen(IMAGE_PATH, "wb");

        if (outputFile != NULL)
        {
            if (fwrite(state.memory, MEMORY_SIZE, 1, outputFile) != 1)
            {
                fprintf(stderr, "Could not write image");
            }

            fclose(outputFile);
        }
    }

    printPressEnterToContinue(state.isNoWait);
    if (bytes != NULL)
    {
        free(bytes);
    }

    if (state.memory != NULL)
    {
        free(state.memory);
    }

    return 0;
}
