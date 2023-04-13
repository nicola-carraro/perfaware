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
void testDecoding(const char *inputFile)
{
    printf("Decoding %s...\n", inputFile);
    char buffer[1024];
    sprintf(buffer, "8086.exe --nowait %s > tmp/output.asm ", inputFile);
    system(buffer);
    system("nasm tmp/output.asm -o tmp/output.out");

    size_t inputFileSize;
    char *inputData = readFile(inputFile, &inputFileSize);

    size_t outputFileSize;
    char *outputData = readFile("tmp/output.out", &outputFileSize);

    assert(inputFileSize == outputFileSize);
    assert(memcmp(inputData, outputData, inputFileSize) == 0);
}

void testFinalState(const char *filePath, State expected)
{

    printf("Executing %s...", filePath);
    char buffer[1024];
    sprintf(buffer, "8086.exe --nowait --dump --execute %s > tmp/output.asm ", filePath);
    system(buffer);

    size_t fileSize;
    char *inputData = readFile(DUMP_PATH, &fileSize);
    State *found = (State *)inputData;

    for (size_t regIndex = 0; regIndex < REG_COUNT; regIndex++)
    {

        printf(
            "%s : expected %u, found %u\n",
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
}

void testFinalState48()
{
    State expected = {0};

    expected.registers[reg_b].x = 2000;
    expected.registers[reg_c].x = 64736;
    expected.instructions.instructionPointer = 14;
    expected.flags[flag_carry] = true;
    expected.flags[flag_sign] = true;

    testFinalState(LISTING_48, expected);
}

void testFinalState49()
{
    State expected = {0};

    expected.registers[reg_b].x = 1030;
    expected.instructions.instructionPointer = 14;
    expected.flags[flag_parity] = true;
    expected.flags[flag_zero] = true;

    testFinalState(LISTING_49, expected);
}

void testFinalState51()
{
    State expected = {0};

    expected.registers[reg_b].x = 1;
    expected.registers[reg_c].x = 2;
    expected.registers[reg_d].x = 10;
    expected.registers[reg_bp].x = 4;
    expected.instructions.instructionPointer = 48;

    testFinalState(LISTING_51, expected);
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

    testFinalState(LISTING_52, expected);
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
    // testDecoding(LISTING_45);
    testDecoding(LISTING_46);
    testDecoding(LISTING_47);
    testDecoding(LISTING_48);
    testDecoding(LISTING_49);
    testDecoding(LISTING_50);
    testDecoding(LISTING_51);
    testDecoding(LISTING_52);

    testFinalState48();
    testFinalState49();
    testFinalState51();
    testFinalState52();

    // testDecoding(LISTING_45);
}