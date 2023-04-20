#include "8086.h"

#define LISTING_37 "../computer_enhance/perfaware/part1/listing_0037_single_register_mov"
#define LISTING_38 "../computer_enhance/perfaware/part1/listing_0038_many_register_mov"
#define LISTING_39 "../computer_enhance/perfaware/part1/listing_0039_more_movs"
#define LISTING_40 "../computer_enhance/perfaware/part1/listing_0040_challenge_movs"
#define LISTING_41 "../computer_enhance/perfaware/part1/listing_0041_add_sub_cmp_jnz"
#define LISTING_42 "../computer_enhance/perfaware/part1/listing_0042_completionist_decode"
#define LISTING_43 "../computer_enhance/perfaware/part1/listing_0043_immediate_movs"
#define LISTING_44 "../computer_enhance/perfaware/part1/listing_0044_register_movs"
#define LISTING_45 "../computer_enhance/perfaware/part1/listing_0045_challenge_register_movs"
#define LISTING_46 "../computer_enhance/perfaware/part1/listing_0046_add_sub_cmp"
#define LISTING_47 "../computer_enhance/perfaware/part1/listing_0047_challenge_flags"
#define LISTING_48 "../computer_enhance/perfaware/part1/listing_0048_ip_register"
#define LISTING_49 "../computer_enhance/perfaware/part1/listing_0049_conditional_jumps"
#define LISTING_50 "../computer_enhance/perfaware/part1/listing_0050_challenge_jumps"
#define LISTING_51 "../computer_enhance/perfaware/part1/listing_0051_memory_mov"
#define LISTING_52 "../computer_enhance/perfaware/part1/listing_0052_memory_add_loop"
#define LISTING_53 "../computer_enhance/perfaware/part1/listing_0053_add_loop_challenge"
#define LISTING_54 "../computer_enhance/perfaware/part1/listing_0054_draw_rectangle"
#define LISTING_55 "../computer_enhance/perfaware/part1/listing_0055_challenge_rectangle"

char *readFile(const char *filename, size_t *len)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        assert(false);
    }
    if (fseek(file, 0, SEEK_END) < 0)
    {
        assert(false);
    }

    *len = ftell(file);

    if (fseek(file, 0, SEEK_SET) < 0)
    {
        assert(false);
    }

    char *bytes = malloc(*len);

    if (!bytes)
    {
        assert(false);
    }

    if (fread(bytes, *len, 1, file) != 1)
    {
        assert(false);
    }

    if (file != NULL)
    {
        fclose(file);
    }

    return bytes;
}

size_t fileNameStart(const char *filePath)
{

    size_t index = 0;
    size_t result = 0;
    while (filePath[index] != '\0')
    {
        if (filePath[index] == '/' || filePath[index] == '\\')
        {
            result = index + 1;
        }
        index++;
    }

    return result;
}

void testDecoding(const char *filePath)
{
    printf("Decoding %s...\n", filePath);
    size_t offset = fileNameStart(filePath);
    char buffer[1024];
    sprintf(buffer, "8086.exe %s > tmp/%s.asm ", filePath, filePath + offset);
    system(buffer);
    sprintf(buffer, "nasm tmp/%s.asm -o tmp/%s.out", filePath + offset, filePath + offset);
    system(buffer);

    size_t inputFileSize;
    char *inputData = readFile(filePath, &inputFileSize);

    size_t outputFileSize;
    sprintf(buffer, "tmp/%s.out", filePath + offset);
    char *outputData = readFile(buffer, &outputFileSize);

    assert(inputFileSize == outputFileSize);
    assert(memcmp(inputData, outputData, inputFileSize) == 0);
}

void testFinalState(const char *filePath, State expected, bool testIp)
{

    printf("Executing %s...\n", filePath);
    char buffer[1024];
    sprintf(buffer, "8086.exe --dump --execute %s > nul ", filePath);
    system(buffer);

    size_t fileSize;
    char *inputData = readFile(DUMP_PATH, &fileSize);
    State *found = (State *)inputData;

    for (size_t regIndex = 0; regIndex < REGISTER_COUNT; regIndex++)
    {

        printf(
            "%s : expected %d, found %d\n",
            RegisterInfos[regIndex].name,
            expected.registers[regIndex].x,
            found->registers[regIndex].x);

        assert(expected.registers[regIndex].x == found->registers[regIndex].x);
    }

    for (size_t flagIndex = 0; flagIndex < FLAG_COUNT; flagIndex++)
    {

        printf(
            "%s : expected %d, found %d\n",
            FlagNames[flagIndex].name,
            expected.flags[flagIndex],
            found->flags[flagIndex]);

        assert(expected.flags[flagIndex] == found->flags[flagIndex]);
    }

    if (testIp)
    {
        printf(
            "ip : expected %d, found %d\n",
            expected.instructions.instructionPointer,
            found->instructions.instructionPointer);

        assert(expected.instructions.instructionPointer == found->instructions.instructionPointer);
    }
}

void testFinalState43()
{
    State expected = {0};

    expected.registers[reg_a].x = 1;
    expected.registers[reg_b].x = 2;
    expected.registers[reg_c].x = 3;
    expected.registers[reg_d].x = 4;
    expected.registers[reg_sp].x = 5;
    expected.registers[reg_bp].x = 6;
    expected.registers[reg_si].x = 7;
    expected.registers[reg_di].x = 8;

    testFinalState(LISTING_43, expected, false);
}

void testFinalState44()
{
    State expected = {0};

    expected.registers[reg_a].x = 4;
    expected.registers[reg_b].x = 3;
    expected.registers[reg_c].x = 2;
    expected.registers[reg_d].x = 1;
    expected.registers[reg_sp].x = 1;
    expected.registers[reg_bp].x = 2;
    expected.registers[reg_si].x = 3;
    expected.registers[reg_di].x = 4;

    testFinalState(LISTING_44, expected, false);
}

void testFinalState45()
{
    State expected = {0};
    expected.registers[reg_a].x = 17425;
    expected.registers[reg_b].x = 13124;
    expected.registers[reg_c].x = 26231;
    expected.registers[reg_d].x = 30600;
    expected.registers[reg_sp].x = 17425;
    expected.registers[reg_bp].x = 13124;
    expected.registers[reg_si].x = 26231;
    expected.registers[reg_di].x = 30600;
    expected.registers[reg_es].x = 26231;
    expected.registers[reg_ss].x = 17425;
    expected.registers[reg_ds].x = 13124;

    testFinalState(LISTING_45, expected, false);
}

void testFinalState46()
{
    State expected = {0};
    expected.registers[reg_b].x = -7934;
    expected.registers[reg_c].x = 3841;
    expected.registers[reg_sp].x = 998;
    expected.flags[flag_parity] = true;
    expected.flags[flag_zero] = true;

    testFinalState(LISTING_46, expected, false);
}

void testFinalState47()
{
    State expected = {0};
    expected.registers[reg_b].x = -25435;
    expected.registers[reg_d].x = 10;
    expected.registers[reg_sp].x = 99;
    expected.registers[reg_bp].x = 98;

    expected.flags[flag_carry] = true;
    expected.flags[flag_parity] = true;
    expected.flags[flag_aux_carry] = true;
    expected.flags[flag_sign] = true;

    testFinalState(LISTING_47, expected, false);
}

void testFinalState48()
{
    State expected = {0};

    expected.registers[reg_b].x = 2000;
    expected.registers[reg_c].x = 64736;
    expected.instructions.instructionPointer = 14;
    expected.flags[flag_carry] = true;
    expected.flags[flag_sign] = true;

    testFinalState(LISTING_48, expected, true);
}

void testFinalState49()
{
    State expected = {0};

    expected.registers[reg_b].x = 1030;
    expected.instructions.instructionPointer = 14;
    expected.flags[flag_parity] = true;
    expected.flags[flag_zero] = true;

    testFinalState(LISTING_49, expected, true);
}

void testFinalState50()
{
    State expected = {0};

    expected.registers[reg_a].x = 13;
    expected.registers[reg_b].x = -5;
    expected.instructions.instructionPointer = 28;
    expected.flags[flag_carry] = true;
    expected.flags[flag_aux_carry] = true;
    expected.flags[flag_sign] = true;

    testFinalState(LISTING_50, expected, true);
}

void testFinalState51()
{
    State expected = {0};

    expected.registers[reg_b].x = 1;
    expected.registers[reg_c].x = 2;
    expected.registers[reg_d].x = 10;
    expected.registers[reg_bp].x = 4;
    expected.instructions.instructionPointer = 48;

    testFinalState(LISTING_51, expected, true);
}

void testFinalState52()
{
    State expected = {0};

    expected.registers[reg_b].x = 6;
    expected.registers[reg_c].x = 4;
    expected.registers[reg_d].x = 6;
    expected.registers[reg_bp].x = 1000;
    expected.registers[reg_si].x = 6;
    expected.instructions.instructionPointer = 35;
    expected.flags[flag_zero] = true;
    expected.flags[flag_parity] = true;

    testFinalState(LISTING_52, expected, true);
}

void testFinalState53()
{
    State expected = {0};

    expected.registers[reg_b].x = 6;
    expected.registers[reg_d].x = 6;
    expected.registers[reg_bp].x = 998;
    expected.instructions.instructionPointer = 33;
    expected.flags[flag_zero] = true;
    expected.flags[flag_parity] = true;

    testFinalState(LISTING_53, expected, true);
}

void testFinalState54()
{
    State expected = {0};

    expected.registers[reg_c].x = 64;
    expected.registers[reg_d].x = 64;
    expected.registers[reg_bp].x = 16640;
    expected.instructions.instructionPointer = 38;
    expected.flags[flag_zero] = true;
    expected.flags[flag_parity] = true;

    testFinalState(LISTING_54, expected, true);
}

void testFinalState55()
{
    State expected = {0};

    expected.registers[reg_b].x = 16388;
    expected.registers[reg_bp].x = 764;
    expected.instructions.instructionPointer = 68;

    testFinalState(LISTING_55, expected, true);
}

int main(void)
{

    testDecoding(LISTING_37);
    testDecoding(LISTING_38);
    testDecoding(LISTING_39);
    testDecoding(LISTING_40);
    testDecoding(LISTING_41);
    testDecoding(LISTING_42);
    testDecoding(LISTING_43);
    testDecoding(LISTING_44);
    testDecoding(LISTING_45);
    testDecoding(LISTING_46);
    testDecoding(LISTING_47);
    testDecoding(LISTING_48);
    testDecoding(LISTING_49);
    testDecoding(LISTING_50);
    testDecoding(LISTING_51);
    testDecoding(LISTING_52);
    testDecoding(LISTING_53);
    testDecoding(LISTING_54);
    testDecoding(LISTING_55);

    testFinalState43();
    testFinalState44();
    testFinalState45();
    testFinalState46();
    testFinalState47();
    testFinalState48();
    testFinalState49();
    testFinalState50();
    testFinalState51();
    testFinalState52();
    testFinalState53();
    testFinalState54();
    testFinalState55();
}