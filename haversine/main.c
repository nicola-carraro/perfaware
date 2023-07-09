
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"

#include "common.c"

#include "parser.c"

void bar();

void foo();

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

void sleep(uint32_t milliseconds)
{
    Sleep(milliseconds);
}

#endif

float getOsSecondsElapsed(uint64_t start, uint64_t frequency)
{
    uint64_t time = getOsTimeStamp();

    return ((float)time - (float)start) / (float)frequency;
}

double getAverageDistance(Value *json, Arena *arena)
{
    TIME_FUNCTION
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

    STOP_COUNTER
    return average;
}

uint64_t estimateCpuCounterFrequency()
{
    uint64_t frequency = getOsTimeFrequency();

    uint64_t osTicks;
    uint64_t cpuStart = __rdtsc();
    uint64_t osStart = getOsTimeStamp();

    do
    {
        osTicks = getOsTimeStamp();
    } while ((osTicks - osStart) < frequency);
    uint64_t cpuTicks = __rdtsc() - cpuStart;

    /* printf("Os frequency %llu\n", frequency);
     printf("Os ticks %llu\n", osTicks - osStart);
     printf("Cpu frequency %llu\n", cpuTicks);*/

    return cpuTicks;
}

void sleepOneSecond()
{
    // TIME_FUNCTION

    sleep(1000);

    // STOP_COUNTER;
}

void foo()
{
    TIME_FUNCTION

    static bool secondTime = false;

    sleepOneSecond();

    if (!secondTime)
    {
        secondTime = true;
        bar();
    }

    STOP_COUNTER
}

void bar()
{
    TIME_FUNCTION
    sleepOneSecond();

    foo();

    STOP_COUNTER
}

int main(void)
{

    COUNTERS.cpuCounterFrequency = estimateCpuCounterFrequency();

    startCounters(&COUNTERS);
    // sleepOneSecond();

#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    Arena arena = arenaInit();

    String text = readFileToString(JSON_PATH, &arena);

    // printf("Json: %.*s", (int)text.size, text.data);
    //  printf("hi");

    Parser parser = initParser(text, &arena);

    Value *json = parseJson(&parser);

    // printElement(json, 2, 0);

    // printf("\n");

    double average = getAverageDistance(json, &arena);

    printf("Average : %1.12f\n\n", average);

    foo();

    //  sleepFiveSeconds();

    stopCounters(&COUNTERS);

    printPerformanceReport(&COUNTERS);

    return 0;
}