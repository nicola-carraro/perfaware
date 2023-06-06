
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

#include "common.c"

#include "parser.c"

#ifdef _WIN32
#include "windows.h"
#endif

void printAverageDistance(Value *json)
{
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
}

int main(void)
{

#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    Arena arena = arenaInit();

    String text = readFileToString(JSON_PATH, &arena);

    // printf("Json: %.*s", (int)text.size, text.data);
    //  printf("hi");

    Parser parser = initParser(text, &arena);

    Value *json = parseElement(&parser);

    printElement(json, 2, 0);

    printf("\n");

    printAverageDistance(json);

    return 0;
}