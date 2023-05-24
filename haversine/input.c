#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "ctype.h"
#include "stdbool.h"
#include "stdlib.h"
#include "assert.h"
#include "stdarg.h"
#include "string.h"
#include "math.h"

#define OUTPUT_PATH "data/input.json"
#define CLUSTER_COUNT 16
#define EARTH_RADIUS 6372.8

typedef struct
{
    double minX;
    double maxX;
    double maxY;
    double minY;
} Cluster;

bool isDigits(char *cString)
{

    while (*cString != '\0')
    {
        if (!isdigit(*cString))
        {
            return false;
        }
        cString++;
    }

    return true;
}

double randomDouble(double min, double max)
{
    assert(min <= max);
    double range = max - min;
    int randomInt = rand();

    double result = min + ((range / (double)RAND_MAX) * randomInt);

    return result;
}

void die(const char *file, const size_t line, char *message, ...)
{
    va_list args;
    va_start(args, message);

    char errorMessage[256];
    printf(errorMessage, args);
    va_end(args);

    printf("ERROR(%s:%zu): %s\n", file, line, message);

    exit(EXIT_FAILURE);
}

void writeToFile(FILE *file, const char *fileName, const char *format, ...)
{

    if (fprintf(file, format) < 0)
    {
        die(__FILE__, __LINE__, "Error while writing to %s", fileName);
        return;
    }
}

double randomLatitude()
{
    return randomDouble(-90.0, 90.0);
}

double randomLongitude()
{
    return randomDouble(0.0, 180.0);
}

Cluster initializeCluster()
{
    Cluster cluster = {0};

    double x1 = randomLongitude();
    double x2 = randomLongitude();
    double y1 = randomLatitude();
    double y2 = randomLatitude();

    double minx = min(x1, x2);
    double maxx = max(x1, x2);
    double miny = min(y1, y2);
    double maxy = max(y1, y2);

    cluster.minX = minx;
    cluster.maxX = maxx;
    cluster.minY = miny;
    cluster.maxY = maxy;

    return cluster;
}

bool cStringsEqual(char *left, char *right)
{
    return strcmp(left, right) == 0;
}

double square(double n)
{
    return n * n;
}

double degreesToRadians(double degrees)
{
    return degrees * 0.01745329251994329577;
}

double haversine(double x1, double y1, double x2, double y2, double radius)
{

    double deltaX = degreesToRadians(x2 - x1);
    double deltaY = degreesToRadians(y2 - y1);
    y1 = degreesToRadians(y1);
    y2 = degreesToRadians(y2);

    double rootTerm = square(sin(deltaY / 2.0)) + cos(y1) * cos(y2) * square(sin(deltaX / 2.0));

    double result = 2 * radius * asin(sqrt(rootTerm));

    return result;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Usage: %s method pairs seed", argv[0]);
        return;
    }

    bool clustered = false;

    if (cStringsEqual(argv[1], "cluster"))
    {
        clustered = true;
    }
    else if (!cStringsEqual(argv[1], "random"))
    {
        printf("Invalid method (allowed : cluster, random): %s", argv[0]);
        return;
    }

    if (!isDigits(argv[2]))
    {
        printf("Error: pairs should be numeric, found: %s", argv[2]);
        return;
    }

    if (!isDigits(argv[3]))
    {
        printf("Error: seed should be numeric, found: %s", argv[3]);
        return;
    }

    size_t pairs = (size_t)atoi(argv[2]);

    int seed = atoi(argv[3]);

    printf("Pairs : %zu\n", pairs);
    printf("Seed  : %d\n", seed);

    srand(seed);

    FILE *file = fopen(OUTPUT_PATH, "w");

    if (!file)
    {
        printf("Error while opening %s", OUTPUT_PATH);
        return;
    }

    writeToFile(file, OUTPUT_PATH, "{\n\t\"pairs\":[\n");

    Cluster clusters[CLUSTER_COUNT];

    if (clustered)
    {
        for (size_t clusterIndex = 0; clusterIndex < CLUSTER_COUNT; clusterIndex++)
        {
            clusters[clusterIndex] = initializeCluster();
        }
    }

    double sum = 0.0;

    for (size_t pair = 0; pair < pairs; pair++)
    {

        double x1;
        double y1;
        double x2;
        double y2;

        if (clustered)
        {
            Cluster cluster = clusters[pair % CLUSTER_COUNT];
            x1 = randomDouble(cluster.minX, cluster.maxX);
            y1 = randomLatitude(cluster.minY, cluster.maxY);
            x2 = randomLongitude(cluster.minX, cluster.maxX);
            y2 = randomLatitude(cluster.minY, cluster.maxY);
        }
        else
        {
            x1 = randomLongitude();
            y1 = randomLatitude();
            x2 = randomLongitude();
            y2 = randomLatitude();
        }

        sum += haversine(x1, y1, x2, y2, EARTH_RADIUS);

        writeToFile(
            file,
            OUTPUT_PATH,
            "\t\t{\"x1\":%f, \"y1\":%f, \"x2\":%f, \"y2\":%f}",
            x1,
            y1,
            x2,
            y2);

        if (pair < pairs - 1)
        {
            writeToFile(file, OUTPUT_PATH, ",\n");
        }
    }

    double average = sum / pairs;

    printf("Average: %f", average);

    writeToFile(file, OUTPUT_PATH, "\n\t]\n}");
}
