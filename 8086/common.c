#include "stdlib.h"

size_t fileNameStart(const char *filePath)
{

    size_t index = 0;
    size_t result = 0;
    while (filePath[index] != '\0')
    {
        if (filePath[index] == '/' || filePath[index] == '\\')
        {
            result = index + 1;
        }
        index++;
    }

    return result;
}