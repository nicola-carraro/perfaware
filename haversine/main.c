
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

#include "common.c"

int main(void)
{

    String text = readFileToString(JSON_PATH);

    printf("Json: %.*s", (int)text.size, text.data);
    // printf("hi");

    return 0;
}