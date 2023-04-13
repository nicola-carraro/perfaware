#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "stdbool.h"
#include "stdint.h"
#include "assert.h"
#include "string.h"

#define DUMP_PATH "tmp/dump"

#define IMAGE_PATH "tmp/image.data"

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
    reg_cs,
    reg_ds,
    reg_ss,
    reg_es,
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

typedef struct
{
    int16_t value;
    bool isRelativeOffset;
    bool isIntersegment;
    int16_t ip;
    int16_t cs;
} Immediate;

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
        Immediate immediate;
    } payload;
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
    {reg_di, "di", false},
    {reg_cs, "cs", false},
    {reg_ds, "ds", false},
    {reg_ss, "ss", false},
    {reg_es, "es", false},
};

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
    instruction_neg,
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
    instruction_rcr,
    instruction_and,
    instruction_test,
    instruction_or,
    instruction_xor,
    instruction_rep,
    instruction_movs,
    instruction_cmps,
    instruction_scas,
    instruction_lods,
    instruction_stos,
    instruction_call,
    instruction_jmp,
    instruction_ret,
    instruction_retf,
    instruction_je,
    instruction_jl,
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
    instruction_ja,
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
    instruction_seg,
    instruction_cs,
    instruction_ds,
    instruction_es,
    instruction_ss,
} InstructionType;

typedef enum
{
    flag_trap,
    flag_direction,
    flag_interrupt_enable,
    flag_overflow,
    flag_sign,
    flag_zero,
    flag_aux_carry,
    flag_parity,
    flag_carry
} Flag;

struct
{
    Flag flag;
    char name[2];
} FlagNames[] =
    {{flag_trap, "T"},
     {flag_direction, "D"},
     {flag_interrupt_enable, "I"},
     {flag_overflow, "O"},
     {flag_sign, "S"},
     {flag_zero, "Z"},
     {flag_aux_carry, "A"},
     {flag_parity, "P"},
     {flag_carry, "C"}};

const struct
{
    InstructionType type;
    char name[10];
    bool needsWithSuffix;
} InstructionInfos[] = {
    {instruction_none, ""},
    {instruction_mov, "mov"},
    {instruction_push, "push"},
    {instruction_pop, "pop"},
    {instruction_xchg, "xchg"},
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
    {instruction_neg, "neg"},
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
    {instruction_rcr, "rcr"},
    {instruction_and, "and"},
    {instruction_test, "test"},
    {instruction_or, "or"},
    {instruction_xor, "xor"},
    {instruction_rep, "rep"},
    {instruction_movs, "movs", true},
    {instruction_cmps, "cmps", true},
    {instruction_scas, "scas", true},
    {instruction_lods, "lods", true},
    {instruction_stos, "stos", true},
    {instruction_call, "call"},
    {instruction_jmp, "jmp"},
    {instruction_ret, "ret"},
    {instruction_retf, "retf"},
    {instruction_je, "je"},
    {instruction_jl, "jl"},
    {instruction_jle, "jle"},
    {instruction_jb, "jb"},
    {instruction_jbe, "jbe"},
    {instruction_jp, "jp"},
    {instruction_jo, "jo"},
    {instruction_js, "js"},
    {instruction_jnz, "jnz"},
    {instruction_jnl, "jnl"},
    {instruction_jnle, "jnle"},
    {instruction_jnb, "jnb"},
    {instruction_ja, "ja"},
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
    {instruction_seg, "seg"},
    {instruction_cs, "cs:"},
    {instruction_ds, "ds:"},
    {instruction_es, "es:"},
    {instruction_ss, "ss:"},
};

typedef struct
{
    InstructionType type;
    Operand firstOperand;
    Operand secondOperand;
    uint8_t operandCount;
    bool isWide;
    bool needsDecorator;
    uint16_t byteCount;
    bool isInt3;
    bool isFar;
    Register segmentRegister;

} Instruction;

typedef struct
{
    char *bytes;
    size_t size;
    uint16_t instructionPointer;
} Stream;

#define REGISTER_COUNT 8
#define FLAG_COUNT 9
#define MEMORY_SIZE 1024 * 1024
typedef struct
{
    bool isNoWait;
    bool execute;
    bool dump;
    bool image;
    Stream instructions;
    uint8_t *memory;
    union
    {
        int16_t x;
        struct
        {
            int8_t l;
            int8_t h;
        } lh;
    } registers[REGISTER_COUNT];

    bool flags[FLAG_COUNT];

} State;

typedef struct
{
    bool isWide;
    bool isCarry;
    union
    {
        int16_t signedWord;
        int8_t signedByte;
        uint8_t unsignedByte;
        uint16_t unsignedWord;
    } value;

} OpValue;

OpValue opValueAdd(OpValue left, OpValue right);