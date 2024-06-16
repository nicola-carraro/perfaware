#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "assert.h"
#include "math.h"
#include "stdint.h"
#include "assert.h"
#include "stdint.h"
#include "stdbool.h"

#pragma warning(push, 0)

#include "windows.h"
#include "psapi.h"

#pragma warning(pop)

#define COUNTER_NAME_CAPACITY 50

typedef struct {
    uint64_t start;
    size_t id;
    uint64_t initialTicksInRoot;
} Counter;

#ifdef PROFILE

#define MAX_COUNTERS 4096

#define READ_SIZE (4 * 1024 * 1024)

typedef struct {
    uint64_t totalTicks;
    uint64_t ticksInRoot;
    uint64_t childrenTicks;
    size_t calls;
    const char *name;
    size_t bytes;
} TimedBlock;

#endif

typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t cpuCounterFrequency;
#ifdef PROFILE
    TimedBlock timedBlocks[MAX_COUNTERS];
    size_t blocksCount;
    Counter stack[MAX_COUNTERS];
    size_t stackSize;
#endif
} Counters;

#ifdef PROFILE

void pushCounter(Counters *counters, size_t id, const char *name, size_t bytes) {
    assert(id != 0);
    size_t count = __rdtsc();

    TimedBlock *timedBlock = &(counters->timedBlocks[id]);
    timedBlock->bytes = bytes;
    if (counters->blocksCount <= id) {
        counters->blocksCount = id + 1;
    }

    if (timedBlock->name == 0) {
        timedBlock->name = name;
    }

    timedBlock->calls++;

    counters->stack[counters->stackSize] .id = id;
    counters->stack[counters->stackSize] .start = count;
    counters->stack[counters->stackSize] .initialTicksInRoot = timedBlock->ticksInRoot;
    counters->stackSize++;
}

void popCounter(Counters *counters) {
    size_t endTicks = __rdtsc();

    assert(counters->stackSize > 0);

    size_t startTicks = counters->stack[counters->stackSize - 1] .start;
    size_t id = counters->stack[counters->stackSize - 1] .id;
    uint64_t initialTicksInRoot = counters->stack[counters->stackSize - 1] .initialTicksInRoot;

    size_t elapsed = endTicks - startTicks;
    TimedBlock *timedBlock = &(counters->timedBlocks[id]);
    timedBlock->totalTicks += elapsed;
    timedBlock->ticksInRoot = initialTicksInRoot + elapsed;

    counters->stackSize--;

    if (counters->stackSize > 0) {
        size_t parentId = counters->stack[counters->stackSize - 1] .id;
        TimedBlock *parentBlock = &(counters->timedBlocks[parentId]);
        parentBlock->childrenTicks += elapsed;
    }
}

int compareTimedBlocks(const void *left, const void *right) {
    TimedBlock *leftTimedBlock = (TimedBlock *) (left);

    TimedBlock *rightTimedBlock = (TimedBlock *) (right);

    uint64_t leftTicksInRoot = leftTimedBlock->ticksInRoot;
    uint64_t rightTicksInRoot = rightTimedBlock->ticksInRoot;

    if (leftTicksInRoot > rightTicksInRoot) {
        return -1;
    }
    else if (leftTicksInRoot < rightTicksInRoot) {
        return 1;
    }
    else {
        return 0;
    }
}

void printTimedBlocksStats(Counters *counters, size_t totalCount) {
    assert(counters->stackSize == 0);

    float totalPercentage = 0.0f;

    char format[] = "%-25s (called %10zu times): \t\t with children: %20.10f (%14.10f %%) \t\t without children:  %20.10f (%14.10f %%) , Throughput: %4.10f 5 gB/S\n";

    qsort(
        (void *) (counters->timedBlocks),
        counters->blocksCount,
        sizeof(*counters->timedBlocks),
        compareTimedBlocks
    );
    for (size_t counterIndex = 1; counterIndex < counters->blocksCount; counterIndex++) {
        TimedBlock *timedBlock = &(counters->timedBlocks[counterIndex]);
        if (timedBlock->name != NULL) {
            uint64_t totalTicks = timedBlock->totalTicks;
            uint64_t ticksInRoot = timedBlock->ticksInRoot;
            uint64_t childrenTicks = timedBlock->childrenTicks;
            uint64_t ticksWithoutChildren = totalTicks - childrenTicks;
            float secondsWithChildren = ((float) (ticksInRoot)) / ((float) (counters->cpuCounterFrequency));
            float secondsWithoutChildren = ((float) (ticksWithoutChildren)) / ((float) (counters->cpuCounterFrequency));
            float percentageWithoutChildren = (((float) ticksWithoutChildren) / ((float) (totalCount))) * 100.0f;
            float percentageWithChildren = (((float) ticksInRoot) / ((float) (totalCount))) * 100.0f;
            float througput = 0.0f;

            if (timedBlock->bytes > 0) {
                througput = ((float) timedBlock->bytes / secondsWithChildren) / (1024 * 1024 * 1024);
            }

            totalPercentage += percentageWithoutChildren;
            printf(
                format,
                timedBlock->name,
                timedBlock->calls,
                secondsWithChildren,
                percentageWithChildren,
                secondsWithoutChildren,
                percentageWithoutChildren,
                througput
            );
        }
    }
    printf("Total percentage: %14.10f\n", totalPercentage);
}

#define MEASURE_THROUGHPUT(NAME, BYTES)\
{\
    pushCounter(&COUNTERS, (__COUNTER__ + 1), NAME, BYTES);\
}

#define MEASURE_FUNCTION_THROUGHPUT(BYTES)\
{\
    pushCounter(&COUNTERS, (__COUNTER__ + 1), __func__, BYTES);\
}

#define TIME_BLOCK(NAME)\
{\
    MEASURE_THROUGHPUT(NAME, 0);\
}

#define TIME_FUNCTION \
{\
    TIME_BLOCK(__func__);\
}

#define STOP_COUNTER \
{\
    popCounter(&COUNTERS);\
}

#else

#define TIME_BLOCK {}

#define MEASURE_THROUGHPUT(NAME, BYTES)\
{\
}

#define MEASURE_FUNCTION_THROUGHPUT(BYTES)\
{\
}

#define TIME_FUNCTION {}

#define STOP_COUNTER {}

#endif

static Counters COUNTERS = {0};

void startCounters(Counters *counters) {
    size_t count = __rdtsc();

    counters->start = count;
}

void stopCounters(Counters *counters) {
    size_t count = __rdtsc();

    counters->end = count;
}

void printPerformanceReport(Counters *counters) {
    size_t totalCount = counters->end - counters->start;

#ifdef PROFILE
    printTimedBlocksStats(counters, totalCount);
#endif

    float totalSeconds = ((float) (totalCount)) / ((float) (counters->cpuCounterFrequency));
    printf("\n");

    printf("Total time:       %14.10f\n", totalSeconds);
}

uint64_t getOsTimeFrequency() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    return frequency.QuadPart;
}

uint64_t getOsTimeStamp() {
    LARGE_INTEGER timestamp;

    QueryPerformanceCounter(&timestamp);

    return timestamp.QuadPart;
}

uint64_t estimateRdtscFrequency() {
    uint64_t frequency = getOsTimeFrequency();

    uint64_t osTicks;
    uint64_t cpuStart = __rdtsc();
    uint64_t osStart = getOsTimeStamp();

    do {
        osTicks = getOsTimeStamp();
    } while ((osTicks - osStart) < frequency);
    uint64_t cpuTicks = __rdtsc() - cpuStart;

    /*
       printf("Os frequency %llu\n", frequency);
       printf("Os ticks %llu\n", osTicks - osStart);
       printf("Cpu frequency %llu\n", cpuTicks);
    */
    return cpuTicks;
}


#define JSON_PATH "data/pairs.json"

#define ANSWERS_PATH "data/answers"

#define ARENA_SIZE 8 * 1024ll * 1024ll * 1024l

#define EARTH_RADIUS 6371

typedef struct {
    union {
        char *signedData;
        uint8_t *unsignedData;
    } data;
    size_t size;
} String;

typedef struct {
    void *memory;
    size_t size;
    size_t currentOffset;
    size_t previousOffset;
} Arena;

void die(const char *file, const size_t line, int errorNumber, const char *message, ...) {
    printf("ERROR (%s:%zu): ", file, line);

    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    if (errorNumber != 0) {
        char *cError = strerror(errorNumber);
        printf(" (%s)", cError);
    }

    exit(EXIT_FAILURE);
}

void writeTextToFile(FILE *file, const char *path, const char *format, ...) {
    va_list varargsList;

    va_start(varargsList, format);

    if (vfprintf(file, format, varargsList) < 0) {
        int error = errno;
        fclose(file);
        die(__FILE__, __LINE__, error, "could not write to %s", path);
    }
    va_end(varargsList);
}

void writeBinaryToFile(FILE *file, const char *path, void *data, size_t size, size_t count) {
    if (fwrite(data, size, count, file) != count) {
        int error = errno;
        fclose(file);
        die(__FILE__, __LINE__, error, "could not write to %s", path);
    }
}

size_t getFileSize(HANDLE *file) {
    TIME_FUNCTION;

    DWORD high = 0;
    DWORD low = GetFileSize(file, &high);

    if (low == INVALID_FILE_SIZE) {
        die(__FILE__, __LINE__, 0, "could not query the size of file");
    }

    LARGE_INTEGER result = {.LowPart = low, .HighPart = high};

    STOP_COUNTER;
    return result.QuadPart;
}

Arena arenaInit() {
    TIME_FUNCTION Arena arena = {0};

    arena.memory = VirtualAlloc(0, ARENA_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!arena.memory) {
        die(__FILE__, __LINE__, errno, "could not initialize arena");
    }

    arena.size = ARENA_SIZE;
    arena.currentOffset = 0;
    arena.previousOffset = 0;

    STOP_COUNTER return arena;
}

void arenaFreeAll(Arena *arena) {
    arena->currentOffset = 0;
    arena->previousOffset = 0;
}

void *arenaAllocate(Arena *arena, size_t size) {
    assert(arena != NULL);

    assert(arena->currentOffset + size < arena->size);

    void *result = (char *) arena->memory + arena->currentOffset;
    arena->previousOffset = arena->currentOffset;
    arena->currentOffset += size;

    return result;
}

void freeLastAllocation(Arena *arena) {
    arena->currentOffset = arena->previousOffset;
}

void readFileToString(HANDLE file, size_t size, char* buffer) {


    size_t totalBytesRead = 0;

    BOOL ok = TRUE;

    while (totalBytesRead < size) {
        DWORD bytesToRead = size <= MAXDWORD ? (DWORD) size : MAXDWORD;

        DWORD bytesRead = 0;

        ok = ReadFile(file, buffer, bytesToRead, &bytesRead, 0);
        totalBytesRead += bytesRead;
        buffer += bytesRead;
    }

    if (!ok) {
        die(__FILE__, __LINE__, errno, "could not read file");
    }

}

char *winErrorMessage() {
    char *result = NULL;

    DWORD error = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
        (LPTSTR) &result,
        0,
        NULL
    );

    return result;
}

uint64_t getPageFaultCount(HANDLE process) {
    uint64_t result = 0;
    PROCESS_MEMORY_COUNTERS memoryCounters = {0};

    if (GetProcessMemoryInfo(process, &memoryCounters, sizeof(memoryCounters))) {
        result = memoryCounters.PageFaultCount;
    }
    else {
        char *message = winErrorMessage();

        die(__FILE__, __LINE__, 0, "Failed to get page fault count: %s\n", message);
    }

    return result;
}

double degreesToRadians(double degrees) {
    return degrees * 0.01745329251994329577;
}

double square(double n) {
    return n *n;
}

double haversine(double x1Degrees, double y1Degrees, double x2Degrees, double y2Degrees, double radius) {
    double x1Radians = degreesToRadians(x1Degrees);
    double y1Radians = degreesToRadians(y1Degrees);
    double x2Radians = degreesToRadians(x2Degrees);
    double y2Radians = degreesToRadians(y2Degrees);

    double rootTerm = square(sin((y2Radians - y1Radians) / 2.0))
        + cos(y1Radians) * cos(y2Radians) * square(sin((x2Radians - x1Radians) / 2.0));

    double result = 2.0 * radius * asin(sqrt(rootTerm));

    return result;
}

#define DOUBLE_QUOTES '"'

#define REVERSE_SOLIDUS '\\'

#define LEFT_BRACE '{'

#define RIGHT_BRACE '}'

#define COLON ':'

#define COMMA ','

#define LEFT_BRACKET '['

#define RIGHT_BRACKET ']'

#define NULL_LITERAL "null"

#define TRUE_LITERAL "true"

#define FALSE_LITERAL "false"

#define UNICODE_ESCAPE_START 'u'

#define MANDATORY_ESCAPES_COUNT 7

typedef struct {
    char characterToEscape;
    char escape;
} JsonEscape;

JsonEscape mandatoryEscapes[MANDATORY_ESCAPES_COUNT] = {
    {'\\', '\\'},
    {'\b', 'b'},
    {'\f', 'f'},
    {'\n', 'n'},
    {'\r', 'r'},
    {'\t', 't'},
};

const String nullLiteral = {NULL_LITERAL, sizeof(NULL_LITERAL) - 1};

const String trueLiteral = {TRUE_LITERAL, sizeof(TRUE_LITERAL) - 1};

const String falseLiteral = {FALSE_LITERAL, sizeof(FALSE_LITERAL) - 1};

typedef struct {
    String text;
    size_t offset;
    size_t bytesRead;
    size_t line;
    size_t column;
    Arena *arena;
    HANDLE file;
} Parser;

typedef enum {
    ValueType_Object, ValueType_Array, ValueType_String, ValueType_Number, ValueType_True, ValueType_False, ValueType_Null
} ValueType;

typedef struct _Value Value;

typedef struct _Member Member;

typedef struct _Members Members;

#define MEMBERS_CAPACITY 4

typedef struct _Members {
    size_t capacity;
    size_t count;
    Member *members;
} Members;

typedef struct {
    size_t capacity;
    size_t count;
    Value **elements;
} Elements;

typedef struct _Value {
    ValueType type;
    union {
        String string;
        double number;
        Members *object;
        Elements *array;
    } payload;
} Value;

typedef struct _Member {
    String key;
    Value *value;
} Member;

uint8_t peekByte(Parser *parser);

uint8_t peekBytePlusN(Parser *parser, size_t n);

bool hasNext(Parser *parser);

Value *parseElement(Parser *parser);

Members *initMembers(Arena *arena);

bool isCharacter(Parser *parser, char character);

void addMember(Members *members, Member member);

void printValue(Value *value, size_t indentation, size_t indentationLevel);

void expectCharacter(Parser *parser, char character);

void addElement(Elements *elements, Value *element);

void printSpace();

bool isHexDigit(Parser *parser);

Elements *initElements(Arena *arena, size_t maxPairs);

void skipChars(Parser *parser, size_t count);

void printString(String string);

Parser initParser(HANDLE file, size_t size, Arena *arena) {
    TIME_FUNCTION Parser parser = {0};
    parser.file = file;
    parser.arena = arena;
    parser.text.data.signedData = arenaAllocate(arena, size);
    parser.text.size = size;
    STOP_COUNTER return parser;
}

uint32_t codePointFromSurrogatePair(uint16_t highSurrogate, uint16_t lowSurrogate) {
    uint32_t highSurrogateTerm = (highSurrogate - 0xd800) * 0X400;
    uint32_t lowSurrogateTerm = lowSurrogate - 0xdc00;
    uint32_t result = (highSurrogateTerm + lowSurrogateTerm) + 0x10000;

    return result;
}

bool isWhitespace(Parser *parser) {
    if (!hasNext(parser)) {
        return false;
    }

    uint8_t byte = parser->text.data.unsignedData[parser->offset];
    return byte == 0x20 || byte == 0x0a || byte == 0x0d || byte == 0x09;
}

bool isLiteral(Parser *parser, String literal) {
    if (parser->text.size - parser->offset < literal.size) {
        return false;
    }

    for (size_t byteIndex = 0; byteIndex < literal.size; byteIndex++) {
        uint8_t byte = parser->text.data.unsignedData[parser->offset + byteIndex];

        if (byte != literal.data.unsignedData[byteIndex]) {
            return false;
        }
    }

    return true;
}

bool isTrueLiteral(Parser *parser) {
    return isLiteral(parser, trueLiteral);
}

bool isFalseLiteral(Parser *parser) {
    return isLiteral(parser, falseLiteral);
}

bool isNullLiteral(Parser *parser) {
    return isLiteral(parser, nullLiteral);
}

bool hasNext(Parser *parser) {

  

    if(parser->offset >= parser->bytesRead){
        size_t remaining =  parser->text.size - parser->offset;
        size_t readSize = (READ_SIZE > remaining) ? remaining : READ_SIZE;
        readFileToString(parser->file, readSize, parser->text.data.signedData + parser->offset);
        parser->bytesRead += readSize;
    }

    return parser->offset < parser->text.size;

}

void nextLine(Parser *parser) {
    parser->line++;
    parser->column = 0;
}

uint32_t decodeUtf8Unchecked(String codepoint) {
    assert(codepoint.data.unsignedData != NULL);
    assert(codepoint.size > 0 && codepoint.size < 5);

    uint8_t firstByte = codepoint.data.unsignedData[0];

    uint8_t fistBytePayload;

    switch (codepoint.size) {
        case 1: {
            fistBytePayload = firstByte;
        }
        break;
        case 2: {
            fistBytePayload = firstByte & 0x1f;
        }
        break;
        case 3: {
            fistBytePayload = firstByte & 0x0f;
        }
        break;
        default: {
            fistBytePayload = firstByte & 0x07;
        }
        break;
    }

    uint32_t result = fistBytePayload;

    for (size_t byteIndex = 1; byteIndex < codepoint.size; byteIndex++) {
        result = result << 6;
        uint8_t continuationByte = codepoint.data.unsignedData[byteIndex];
        uint8_t continuationBytePayload = continuationByte & 0x3f;
        result |= continuationBytePayload;
    }

    return result;
}

char next(Parser *parser) {
    assert(hasNext(parser));

    char result = (parser->text.data.signedData + parser->offset)[0];

    bool isNewLine = result == '\n';

    if (isNewLine) {
        nextLine(parser);
    }
    else {
        parser->column++;
    }

    parser->offset++;

    return result;
}

uint8_t peekByte(Parser *parser) {
    uint8_t result = parser->text.data.unsignedData[parser->offset];

    return result;
}

uint8_t peekBytePlusN(Parser *parser, size_t n) {
    uint8_t result = (parser->text.data.unsignedData + n)[parser->offset];

    return result;
}

void skipWhitespace(Parser *parser) {
    char byte = peekByte(parser);
    while (isWhitespace(parser)) {
        next(parser);
        byte = peekByte(parser);
    }
}

bool isDoubleQuotes(Parser *parser) {
    return isCharacter(parser, DOUBLE_QUOTES);
}

bool isLeftBracket(Parser *parser) {
    return isCharacter(parser, LEFT_BRACKET);
}

bool isRightBracket(Parser *parser) {
    return isCharacter(parser, RIGHT_BRACKET);
}

bool isReverseSolidus(Parser *parser) {
    return isCharacter(parser, REVERSE_SOLIDUS);
}

String parseString(Parser *parser) {
    TIME_FUNCTION assert(parser != NULL);
    assert(parser->text.data.unsignedData != NULL);
    assert(hasNext(parser));

    String result = {0};

    next(parser);

    result.data.unsignedData = parser->text.data.unsignedData + parser->offset;

    while (!isDoubleQuotes(parser)) {
        next(parser);

        result.size++;
    }

    next(parser);

    STOP_COUNTER return result;
}

bool isMinusSign(Parser *parser) {
    return isCharacter(parser, '-');
}

bool isZero(Parser *parser) {
    return isCharacter(parser, '0');
}

bool isDot(Parser *parser) {
    return isCharacter(parser, '.');
}

bool isExponentStart(Parser *parser) {
    return isCharacter(parser, 'e') || isCharacter(parser, 'E');
}

bool isOneNine(Parser *parser) {
    if (!hasNext(parser)) {
        return false;
    }
    char byte = peekByte(parser);

    return byte >= '1' && byte <= '9';
}

bool isHexDigit(Parser *parser) {
    if (!hasNext(parser)) {
        return false;
    }
    char byte = peekByte(parser);

    return (byte >= '0' && byte <= '9') || (byte >= 'A' && byte <= 'F') || (byte >= 'a' && byte <= 'f');
}

bool isDigit(Parser *parser) {
    if (!hasNext(parser)) {
        return false;
    }

    return isZero(parser) || isOneNine(parser);
}

bool isNumberStart(Parser *parser) {
    if (!hasNext(parser)) {
        return false;
    }

    return isDigit(parser) || isMinusSign(parser);
}

bool isLeftBrace(Parser *parser) {
    return isCharacter(parser, LEFT_BRACE);
}

bool isRightBrace(Parser *parser) {
    return isCharacter(parser, RIGHT_BRACE);
}

bool isColon(Parser *parser) {
    return isCharacter(parser, COLON);
}

bool isCharacter(Parser *parser, char character) {
    if (!hasNext(parser)) {
        return false;
    }

    uint8_t byte = peekByte(parser);

    return byte == character;
}

size_t countDigits(Parser *parser) {
    size_t result = 0;
    while (isDigit(parser)) {
        next(parser);
        result++;
    }

    return result;
}

size_t countIntegerDigits(Parser *parser) {
    size_t result = 0;

    if (isOneNine(parser)) {
        result += countDigits(parser);
    }
    else if (isDigit(parser)) {
        next(parser);
        result++;
    }

    return result;
}

double parseNumberWithStrtod(Parser *parser) {
    char *numberStart = parser->text.data.signedData + parser->offset;

    char *numberEnd;

    TIME_BLOCK("strtod");
    double result = strtod(numberStart, &numberEnd);
    STOP_COUNTER skipChars(parser, numberEnd - numberStart);

    return result;
}

double parseDigit(Parser *parser) {
    char digit = next(parser);

    double result = (double) (digit - '0');

    return result;
}

double parseNumberByHand(Parser *parser) {
    // TIME_FUNCTION
    bool isNegative = false;

    if (isMinusSign(parser)) {
        isNegative = true;
        next(parser);
    }

    double integerValue = parseDigit(parser);

    while (!isDot(parser)) {
        integerValue *= 10.0;
        double digitValue = parseDigit(parser);
        integerValue += digitValue;
    }

    next(parser);

    double multiplier = 0.1;
    double fractionValue = 0.0;
    while (isDigit(parser)) {
        double digitValue = parseDigit(parser);
        fractionValue += digitValue * multiplier;
        multiplier /= 10.0;
    }

    double result = integerValue + fractionValue;

    if (isNegative) {
        result = -result;
    }

    // STOP_COUNTER;
    return result;
}

double parseNumber(Parser *parser) {
    assert(parser != NULL);
    assert(parser->text.data.unsignedData != NULL);
    assert(hasNext(parser));
    assert(isNumberStart(parser));

#if 0
    double result = parseNumberWithStrtod(parser);
#else
    double result = parseNumberByHand(parser);
#endif
    return result;
}

Member parseMember(Parser *parser) {
    skipWhitespace(parser);

    Member result = {0};

    result.key = parseString(parser);

    skipWhitespace(parser);

    next(parser);

    result.value = parseElement(parser);

    return result;
}

Members *parseObject(Parser *parser) {
    assert(parser != NULL);
    assert(isLeftBrace(parser));

    Members *result = initMembers(parser->arena);

    next(parser);

    skipWhitespace(parser);

    if (!isRightBrace(parser)) {
        Member member = parseMember(parser);
        addMember(result, member);
    }

    while (!isRightBrace(parser)) {
        next(parser);
        Member member = parseMember(parser);
        addMember(result, member);
    }

    next(parser);

    return result;
}

Elements *parseArray(Parser *parser) {
    TIME_FUNCTION assert(parser != NULL);
    assert(isLeftBracket(parser));
    next(parser);

    const size_t pairMaxSize = 14 * 4;

    size_t maxPairs = parser->text.size / pairMaxSize;

    Elements *result = initElements(parser->arena, maxPairs);

    if (!isRightBracket(parser)) {
        Value *element = parseElement(parser);
        addElement(result, element);
    }

    while (!isRightBracket(parser)) {
        next(parser);
        Value *element = parseElement(parser);
        addElement(result, element);
    }

    next(parser);

    STOP_COUNTER return result;
}

void printString(String string) {
    int bytesToPrint;
    if (string.size > INT_MAX) {
        bytesToPrint = INT_MAX;
    }
    else {
        bytesToPrint = (int) string.size;
    }
    printf("\"%.*s\"", bytesToPrint, string.data.signedData);
}

void printIndentation(size_t indentation, size_t indentationLevel) {
    for (size_t space = 0; space < indentation *indentationLevel; space++) {
        printSpace();
    }
}

void printSpace() {
    printf(" ");
}

void printNumber(double number) {
    printf("%1.12f", number);
}

void printMember(Member member, size_t indentation, size_t indentationLevel) {
    printIndentation(indentation, indentationLevel + 1);
    printString(member.key);
    printf(" : ");
    printValue(member.value, indentation, indentationLevel + 1);
}

void printObject(Members *object, size_t indentation, size_t indentationLevel) {
    printf("{\n");
    for (size_t memberIndex = 0; memberIndex < object->count - 1; memberIndex++) {
        Member member = object->members[memberIndex];
        printMember(member, indentation, indentationLevel);
        printf(",\n");
    }

    if (object->count > 0) {
        Member member = object->members[object->count - 1];
        printMember(member, indentation, indentationLevel);
        printf("\n");
    }
    printIndentation(indentation, indentationLevel);
    printf("}");
}

void printElement(Value *element, size_t indentation, size_t indentationLevel) {
    printIndentation(indentation, indentationLevel + 1);
    printValue(element, indentation, indentationLevel + 1);
}

void printArray(Elements *array, size_t indentation, size_t indentationLevel) {
    printf("[\n");
    for (size_t elementIndex = 0; elementIndex < array->count - 1; elementIndex++) {
        Value *element = array->elements[elementIndex];
        printElement(element, indentation, indentationLevel);
        printf(",\n");
    }

    if (array->count > 0) {
        Value *element = array->elements[array->count - 1];
        printElement(element, indentation, indentationLevel);
        printf("\n");
    }
    printIndentation(indentation, indentationLevel);
    printf("]");
}

void printValue(Value *value, size_t indentation, size_t indentationLevel) {
    switch (value->type) {
        case ValueType_String: {
            printString(value->payload.string);
        }
        break;
        case ValueType_Number: {
            printNumber(value->payload.number);
        }
        break;
        case ValueType_Object: {
            printObject(value->payload.object, indentation, indentationLevel);
        }
        break;
        case ValueType_Array: {
            printArray(value->payload.array, indentation, indentationLevel);
        }
        break;
        case ValueType_True: {
            printf(TRUE_LITERAL);
        }
        break;
        case ValueType_False: {
            printf(FALSE_LITERAL);
        }
        break;
        case ValueType_Null: {
            printf(NULL_LITERAL);
        }
        break;
        default: {
            assert(false);
        }
    }
}

Members *initMembers(Arena *arena) {
    Members *result = arenaAllocate(arena, sizeof(Members));

    result->capacity = MEMBERS_CAPACITY;

    result->members = arenaAllocate(arena, result->capacity *sizeof(Member));

    result->count = 0;

    return result;
}

Elements *initElements(Arena *arena, size_t maxPairs) {
    Elements *result = arenaAllocate(arena, sizeof(Elements));

    result->capacity = maxPairs;

    result->elements = arenaAllocate(arena, result->capacity *sizeof(*result->elements));

    result->count = 0;

    return result;
}

bool stringsEqual(String left, String right) {
    if (left.size != right.size) {
        return false;
    }

    size_t size = left.size;

    for (size_t byteIndex = 0; byteIndex < size; byteIndex++) {
        if (left.data.unsignedData[byteIndex] != right.data.unsignedData[byteIndex]) {
            return false;
        }
    }

    return true;
}

bool stringEqualsCstring(String string, char *cString) {
    assert(cString != NULL);
    assert(string.data.unsignedData != NULL);
    for (size_t byteIndex = 0; byteIndex < string.size; byteIndex++) {
        char byte = cString[byteIndex];
        if (byte == '\0' || byte != string.data.unsignedData[byteIndex]) {
            return false;
        }
    }

    char byte = cString[string.size];

    if (byte != '\0') {
        return false;
    }
    else {
        return true;
    }
}

Value *getMemberValueOfMembers(Members *members, String key) {
    for (size_t memberIndex = 0; memberIndex < members->count; memberIndex++) {
        Member member = members->members[memberIndex];

        if (stringsEqual(member.key, key)) {
            return member.value;
        }
    }

    return NULL;
}

bool hasKey(Members *members, String key) {
    return getMemberValueOfMembers(members, key) != NULL;
}

void addMember(Members *members, Member member) {
    assert(members != NULL);

    assert(members->count < members->capacity);

    members->members[members->count] = member;
    members->count++;
}

void addElement(Elements *elements, Value *element) {
    assert(elements != NULL);

    assert(elements->count < elements->capacity);

    elements->elements[elements->count] = element;
    elements->count++;
}

void skipChars(Parser *parser, size_t count) {
    for (size_t codepointIndex = 0; codepointIndex < count; codepointIndex++) {
        next(parser);
    }
}

void escapeMandatory(char *source, char *dest) {
    size_t readOffset = 0;
    size_t writeOffset = 0;

    char byte = source[readOffset];
    while (byte != '\0') {
        bool isEscape = false;

        for (size_t escapeIndex = 0; escapeIndex < MANDATORY_ESCAPES_COUNT; escapeIndex++) {
            JsonEscape escape = mandatoryEscapes[escapeIndex];
            if (escape.characterToEscape == byte) {
                dest[writeOffset] = REVERSE_SOLIDUS;
                dest[writeOffset + 1] = escape.escape;
                writeOffset += 2;
                isEscape = true;
            }
        }

        if (!isEscape) {
            dest[writeOffset] = source[readOffset];
            writeOffset++;
        }

        readOffset++;

        byte = source[readOffset];
    }

    dest[writeOffset] = '\0';
}

Value *getMemberValueOfObject(Value *object, char *key) {
    Value *result = NULL;

    for (size_t memberIndex = 0; memberIndex < object->payload.object->count; memberIndex++) {
        Member member = object->payload.object->members[memberIndex];

        if (stringEqualsCstring(member.key, key)) {
            result = member.value;
            break;
        }
    }

    return result;
}

double getAsNumber(Value *number) {
    assert(number->type == ValueType_Number);

    return number->payload.number;
}

Value *getElementOfArray(Value *array, size_t index) {
    Value *result = NULL;
    assert(array->type == ValueType_Array);

    assert(array->payload.array->count > index);

    result = array->payload.array->elements[index];

    return result;
}

size_t getElementCount(Value *array) {
    assert(array->type == ValueType_Array);

    return array->payload.array->count;
}

Value *parseElement(Parser *parser) {
    assert(parser != NULL);
    Value *result = (Value *) arenaAllocate(parser->arena, sizeof(Value));
    skipWhitespace(parser);

    if (isDoubleQuotes(parser)) {
        result->payload.string = parseString(parser);
        result->type = ValueType_String;
        // printString(result.payload.string);
    }
    else if (isNumberStart(parser)) {
        result->payload.number = parseNumber(parser);
        result->type = ValueType_Number;
        // printf("%f", result->payload.number);
    }
    else if (isLeftBrace(parser)) {
        result->payload.object = parseObject(parser);
        result->type = ValueType_Object;
    }
    else if (isLeftBracket(parser)) {
        result->payload.array = parseArray(parser);
        result->type = ValueType_Array;
    }
    else if (isTrueLiteral(parser)) {
        skipChars(parser, trueLiteral.size);
        result->type = ValueType_True;
    }
    else if (isFalseLiteral(parser)) {
        skipChars(parser, falseLiteral.size);
        result->type = ValueType_False;
    }
    else if (isNullLiteral(parser)) {
        skipChars(parser, nullLiteral.size);
        result->type = ValueType_Null;
    }
    else {
        assert(false);
    }

    skipWhitespace(parser);

    return result;
}

Value *parseJson(Parser *parser) {
    TIME_FUNCTION Value *json = parseElement(parser);

    STOP_COUNTER return json;
}

void bar();

void foo();

void sleep(uint32_t milliseconds) {
    Sleep(milliseconds);
}

float getOsSecondsElapsed(uint64_t start, uint64_t frequency) {
    uint64_t time = getOsTimeStamp();

    return ((float) time - (float) start) / (float) frequency;
}

double getAverageDistance(Value *json) {
    Value *pairs = getMemberValueOfObject(json, "pairs");

    double sum = 0;
    size_t count = getElementCount(pairs);

    MEASURE_THROUGHPUT("pairsLoop", count *4 *sizeof(double));

    for (size_t elementIndex = 0; elementIndex < count; elementIndex++) {
        Value *pair = getElementOfArray(pairs, elementIndex);

        double x1 = getAsNumber(getMemberValueOfObject(pair, "x1"));
        double y1 = getAsNumber(getMemberValueOfObject(pair, "y1"));

        double x2 = getAsNumber(getMemberValueOfObject(pair, "x2"));
        double y2 = getAsNumber(getMemberValueOfObject(pair, "y2"));

        double distance = haversine(x1, y1, x2, y2, EARTH_RADIUS);

        sum += distance;
    }
    double average = sum / (double) count;

    STOP_COUNTER return average;
}

void sleepOneSecond() {
    // TIME_FUNCTION
    sleep(1000);

    // STOP_COUNTER;
}

void foo() {
    TIME_FUNCTION static bool secondTime = false;

    sleepOneSecond();

    if (!secondTime) {
        secondTime = true;
        bar();
    }

    STOP_COUNTER
}

void bar() {
    TIME_FUNCTION sleepOneSecond();

    foo();

    STOP_COUNTER
}

int main(void) {
    COUNTERS.cpuCounterFrequency = estimateRdtscFrequency();

    startCounters(&COUNTERS);
    // sleepOneSecond();
    SetConsoleOutputCP(65001);

    Arena arena = arenaInit();

    HANDLE file = CreateFile(JSON_PATH, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    size_t size = getFileSize(file);


    if (file == INVALID_HANDLE_VALUE) {
        die(__FILE__, __LINE__, errno, "could not open %s", JSON_PATH);
    }



    // printf("Json: %.*s", (int)text.size, text.data);
    // printf("hi");
    Parser parser = initParser(file, size, &arena);

    Value *json = parseJson(&parser);

    bool  ok = CloseHandle(file);


    if (!ok) {
        die(__FILE__, __LINE__, errno, "could not close %s", JSON_PATH);
    }

    // printElement(json, 2, 0);
    // printf("\n");
    double average = getAverageDistance(json);

    printf("Average : %1.12f\n\n", average);

    // foo();
    // sleepFiveSeconds();
    stopCounters(&COUNTERS);

    printPerformanceReport(&COUNTERS);

    return 0;
}
