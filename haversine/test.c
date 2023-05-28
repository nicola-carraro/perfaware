
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

int main(void)
{
    FILE *file = fopen("data/pairs.double", "r");

    for (size_t i = 0; i < 100; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            double d;
            fread(&d, sizeof(double), 1, file);
            printf("%f ", d);
        }
        printf("\n");
    }
}