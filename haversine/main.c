
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

#include "common.c"

#include "parser.c"

int main(void)
{

    Arena arena = arenaInit();

    String text = readFileToString(JSON_PATH, &arena);

    // printf("Json: %.*s", (int)text.size, text.data);
    //  printf("hi");

    Parser parser = initParser(text, &arena);

    return 0;
}