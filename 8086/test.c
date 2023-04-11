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

void test42()
{
    system("nasm data/listing42.asm -o data/listing42");
    testDecoding("data/listing42");
}

void test51()
{
    system("nasm data/listing51.asm -o data/listing51");
    testDecoding("data/listing51");
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
    test42();
    test51();
}