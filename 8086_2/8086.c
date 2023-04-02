#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "stdbool.h"
#include "stdint.h"
#include "assert.h"
#include "string.h"

#define REG_COUNT 8

typedef enum
{
    reg_a,
    reg_b,
    reg_c,
    reg_d,
    reg_sp,
    reg_bp,
    reg_si,
    reg_di,
    reg_none
} Register;

typedef enum
{
    reg_portion_x,
    reg_portion_l,
    reg_portion_h
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
    operand_type_memory,
    operand_type_register,
    operand_type_immediate
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
    {reg_a, "a", true},
    {reg_b, "b", true},
    {reg_c, "c", true},
    {reg_d, "d", true},
    {reg_sp, "sp", false},
    {reg_bp, "bp", false},
    {reg_si, "si", false},
    {reg_di, "di", false}};

const struct
{
    const RegisterPortion portion;
    const char name[2];
} RegisterPortionInfos[] = {
    {reg_portion_x, "x"},
    {reg_portion_l, "l"},
    {reg_portion_h, "h"},
};

const struct
{
    RegisterLocation w0Reg;
    RegisterLocation w1Reg;
} RegFieldInfo[] = {
    {{reg_a, reg_portion_l}, {reg_a, reg_portion_x}},
    {{reg_c, reg_portion_l}, {reg_c, reg_portion_x}},
    {{reg_d, reg_portion_l}, {reg_d, reg_portion_x}},
    {{reg_b, reg_portion_l}, {reg_b, reg_portion_x}},
    {{reg_a, reg_portion_h}, {reg_sp}},
    {{reg_c, reg_portion_h}, {reg_bp}},
    {{reg_d, reg_portion_h}, {reg_si}},
    {{reg_b, reg_portion_h}, {reg_di}}};

const struct
{
    RegisterLocation mod3W0Reg;
    RegisterLocation mod3w1Reg;

    MemoryLocation memoryLocation;
} RmFieldInfo[] = {
    {{reg_a, reg_portion_l}, {reg_a, reg_portion_x}, {{reg_b, reg_portion_x}, {reg_si}, 2}},
    {{reg_c, reg_portion_l}, {reg_c, reg_portion_x}, {{reg_b, reg_portion_x}, {reg_di}, 2}},
    {{reg_d, reg_portion_l}, {reg_d, reg_portion_x}, {{reg_bp}, {reg_si}, 2}},
    {{reg_b, reg_portion_l}, {reg_b, reg_portion_x}, {{reg_bp}, {reg_di}, 2}},
    {{reg_a, reg_portion_h}, {reg_sp}, {{reg_si}, {reg_none}, 1}},
    {{reg_c, reg_portion_h}, {reg_bp}, {{reg_di}, {reg_none}, 1}},
    {{reg_d, reg_portion_h}, {reg_si}, {{reg_bp}, {reg_none}, 1}},
    {{reg_b, reg_portion_h}, {reg_di}, {{reg_b, reg_portion_x}, {reg_none}, 1}},
};

typedef enum
{
    instruction_none,
    instruction_mov,
    instruction_push,
    instruction_pop,
    instruction_xchg,
    instruction_in,
    instruction_out,
    instruction_xlat,
    instruction_lea,
    instruction_lds,
    instruction_les,
    instruction_lahf,
    instruction_sahf,
    instruction_pushf,
    instruction_popf,
    instruction_add,
    instruction_adc,
    instruction_inc,
    instruction_aaa,
    instruction_daa,
    instruction_sub,
    instruction_sbb,
    instruction_dec,
    instruction_cmp,
    instruction_aas,
    instruction_das,
    instruction_mul,
    instruction_imul,
    instruction_aam,
    instruction_div,
    instruction_idiv,
    instruction_aad,
    instruction_cbw,
    instruction_cwd,
    instruction_not,
    instruction_shl,
    instruction_shr,
    instruction_sar,
    instruction_rol,
    instruction_ror,
    instruction_rcl,
    instruction_and,
    instruction_test,
    instruction_or,
    instruction_xor,
    instruction_rep,
    instruction_movs,
    instruction_cmps,
    instruction_scas,
    instruction_lods,
    instruction_stds,
    instruction_call,
    instruction_jmp,
    instruction_ret,
    instruction_je,
    instruction_jle,
    instruction_jb,
    instruction_jbe,
    instruction_jp,
    instruction_jo,
    instruction_js,
    instruction_jnz,
    instruction_jnl,
    instruction_jnle,
    instruction_jnb,
    instruction_jnbe,
    instruction_jnp,
    instruction_jno,
    instruction_jns,
    instruction_loop,
    instruction_loopz,
    instruction_loopnz,
    instruction_jcxz,
    instruction_int,
    instruction_into,
    instruction_iret,
    instruction_clc,
    instruction_cmc,
    instruction_stc,
    instruction_cld,
    instruction_std,
    instruction_cli,
    instruction_sti,
    instruction_hlt,
    instruction_wait,
    instruction_esc,
    instruction_lock,
    instruction_seg
} InstructionType;

const struct
{
    InstructionType type;
    char name[10];
} InstructionNames[] = {
    {instruction_none, ""},
    {instruction_mov, "mov"},
    {instruction_push, "push"},
    {instruction_pop, "pop"},
    {instruction_in, "in"},
    {instruction_out, "out"},
    {instruction_xlat, "xlat"},
    {instruction_lea, "lea"},
    {instruction_lds, "lds"},
    {instruction_les, "les"},
    {instruction_lahf, "lahf"},
    {instruction_sahf, "sahf"},
    {instruction_pushf, "pushf"},
    {instruction_popf, "popf"},
    {instruction_add, "add"},
    {instruction_adc, "adc"},
    {instruction_inc, "inc"},
    {instruction_aaa, "aaa"},
    {instruction_daa, "daa"},
    {instruction_sub, "sub"},
    {instruction_sbb, "sbb"},
    {instruction_dec, "dec"},
    {instruction_cmp, "cmp"},
    {instruction_aas, "aas"},
    {instruction_das, "das"},
    {instruction_mul, "mul"},
    {instruction_imul, "imul"},
    {instruction_aam, "aam"},
    {instruction_div, "div"},
    {instruction_idiv, "idiv"},
    {instruction_aad, "aad"},
    {instruction_cbw, "cbw"},
    {instruction_cwd, "cwd"},
    {instruction_not, "not"},
    {instruction_shl, "shl"},
    {instruction_shr, "shr"},
    {instruction_sar, "sar"},
    {instruction_rol, "rol"},
    {instruction_ror, "ror"},
    {instruction_rcl, "rcl"},
    {instruction_and, "and"},
    {instruction_test, "test"},
    {instruction_or, "or"},
    {instruction_xor, "xor"},
    {instruction_rep, "rep"},
    {instruction_movs, "movs"},
    {instruction_cmps, "cmps"},
    {instruction_scas, "scas"},
    {instruction_lods, "lods"},
    {instruction_stds, "stds"},
    {instruction_call, "call"},
    {instruction_jmp, "jmp"},
    {instruction_ret, "ret"},
    {instruction_je, "je"},
    {instruction_jle, "jle"},
    {instruction_jb, "jb"},
    {instruction_jbe, "jbe"},
    {instruction_jp, "jp"},
    {instruction_jo, "jo"},
    {instruction_js, "js"},
    {instruction_jnz, "jnz"},
    {instruction_jnl, "jnl"},
    {instruction_jnle, "jnle"},
    {instruction_jnb, "jnne"},
    {instruction_jnbe, "jnbe"},
    {instruction_jnp, "jnp"},
    {instruction_jno, "jno"},
    {instruction_jns, "jns"},
    {instruction_loop, "loop"},
    {instruction_loopz, "loopz"},
    {instruction_loopnz, "loopnz"},
    {instruction_jcxz, "jcxz"},
    {instruction_int, "int"},
    {instruction_into, "into"},
    {instruction_iret, "iret"},
    {instruction_clc, "clc"},
    {instruction_cmc, "cmc"},
    {instruction_stc, "stc"},
    {instruction_cld, "cld"},
    {instruction_std, "std"},
    {instruction_cli, "cli"},
    {instruction_sti, "sti"},
    {instruction_hlt, "hlt"},
    {instruction_wait, "wait"},
    {instruction_esc, "esc"},
    {instruction_lock, "lock"},
    {instruction_seg, "seg"}};

typedef struct
{
    InstructionType type;
    Operand firstOperand;
    Operand secondOperand;
    uint8_t operandCount;
    bool isWide;
} Instruction;

typedef struct
{
    char *bytes;
    size_t size;
    size_t position;
} Stream;

typedef struct
{
    bool isNoWait;
} State;

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

int16_t consumeTwoBytesAsSigned(Stream *instructions, State *state)
{
    if (instructions->position + 1 >= instructions->size)
    {
        error(__FILE__, __LINE__, state->isNoWait, "reached end of instuctions stream");
    }

    int16_t result = *((int16_t *)(instructions + instructions->position));

    instructions->position++;

    return result;
}

uint8_t consumeByteAsUnsigned(Stream *instructions, State *state)
{
    if (instructions->position >= instructions->size)
    {
        error(__FILE__, __LINE__, state->isNoWait, "reached end of instuctions stream");
    }

    uint8_t result = instructions->bytes[instructions->position];

    instructions->position++;

    return result;
}

int8_t consumeByteAsSigned(Stream *instructions, State *state)
{
    if (instructions->position >= instructions->size)
    {
        error(__FILE__, __LINE__, state->isNoWait, "reached end of instuctions stream");
    }

    int8_t result = instructions->bytes[instructions->position];

    instructions->position++;

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
    result.location.reg = wBit ? RegFieldInfo[reg].w1Reg : RegFieldInfo[reg].w0Reg;

    return result;
}

Operand decodeRmOperand(bool wBit, uint8_t mod, uint8_t rm, Stream *instructions, State *state)
{
    checkMod(__FILE__, __LINE__, mod, state);
    checkRm(__FILE__, __LINE__, rm, state);
    assert(rm <= 7);

    Operand result = {0};

    bool immediateAddressing = (mod == 0 && rm == 6);
    if (immediateAddressing)
    {
        result.type = operand_type_memory;
        result.location.memory.regCount = 0;
        result.location.memory.displacement = consumeTwoBytesAsSigned(instructions, state);
    }
    else
    {
        if (mod == 3)
        {
            result.type = operand_type_register;
            result.location.reg = wBit ? RmFieldInfo[rm].mod3w1Reg : RmFieldInfo[rm].mod3W0Reg;
        }
        else
        {
            result.type = operand_type_memory;
            result.location.memory = RmFieldInfo[rm].memoryLocation;

            if (mod == 1)
            {
                int16_t displacement = consumeByteAsSigned(instructions, state);
                result.location.memory.displacement = displacement;
            }
            else if (mod == 2)
            {
                result.location.memory.displacement = consumeTwoBytesAsSigned(instructions, state);
            }
        }
    }

    return result;
}

void printOperand(Operand operand)
{

    printf(" ");
    if (operand.type == operand_type_register)
    {

        assert(RegisterInfos[operand.location.reg.reg].reg == operand.location.reg.reg);
        printf(RegisterInfos[operand.location.reg.reg].name);

        if (RegisterInfos[operand.location.reg.reg].isPartiallyAdressable)
        {
            assert(RegisterPortionInfos[operand.location.reg.portion].portion == operand.location.reg.portion);
            printf(RegisterPortionInfos[operand.location.reg.portion].name);
        }
    }
    else
    {
        assert(false && "Unimplemented");
    }
}

void printInstruction(Instruction instruction)
{
    assert(instruction.type != instruction_none);
    assert(InstructionNames[instruction.type].type == instruction.type);

    const char *mnemonic = InstructionNames[instruction.type].name;
    printf(mnemonic);

    if (instruction.operandCount > 0)
    {
        printOperand(instruction.firstOperand);
    }

    if (instruction.operandCount > 1)
    {
        printf(",");
        printOperand(instruction.secondOperand);
    }
}

Instruction decodeRegMemToFromRegMem(bool dBit, bool wBit, uint8_t mod, uint8_t reg, uint8_t rm, Stream *instructions, State *state)
{
    Instruction result = {0};

    result.operandCount = 2;
    Operand rmOperand = decodeRmOperand(wBit, mod, rm, instructions, state);
    Operand regOperand = decodeRegOperand(wBit, reg, state);

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

    const char *fileName = NULL;

    for (size_t argumentIndex = 1; argumentIndex < argc; argumentIndex++)
    {
        char *argument = argv[argumentIndex];

        if (cStringsEqual(argument, "--nowait"))
        {
            state.isNoWait = true;
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

    Stream instructions = {0};

    instructions.bytes = bytes;
    instructions.size = fileSize;

    while (instructions.position < instructions.size)
    {
        uint8_t firstByte = consumeByteAsUnsigned(&instructions, &state);

        Instruction instruction = {0};

        bool wBit = extractLowBits(firstByte, 1);
        bool dBit = extractBits(firstByte, 1, 1);

        if (firstByte >= 0x88 && firstByte <= 0x8b)
        {
            assert(instruction.type == instruction_none);

            uint8_t secondByte = consumeByteAsUnsigned(&instructions, &state);
            uint8_t mod = extractBits(secondByte, 6, 8);
            uint8_t reg = extractBits(secondByte, 3, 6);
            uint8_t rm = extractLowBits(secondByte, 3);

            instruction = decodeRegMemToFromRegMem(dBit, wBit, mod, reg, rm, &instructions, &state);
            instruction.type = instruction_mov;
        }

        if (instruction.type == instruction_none)
        {
            error(__FILE__, __LINE__, state.isNoWait, "Unknown instruction, first byte=%#X", firstByte);
        }

        printInstruction(instruction);

        printf("\n");
    }

    printPressEnterToContinue(state.isNoWait);
    if (bytes != NULL)
    {
        free(bytes);
    }

    return 0;
}
