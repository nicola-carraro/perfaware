#include "stdio.h"

void avx512_zmm(void *buffer);

int main(void) {
    char buffer[64] = {0};
    printf("avx512_zmm\n");
    avx512_zmm(buffer);

    printf("end\n");
}
