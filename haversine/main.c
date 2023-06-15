
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

#include "common.c"

#include "parser.c"

#ifdef _WIN32
#include "windows.h"
#include "intrin.h"
#endif

#ifdef _WIN32
uint64_t getOsTimeFrequency()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    return frequency.QuadPart;
}

uint64_t getOsTimeStamp()
{
    LARGE_INTEGER timestamp;

    QueryPerformanceCounter(&timestamp);

    return timestamp.QuadPart;
}

#endif

float getOsSecondsElapsed(uint64_t start, uint64_t frequency)
{
    uint64_t time = getOsTimeStamp();

    return ((float)time - (float)start) / (float)frequency;
}

double getAverageDistance(Value *json, Arena *arena)
{

    Value *pairs = getMemberValueOfObject(json, "pairs", arena);

    double sum = 0;
    size_t count = getElementCount(pairs);

    for (size_t elementIndex = 0; elementIndex < count; elementIndex++)
    {
        Value *pair = getElementOfArray(pairs, elementIndex);

        double x1 = getAsNumber(getMemberValueOfObject(pair, "x1", arena));
        double y1 = getAsNumber(getMemberValueOfObject(pair, "y1", arena));

        double x2 = getAsNumber(getMemberValueOfObject(pair, "x2", arena));
        double y2 = getAsNumber(getMemberValueOfObject(pair, "y2", arena));

        double distance = haversine(x1, y1, x2, y2, EARTH_RADIUS);

        sum += distance;
    }

    double average = sum / (double)count;

    return average;
}

uint64_t estimateCpuFrequency()
{
    uint64_t frequency = getOsTimeFrequency();
    uint64_t osStart = getOsTimeStamp();

    uint64_t osTicks;
    uint64_t cpuStart = __rdtsc();
    do
    {
        osTicks = getOsTimeStamp();
    } while ((osTicks - osStart) < frequency);
    uint64_t cpuTicks = __rdtsc() - cpuStart;

    // printf("Os frequency %llu\n", frequency);
    // printf("Os ticks %llu\n", osTicks - osStart);
    // printf("Cpu frequency %llu\n", cpuTicks);

    return cpuTicks;
}

int main(void)
{

    uint64_t start = __rdtsc();

    uint64_t cpuFrequency = estimateCpuFrequency();

#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    Arena arena = arenaInit();

    uint64_t setupEnd = __rdtsc();

    String text = readFileToString(JSON_PATH, &arena);

    uint64_t readEnd = __rdtsc();

    // printf("Json: %.*s", (int)text.size, text.data);
    //  printf("hi");

    Parser parser = initParser(text, &arena);

    Value *json = parseElement(&parser);

    uint64_t parseEnd = __rdtsc();

    // printElement(json, 2, 0);

    // printf("\n");

    double average = getAverageDistance(json, &arena);

    uint64_t mathEnd = __rdtsc();

    printf("Average : %1.12f\n\n", average);

    uint64_t printEnd = __rdtsc();

    float setupTime = ((float)setupEnd - (float)start) / (float)cpuFrequency;

    float readTime = ((float)readEnd - (float)setupEnd) / (float)cpuFrequency;

    float parseTime = ((float)parseEnd - (float)readEnd) / (float)cpuFrequency;

    float mathTime = ((float)mathEnd - (float)parseEnd) / (float)cpuFrequency;

    float printTime = ((float)printEnd - (float)mathEnd) / (float)cpuFrequency;

    float total = ((float)printEnd - (float)start) / (float)cpuFrequency;

    printf("Setup : %f\n\n", setupTime);

    printf("Read : %f\n\n", readTime);

    printf("Pare : %f\n\n", parseTime);

    printf("Math : %f\n\n", mathTime);

    printf("Print : %f\n\n", printTime);

    printf("Total : %f\n\n", total);

    return 0;
}