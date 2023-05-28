#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "ctype.h"
#include "stdbool.h"
#include "stdlib.h"
#include "assert.h"
#include "stdarg.h"
#include "string.h"
#include "math.h"

#define JSON_PATH "datar/pairs.json"
#define DOUBLES_PATH "data/pairs.double"

#define UNIFORM_METHOD "uniform"
#define CLUSTER_METHOD "cluster"

#define CLUSTER_COUNT 16
#define EARTH_RADIUS 6371
#define CLUSTER_WIDTH 5
#define CLUSTER_HEIGHT 5
#define MIN_X -180.0
#define MAX_X 180.0
#define MIN_Y -90.0
#define MAX_Y 90.0

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

void die(char *file, const size_t line, char *message, ...)
{
    va_list args;
    va_start(args, message);
    printf("ERROR(%s:%zu): ", file, line);
    vprintf(message, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

void writeTextToFile(FILE *file, const char *path, const char *format, ...)
{
    va_list varargsList;

    va_start(varargsList, format);

    if (vfprintf(file, format, varargsList) < 0)
    {
        fclose(file);
        die(__FILE__, __LINE__, "Error while writing to %s", path);
        return;
    }
    va_end(varargsList);
}

void writeBinaryToFile(FILE *file, const char *path, void *data, size_t size, size_t count)
{
    if (fwrite(data, size, count, file) != count)
    {
        fclose(file);
        die(__FILE__, __LINE__, "Error while writing to %s", path);
        return;
    }
}

double randomX()
{
    double result = randomDouble(MIN_X, MAX_X);

    return result;
}

double randomY()
{
    double result = randomDouble(MIN_Y, MAX_Y);

    return result;
}

Cluster initializeCluster()
{
    Cluster cluster = {0};

    double x1 = randomX();
    double x2 = x1 + CLUSTER_WIDTH;

    if (x2 > MAX_X)
    {
        x2 = MIN_X + (x2 - MAX_X);
    }
    else if (x2 < MIN_X)
    {
        x2 = MAX_X - (MIN_X - x2);
    }

    double y1 = randomY();
    double y2 = y1 + CLUSTER_HEIGHT;

    if (y2 > MAX_Y)
    {
        y2 = MIN_Y + (y2 - MAX_Y);
    }
    else if (y2 < MIN_Y)
    {
        y2 = MAX_Y - (MIN_Y - y2);
    }

    cluster.minX = min(x1, x2);
    cluster.maxX = max(x1, x2);
    cluster.minY = min(y1, y2);
    cluster.maxY = max(y1, y2);

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

double haversine(double x1Degrees, double y1Degrees, double x2Degrees, double y2Degrees, double radius)
{

    double x1Radians = degreesToRadians(x1Degrees);
    double y1Radians = degreesToRadians(y1Degrees);
    double x2Radians = degreesToRadians(x2Degrees);
    double y2Radians = degreesToRadians(y2Degrees);

    double rootTerm = square(sin((y2Radians - y1Radians) / 2.0)) + cos(y1Radians) * cos(y2Radians) * square(sin((x2Radians - x1Radians) / 2.0));

    double result = 2.0 * radius * asin(sqrt(rootTerm));

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

    if (cStringsEqual(argv[1], CLUSTER_METHOD))
    {
        clustered = true;
    }
    else if (!cStringsEqual(argv[1], UNIFORM_METHOD))
    {
        printf("Invalid method (allowed : %s, %s): %s", UNIFORM_METHOD, CLUSTER_METHOD, argv[1]);
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

    FILE *jsonFile = fopen(JSON_PATH, "w");
    FILE *doublesFile = fopen(DOUBLES_PATH, "wb");

    if (!jsonFile)
    {
        die(__FILE__, __LINE__, "Error while opening %s", JSON_PATH);
    }

    if (!doublesFile)
    {
        die(__FILE__, __LINE__, "Error while opening %s", DOUBLES_PATH);
    }

    writeTextToFile(jsonFile, JSON_PATH, "{\n\t\"pairs\":[\n");

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
            int cluster1Index = rand() % CLUSTER_COUNT;
            int cluster2Index = rand() % CLUSTER_COUNT;
            Cluster cluster1 = clusters[cluster1Index];
            Cluster cluster2 = clusters[cluster2Index];

            x1 = randomDouble(cluster1.minX, cluster1.maxX);
            y1 = randomDouble(cluster1.minY, cluster1.maxY);
            x2 = randomDouble(cluster2.minX, cluster2.maxX);
            y2 = randomDouble(cluster2.minY, cluster2.maxY);
        }
        else
        {
            x1 = randomX();
            y1 = randomY();
            x2 = randomX();
            y2 = randomY();
        }

        double distance = haversine(x1, y1, x2, y2, EARTH_RADIUS);

        sum += distance;

        // printf("x1 : %f, y1 : %f, x2 : %f, y2 : %f\n", x1, y1, x2, y2);

        // printf("distance : %f,\n", distance);

        writeTextToFile(
            jsonFile,
            JSON_PATH,
            "\t\t{\"x1\":%1.12f, \"y1\":%1.12f, \"x2\":%1.12f, \"y2\":%1.12f}",
            x1,
            y1,
            x2,
            y2);

        writeBinaryToFile(
            doublesFile,
            DOUBLES_PATH,
            &x1,
            sizeof(double),
            1);

        writeBinaryToFile(
            doublesFile,
            DOUBLES_PATH,
            &y1,
            sizeof(double),
            1);

        writeBinaryToFile(
            doublesFile,
            DOUBLES_PATH,
            &x2,
            sizeof(double),
            1);

        writeBinaryToFile(
            doublesFile,
            DOUBLES_PATH,
            &y2,
            sizeof(double),
            1);

        if (pair < pairs - 1)
        {
            writeTextToFile(jsonFile, JSON_PATH, ",\n");
        }
    }

    double average = sum / (double)pairs;
    printf("Sum: %1.12f\n", sum);
    printf("Average: %1.12f\n", average);

    writeTextToFile(jsonFile, JSON_PATH, "\n\t]\n}");

    fclose(jsonFile);
    fclose(doublesFile);
}
