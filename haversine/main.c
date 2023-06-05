
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

    Value *json = parseElement(&parser);

    Value *pairs = getMemberValueOfObject(json, "pairs");

    double sum = 0;
    size_t count = getElementCount(pairs);

    for (size_t elementIndex = 0; elementIndex < count; elementIndex++)
    {
        Value *pair = getElementOfArray(pairs, elementIndex);

        double x1 = getAsNumber(getMemberValueOfObject(pair, "x1"));
        double y1 = getAsNumber(getMemberValueOfObject(pair, "y1"));

        double x2 = getAsNumber(getMemberValueOfObject(pair, "x2"));
        double y2 = getAsNumber(getMemberValueOfObject(pair, "y2"));

        double distance = haversine(x1, y1, x2, y2, EARTH_RADIUS);

        sum += distance;
    }

    double average = sum / (double)count;

    printf("Average : %1.12f\n\n", average);

    return 0;
}