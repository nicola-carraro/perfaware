
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

int main(void)
{
    FILE *file = fopen("data/pairs.double", "rb");

    for (size_t i = 0; i < 1000; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            double d;
            if (fread(&d, sizeof(double), 1, file) != 1)
            {
                printf("Reached end");
                return;
            }
            printf("%f ", d);
        }
        printf("\n");
    }
}