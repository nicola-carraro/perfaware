#include "8086.h"

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
    sprintf(buffer, "8086.exe --nowait %s > data/output.asm ", inputFile);
    system(buffer);
    system("nasm data/output.asm -o data/output.out");

    size_t inputFileSize;
    char *inputData = readFile(inputFile, &inputFileSize);

    size_t outputFileSize;
    char *outputData = readFile("data/output.out", &outputFileSize);

    assert(inputFileSize == outputFileSize);
    assert(memcmp(inputData, outputData, inputFileSize) == 0);
}

void testFinalState(const char *filePath, State expected)
{

    printf("Executing %s...", filePath);
    char buffer[1024];
    sprintf(buffer, "8086.exe --nowait --dump --execute %s > data/output.asm ", filePath);
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

    testFinalState("../computer_enhance/perfaware/part1/listing_0048_ip_register", expected);
}

void testFinalState49()
{
    State expected = {0};

    expected.registers[reg_b].x = 1030;
    expected.instructions.instructionPointer = 14;
    expected.flags[flag_parity] = true;
    expected.flags[flag_zero] = true;

    testFinalState("../computer_enhance/perfaware/part1/listing_0049_conditional_jumps", expected);
}

void testFinalState51()
{
    State expected = {0};

    expected.registers[reg_b].x = 1;
    expected.registers[reg_c].x = 2;
    expected.registers[reg_d].x = 10;
    expected.registers[reg_bp].x = 4;
    expected.instructions.instructionPointer = 48;

    testFinalState("../computer_enhance/perfaware/part1/listing_0051_memory_mov", expected);
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

    testFinalState("../computer_enhance/perfaware/part1/listing_0052_memory_add_loop", expected);
}

void testDecoding42()
{
    system("nasm data/listing42.asm -o data/listing42");
    testDecoding("data/listing42");
}

int main(void)
{

    testDecoding("../computer_enhance/perfaware/part1/listing_0037_single_register_mov");
    testDecoding("../computer_enhance/perfaware/part1/listing_0038_many_register_mov");
    testDecoding("../computer_enhance/perfaware/part1/listing_0039_more_movs");
    testDecoding("../computer_enhance/perfaware/part1/listing_0040_challenge_movs");
    testDecoding("../computer_enhance/perfaware/part1/listing_0041_add_sub_cmp_jnz");
    testDecoding("../computer_enhance/perfaware/part1/listing_0043_immediate_movs");
    testDecoding("../computer_enhance/perfaware/part1/listing_0044_register_movs");
    testDecoding("../computer_enhance/perfaware/part1/listing_0046_add_sub_cmp");
    testDecoding("../computer_enhance/perfaware/part1/listing_0048_ip_register");
    testDecoding("../computer_enhance/perfaware/part1/listing_0049_conditional_jumps");
    testDecoding("../computer_enhance/perfaware/part1/listing_0051_memory_mov");
    testDecoding("../computer_enhance/perfaware/part1/listing_0052_memory_add_loop");

    testFinalState48();
    testFinalState49();
    testFinalState51();
    testFinalState52();

    testDecoding42();
}