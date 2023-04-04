#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "assert.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"

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

void testCurrent()
{
    system("nasm data/listing41.asm -o data/listing41");
    testDecoding("data/listing41");
}

int main(void)
{

    testDecoding("../computer_enhance/perfaware/part1/listing_0037_single_register_mov");
    testDecoding("../computer_enhance/perfaware/part1/listing_0038_many_register_mov");
    testDecoding("../computer_enhance/perfaware/part1/listing_0039_more_movs");
    testCurrent();
    // testDecoding("../computer_enhance/perfaware/part1/listing_0041_add_sub_cmp_jnz");
}