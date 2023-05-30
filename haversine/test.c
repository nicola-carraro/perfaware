
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

#include "common.c"

int main(void)
{
    FILE *file = fopen("data/pairs.double", "rb");

    for (size_t i = 0; i < 100; i++)
    {

        double d;
        if (fread(&d, sizeof(double), 1, file) != 1)
        {
            printf("Reached end");
            return;
        }
        printf("%zu %f ", i, d);

        printf("\n");
    }
}