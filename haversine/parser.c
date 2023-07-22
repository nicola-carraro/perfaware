#ifndef PARSER_C
#define PARSER_C
#include "common.c"
#include "stdbool.h"
#include "stdint.h"
#include "profiler.c"

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

typedef struct
{
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

typedef struct
{
   String text;
   size_t offset;
   size_t line;
   size_t column;
   Arena *arena;
} Parser;

typedef enum
{
   ValueType_Object,
   ValueType_Array,
   ValueType_String,
   ValueType_Number,
   ValueType_True,
   ValueType_False,
   ValueType_Null
} ValueType;

typedef struct _Value Value;
typedef struct _Member Member;
typedef struct _Members Members;

#define MEMBERS_INITIAL_CAPACITY 10
typedef struct _Members
{
   size_t capacity;
   size_t count;
   Member *members;
} Members;

#define ELEMENTS_INITIAL_CAPACITY 10
typedef struct
{
   size_t capacity;
   size_t count;
   Value **elements;
} Elements;

typedef struct _Value
{
   ValueType type;
   union
   {
      String string;
      double number;
      Members *object;
      Elements *array;
   } payload;
} Value;

typedef struct _Member
{
   String key;
   Value *value;
} Member;

uint8_t peekByte(Parser *parser);

uint8_t peekBytePlusN(Parser *parser, size_t n);

bool hasNext(Parser *parser);

Value *parseElement(Parser *parser);

Members *initMembers(Arena *arena);

bool isCharacter(Parser *parser, char character);

void addMember(Members *members, Member member, Parser *parser);

void printValue(Value *value, size_t indentation, size_t indentationLevel);

void expectCharacter(Parser *parser, char character);

void addElement(Elements *elements, Value *element, Arena *arena);

void printSpace();

bool isHexDigit(Parser *parser);

Elements *initElements(Arena *arena);

void skipCodepoints(Parser *parser, size_t count);

void printString(String string);

Parser initParser(String text, Arena *arena)
{
   TIME_FUNCTION
   Parser parser = {0};
   parser.arena = arena;
   parser.text = text;

   STOP_COUNTER

   return parser;
}

uint32_t codePointFromSurrogatePair(uint16_t highSurrogate, uint16_t lowSurrogate)
{
   uint32_t highSurrogateTerm = (highSurrogate - 0xd800) * 0X400;
   uint32_t lowSurrogateTerm = lowSurrogate - 0xdc00;
   uint32_t result = (highSurrogateTerm + lowSurrogateTerm) + 0x10000;

   return result;
}

bool isWhitespace(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }

   uint8_t byte = parser->text.data.unsignedData[parser->offset];
   return byte == 0x20 || byte == 0x0a || byte == 0x0d || byte == 0x09;
}

bool isLiteral(Parser *parser, String literal)
{
   if (parser->text.size - parser->offset < literal.size)
   {
      return false;
   }

   for (size_t byteIndex = 0; byteIndex < literal.size; byteIndex++)
   {
      uint8_t byte = parser->text.data.unsignedData[parser->offset + byteIndex];

      if (byte != literal.data.unsignedData[byteIndex])
      {
         return false;
      }
   }

   return true;
}

bool isTrueLiteral(Parser *parser)
{
   return isLiteral(parser, trueLiteral);
}

bool isFalseLiteral(Parser *parser)
{
   return isLiteral(parser, falseLiteral);
}

bool isNullLiteral(Parser *parser)
{
   return isLiteral(parser, nullLiteral);
}

bool hasNext(Parser *parser)
{

   return parser->offset < parser->text.size;
}

void nextLine(Parser *parser)
{
   parser->line++;
   parser->column = 0;
}

uint32_t decodeUtf8Unchecked(String codepoint)
{
   assert(codepoint.data.unsignedData != NULL);
   assert(codepoint.size > 0 && codepoint.size < 5);

   uint8_t firstByte = codepoint.data.unsignedData[0];

   uint8_t fistBytePayload;

   switch (codepoint.size)
   {
   case 1:
   {
      fistBytePayload = firstByte;
   }
   break;
   case 2:
   {
      fistBytePayload = firstByte & 0x1f;
   }
   break;
   case 3:
   {
      fistBytePayload = firstByte & 0x0f;
   }
   break;
   default:
   {
      fistBytePayload = firstByte & 0x07;
   }
   break;
   }

   uint32_t result = fistBytePayload;

   for (size_t byteIndex = 1; byteIndex < codepoint.size; byteIndex++)
   {
      result = result << 6;
      uint8_t continuationByte = codepoint.data.unsignedData[byteIndex];
      uint8_t continuationBytePayload = continuationByte & 0x3f;
      result |= continuationBytePayload;
   }

   return result;
}

String
next(Parser *parser)
{

   assert(hasNext(parser));
   String result = {0};
   result.data.unsignedData = parser->text.data.unsignedData + parser->offset;

   uint8_t firstByte = peekByte(parser);

   bool isNewLine = firstByte == '\n';

   size_t byteCount = 0;

   if (firstByte <= 0x7f)
   {
      byteCount = 1;
   }
   else if (firstByte <= 0xdf)
   {
      byteCount = 2;
   }
   else if (firstByte <= 0xef)
   {
      byteCount = 3;
   }
   else if (firstByte <= 0xf7)
   {
      byteCount = 4;
   }
   else
   {
      die(__FILE__, __LINE__, 0, "invalid first byte in UTF-8 codepoint, found : found %#02X (%zu:%zu)", firstByte, parser->line, parser->column);
   }

   size_t remainingBytes = parser->text.size - parser->offset;

   if (remainingBytes < byteCount)
   {
      die(__FILE__, __LINE__, 0, "truncated UTF-8 codepoint, expected %zu bytes, but stream only has %zu left", byteCount, remainingBytes);
   }

   for (size_t byteIndex = 1; byteIndex < byteCount; byteIndex++)
   {
      uint8_t continuationByte = peekBytePlusN(parser, byteIndex);
      if (continuationByte < 0x80 || continuationByte > 0xbf)
      {
         die(__FILE__, __LINE__, 0, "invalid continuation byte in UTF-8 codepoint : found %#02X (%zu:%zu)", continuationByte, parser->line, parser->column);
      }
   }

   parser->offset += byteCount;

   result.size = byteCount;

   uint32_t codepoint = decodeUtf8Unchecked(result);

   if (codepoint > 0x10ffff)
   {
      die(__FILE__, __LINE__, 0, "invalid UTF-8 codepoint : found %#06X (%zu:%zu)", codepoint, parser->line, parser->column);
   }

   bool isOverlong = (codepoint < 0x0080 && result.size > 1) || (codepoint < 0x0800 && result.size > 2) || (codepoint < 0x10000 && result.size > 3);

   if (isOverlong)
   {
      die(__FILE__, __LINE__, 0, "overlong encoding of UTF-8 codepoint : %#06X encoded using %zu bytes (%zu:%zu)", codepoint, result.size, parser->line, parser->column);
   }

   if (isNewLine)
   {
      nextLine(parser);
   }
   else
   {
      parser->column++;
   }

   return result;
}

uint8_t peekByte(Parser *parser)
{
   uint8_t result = parser->text.data.unsignedData[parser->offset];

   return result;
}

uint8_t peekBytePlusN(Parser *parser, size_t n)
{
   uint8_t result = (parser->text.data.unsignedData + n)[parser->offset];

   return result;
}

void skipWhitespace(Parser *parser)
{
   char byte = peekByte(parser);
   while (isWhitespace(parser))
   {
      next(parser);
      byte = peekByte(parser);
   }
}

bool isDoubleQuotes(Parser *parser)
{
   return isCharacter(parser, DOUBLE_QUOTES);
}

bool isLeftBracket(Parser *parser)
{
   return isCharacter(parser, LEFT_BRACKET);
}

bool isRightBracket(Parser *parser)
{
   return isCharacter(parser, RIGHT_BRACKET);
}

bool isReverseSolidus(Parser *parser)
{
   return isCharacter(parser, REVERSE_SOLIDUS);
}

bool isControlCharacter(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }
   uint8_t byte = peekByte(parser);

   return byte < 0x20;
}

bool shoudBeEscaped(char character)
{
   for (size_t escapeIndex = 0; escapeIndex < MANDATORY_ESCAPES_COUNT; escapeIndex++)
   {
      if (mandatoryEscapes[escapeIndex].characterToEscape == character)
      {
         return true;
      }
   }

   return false;
}

bool isMandatoryCharacterEscape(Parser *parser)
{
   char byte = peekByte(parser);
   for (size_t escapeIndex = 0; escapeIndex < MANDATORY_ESCAPES_COUNT; escapeIndex++)
   {
      if (mandatoryEscapes[escapeIndex].escape == byte)
      {
         return true;
      }
   }

   return false;
}

bool isSolidus(Parser *parser)
{
   return isCharacter(parser, '/');
}

uint16_t decodeUnicodeEscape(Parser *parser)
{

   uint16_t result = 0;
   uint16_t power = 16 * 16 * 16;
   for (size_t byteIndex = 0; byteIndex < 4; byteIndex++)
   {
      uint16_t digitValue = 0;

      if (isHexDigit(parser))
      {
         char byte = peekByte(parser);
         if (byte >= '0' && byte <= '9')
         {
            digitValue = byte - '0';
         }
         else if (byte >= 'a' && byte <= 'f')
         {
            digitValue = byte - 'a' + 10;
         }
         else if (byte >= 'A' && byte <= 'F')
         {
            digitValue = byte - 'A' + 10;
         }
      }
      else if (!hasNext(parser))
      {
         die(__FILE__, __LINE__, 0, "Truncated unicode escape sequence expected hex, found end of fail (%zu:%zu)\n (%zu:%zu)\n", parser->line + 1, parser->column + 1);
      }
      else
      {
         String codepoint = next(parser);
         die(__FILE__, __LINE__, 0, "illegal character in unicode escape sequence: expected hex, found %.*s (%zu:%zu)\n (%zu:%zu)\n", codepoint.size, codepoint.data, parser->line + 1, parser->column + 1);
      }

      result += digitValue * power;

      power /= 16;

      next(parser);
   }

   return result;
}

void encodeCodepoint(uint32_t codepoint, String *output)
{

   if (codepoint <= 0x007f)
   {
      output->size = 1;
      strncpy(output->data.signedData, (char *)(&codepoint), 1);
   }
   else if (codepoint <= 0x07ff)
   {
      output->size = 2;
      uint8_t firstByte = (uint8_t)((codepoint >> 6U) | 0xc0U);
      uint8_t secondByte = (uint8_t)((codepoint & 0x3fU) | 0x80U);
      output->data.unsignedData[0] = firstByte;
      output->data.unsignedData[1] = secondByte;
   }
   else if (codepoint <= 0xffff)
   {
      output->size = 3;
      uint8_t firstByte = ((uint8_t)(codepoint >> 12U)) | 0xe0U;
      uint8_t secondByte = ((codepoint >> 6U) & 0x3fU) | 0x80U;
      uint8_t thirdByte = (codepoint & 0x3fU) | 0x80U;
      output->data.unsignedData[0] = firstByte;
      output->data.unsignedData[1] = secondByte;
      output->data.unsignedData[2] = thirdByte;
   }
   else
   {
      output->size = 4;
      uint8_t firstByte = ((uint8_t)(codepoint >> 18U)) | 0xf0U;
      uint8_t secondByte = ((codepoint >> 12U) & 0x3fU) | 0x80U;
      uint8_t thirdByte = ((codepoint >> 6U) & 0x3fU) | 0x80U;
      uint8_t fourthByte = (codepoint & 0x3fU) | 0x80U;
      output->data.unsignedData[0] = firstByte;
      output->data.unsignedData[1] = secondByte;
      output->data.unsignedData[2] = thirdByte;
      output->data.unsignedData[3] = fourthByte;
   }
}

bool isUnicodeEscapeStart(Parser *parser)
{
   return isCharacter(parser, UNICODE_ESCAPE_START);
}

uint32_t parseLowSurrogateEscape(uint16_t highSurrogate, Parser *parser)
{
   String nextCodepoint;
   if (!isReverseSolidus(parser))
   {
      nextCodepoint = next(parser);
      die(__FILE__, __LINE__, 0, "high surrogate %#04X, not followed by escape, found %.*s (%zu:%zu)\n", highSurrogate, nextCodepoint.size, nextCodepoint.data.signedData, parser->line + 1, parser->column + 1);
   }

   next(parser);

   if (!isUnicodeEscapeStart(parser))
   {
      nextCodepoint = next(parser);
      die(__FILE__, __LINE__, 0, "expected unicode escape after high surrogate %#04X, found \\%.*s (%zu:%zu)\n", highSurrogate, nextCodepoint.size, nextCodepoint.data.signedData, parser->line + 1, parser->column + 1);
   }

   next(parser);

   uint16_t lowSurrogate = decodeUnicodeEscape(parser);

   if (lowSurrogate < 0xdc00 || lowSurrogate > 0xdfff)
   {
      die(__FILE__, __LINE__, 0, "expected low surrogate after high surrogate %#04X, found %#04X (%zu:%zu)\n", highSurrogate, lowSurrogate, parser->line + 1, parser->column + 1);
   }

   uint32_t codepoint = codePointFromSurrogatePair(highSurrogate, lowSurrogate);

   return codepoint;
}

String parseString(Parser *parser)
{

   assert(parser != NULL);
   assert(parser->text.data.unsignedData != NULL);
   assert(hasNext(parser));
   expectCharacter(parser, DOUBLE_QUOTES);

   size_t stringFirstLine = parser->line;
   size_t stringFirstColumn = parser->column;
   String result = {0};

   next(parser);

   if (!hasNext(parser))
   {
      die(__FILE__, __LINE__, 0, "unclosed string: expected \"\"\", found end of file (%zu:%zu)\n", stringFirstLine + 1, stringFirstColumn + 1);
   }

   result.data.unsignedData = parser->text.data.unsignedData + parser->offset;

   String nextCodepoint = {0};
   nextCodepoint.size = 0;

   while (!isDoubleQuotes(parser))
   {

      if (!hasNext(parser))
      {
         die(__FILE__, __LINE__, 0, "unclosed string: expected \"\"\", found end of file (%zu:%zu)\n", stringFirstLine + 1, stringFirstColumn + 1);
      }

      nextCodepoint = next(parser);

      assert(nextCodepoint.size > 0);

      result.size += nextCodepoint.size;
      // printString(nextCodepoint);
      // printString(result);
      // printf("\n");
   }

   next(parser);

   return result;
}

bool isMinusSign(Parser *parser)
{
   return isCharacter(parser, '-');
}

bool isZero(Parser *parser)
{
   return isCharacter(parser, '0');
}

bool isDot(Parser *parser)
{
   return isCharacter(parser, '.');
}

bool isExponentStart(Parser *parser)
{
   return isCharacter(parser, 'e') || isCharacter(parser, 'E');
}

bool isOneNine(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }
   char byte = peekByte(parser);

   return byte >= '1' && byte <= '9';
}

bool isHexDigit(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }
   char byte = peekByte(parser);

   return (byte >= '0' && byte <= '9') || (byte >= 'A' && byte <= 'F') || (byte >= 'a' && byte <= 'f');
}

bool isDigit(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }

   return isZero(parser) || isOneNine(parser);
}

bool isNumberStart(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }

   return isDigit(parser) || isMinusSign(parser);
}

bool isLeftBrace(Parser *parser)
{
   return isCharacter(parser, LEFT_BRACE);
}

bool isRightBrace(Parser *parser)
{
   return isCharacter(parser, RIGHT_BRACE);
}

bool isColon(Parser *parser)
{
   return isCharacter(parser, COLON);
}

bool isCharacter(Parser *parser, char character)
{
   if (!hasNext(parser))
   {
      return false;
   }

   uint8_t byte = peekByte(parser);

   return byte == character;
}

size_t countDigits(Parser *parser)
{
   size_t result = 0;
   while (isDigit(parser))
   {
      next(parser);
      result++;
   }

   return result;
}

size_t countIntegerDigits(Parser *parser)
{
   size_t result = 0;

   if (isOneNine(parser))
   {
      result += countDigits(parser);
   }
   else if (isDigit(parser))
   {

      next(parser);
      result++;
   }

   return result;
}

double parseNumber(Parser *parser)
{
   assert(parser != NULL);
   assert(parser->text.data.unsignedData != NULL);
   assert(hasNext(parser));
   assert(isNumberStart(parser));

   char *numberStart = parser->text.data.signedData + parser->offset;

   char *numberEnd;

   double result = strtod(numberStart, &numberEnd);

   skipCodepoints(parser, numberEnd - numberStart);

   return result;
}

void expectCharacter(Parser *parser, char character)
{
   if (!hasNext(parser))
   {
      die(__FILE__, __LINE__, 0, "expected %c, found end of file (%zu:%zu)\n", character, parser->line + 1, parser->column + 1);
   }
   else if (!isCharacter(parser, character))
   {

      String nextCodepoint = next(parser);
      die(__FILE__, __LINE__, 0, "expected %c, found %.*s (%zu:%zu)\n", character, nextCodepoint.size, nextCodepoint.data, parser->line + 1, parser->column + 1);
   }
}

void expectColon(Parser *parser)
{
   expectCharacter(parser, COLON);
}

void expectComma(Parser *parser)
{
   expectCharacter(parser, COMMA);
}

Member parseMember(Parser *parser)
{

   skipWhitespace(parser);

   Member result = {0};

   result.key = parseString(parser);

   skipWhitespace(parser);

   expectColon(parser);

   next(parser);

   result.value = parseElement(parser);

   return result;
}

Members *parseObject(Parser *parser)
{
   assert(parser != NULL);
   assert(isLeftBrace(parser));

   Members *result = initMembers(parser->arena);

   next(parser);

   skipWhitespace(parser);

   if (!isRightBrace(parser))
   {
      Member member = parseMember(parser);
      addMember(result, member, parser);
   }

   while (!isRightBrace(parser))
   {
      expectComma(parser);
      next(parser);
      Member member = parseMember(parser);
      addMember(result, member, parser);
   }

   next(parser);

   return result;
}

Elements *parseArray(Parser *parser)
{
   TIME_FUNCTION
   assert(parser != NULL);
   assert(isLeftBracket(parser));
   next(parser);

   Elements *result = initElements(parser->arena);

   if (!isRightBracket(parser))
   {
      Value *element = parseElement(parser);
      addElement(result, element, parser->arena);
   }

   while (!isRightBracket(parser))
   {
      expectComma(parser);
      next(parser);
      Value *element = parseElement(parser);
      addElement(result, element, parser->arena);
   }

   next(parser);

   STOP_COUNTER
   return result;
}

void printString(String string)
{

   int bytesToPrint;
   if (string.size > INT_MAX)
   {
      bytesToPrint = INT_MAX;
   }
   else
   {
      bytesToPrint = (int)string.size;
   }
   printf("\"%.*s\"", bytesToPrint, string.data.signedData);
}

void printIndentation(size_t indentation, size_t indentationLevel)
{
   for (size_t space = 0; space < indentation * indentationLevel; space++)
   {
      printSpace();
   }
}

void printSpace()
{
   printf(" ");
}

void printNumber(double number)
{
   printf("%1.12f", number);
}

void printMember(Member member, size_t indentation, size_t indentationLevel)
{
   printIndentation(indentation, indentationLevel + 1);
   printString(member.key);
   printf(" : ");
   printValue(member.value, indentation, indentationLevel + 1);
}

void printObject(Members *object, size_t indentation, size_t indentationLevel)
{
   printf("{\n");
   for (size_t memberIndex = 0; memberIndex < object->count - 1; memberIndex++)
   {

      Member member = object->members[memberIndex];
      printMember(member, indentation, indentationLevel);
      printf(",\n");
   }

   if (object->count > 0)
   {
      Member member = object->members[object->count - 1];
      printMember(member, indentation, indentationLevel);
      printf("\n");
   }
   printIndentation(indentation, indentationLevel);
   printf("}");
}

void printElement(Value *element, size_t indentation, size_t indentationLevel)
{
   printIndentation(indentation, indentationLevel + 1);
   printValue(element, indentation, indentationLevel + 1);
}

void printArray(Elements *array, size_t indentation, size_t indentationLevel)
{
   printf("[\n");
   for (size_t elementIndex = 0; elementIndex < array->count - 1; elementIndex++)
   {

      Value *element = array->elements[elementIndex];
      printElement(element, indentation, indentationLevel);
      printf(",\n");
   }

   if (array->count > 0)
   {
      Value *element = array->elements[array->count - 1];
      printElement(element, indentation, indentationLevel);
      printf("\n");
   }
   printIndentation(indentation, indentationLevel);
   printf("]");
}

void printValue(Value *value, size_t indentation, size_t indentationLevel)
{

   switch (value->type)
   {
   case ValueType_String:
   {
      printString(value->payload.string);
   }
   break;
   case ValueType_Number:
   {
      printNumber(value->payload.number);
   }
   break;
   case ValueType_Object:
   {
      printObject(value->payload.object, indentation, indentationLevel);
   }
   break;
   case ValueType_Array:
   {
      printArray(value->payload.array, indentation, indentationLevel);
   }
   break;
   case ValueType_True:
   {
      printf(TRUE_LITERAL);
   }
   break;
   case ValueType_False:
   {
      printf(FALSE_LITERAL);
   }
   break;
   case ValueType_Null:
   {
      printf(NULL_LITERAL);
   }
   break;
   default:
   {
      die(__FILE__, __LINE__, 0, "unsupported value type %d", value->type);
   }
   }
}

Members *initMembers(Arena *arena)
{

   Members *result = arenaAllocate(arena, sizeof(Members));

   result->capacity = MEMBERS_INITIAL_CAPACITY;

   result->members = arenaAllocate(arena, result->capacity * sizeof(Member));

   result->count = 0;

   return result;
}

Elements *initElements(Arena *arena)
{

   Elements *result = arenaAllocate(arena, sizeof(Elements));

   result->capacity = ELEMENTS_INITIAL_CAPACITY;

   result->elements = arenaAllocate(arena, result->capacity * sizeof(*result->elements));

   result->count = 0;

   return result;
}

bool stringsEqual(String left, String right)
{
   if (left.size != right.size)
   {
      return false;
   }

   size_t size = left.size;

   for (size_t byteIndex = 0; byteIndex < size; byteIndex++)
   {

      if (left.data.unsignedData[byteIndex] != right.data.unsignedData[byteIndex])
      {
         return false;
      }
   }

   return true;
}

bool stringEqualsCstring(String string, char *cString)
{

   assert(cString != NULL);
   assert(string.data.unsignedData != NULL);
   for (size_t byteIndex = 0; byteIndex < string.size; byteIndex++)
   {

      char byte = cString[byteIndex];
      if (byte == '\0' || byte != string.data.unsignedData[byteIndex])
      {
         return false;
      }
   }

   char byte = cString[string.size];

   if (byte != '\0')
   {
      return false;
   }
   else
   {
      return true;
   }
}

Value *getMemberValueOfMembers(Members *members, String key)
{
   for (size_t memberIndex = 0; memberIndex < members->count; memberIndex++)
   {
      Member member = members->members[memberIndex];

      if (stringsEqual(member.key, key))
      {
         return member.value;
      }
   }

   return NULL;
}

bool hasKey(Members *members, String key)
{
   return getMemberValueOfMembers(members, key) != NULL;
}

void addMember(Members *members, Member member, Parser *parser)
{

   assert(members != NULL);

   if (hasKey(members, member.key))
   {
      die(__FILE__, __LINE__, 0, "duplicate key: \"%.*s\" (%zu:%zu)\n", member.key.size, member.key.data, parser->line + 1, parser->column + 1);
   }

   if (members->count >= members->capacity)
   {
      size_t newCapacity = members->capacity * 2;
      Member *newMembers = arenaAllocate(parser->arena, newCapacity * sizeof(Member));
      memcpy(newMembers, members->members, members->capacity * sizeof(Member));
      members->capacity = newCapacity;
      members->members = newMembers;
   }

   members->members[members->count] = member;
   members->count++;
}

void addElement(Elements *elements, Value *element, Arena *arena)
{

   assert(elements != NULL);

   if (elements->count >= elements->capacity)
   {
      size_t newCapacity = elements->capacity * 2;
      Value **newElements = arenaAllocate(arena, newCapacity * sizeof(*elements->elements));
      memcpy(newElements, elements->elements, elements->capacity * sizeof(*elements->elements));
      elements->capacity = newCapacity;
      elements->elements = newElements;
   }

   elements->elements[elements->count] = element;
   elements->count++;
}

void skipCodepoints(Parser *parser, size_t count)
{
   for (size_t codepointIndex = 0; codepointIndex < count; codepointIndex++)
   {
      next(parser);
   }
}

void escapeMandatory(char *source, char *dest)
{
   size_t readOffset = 0;
   size_t writeOffset = 0;

   char byte = source[readOffset];
   while (byte != '\0')
   {
      bool isEscape = false;

      for (size_t escapeIndex = 0; escapeIndex < MANDATORY_ESCAPES_COUNT; escapeIndex++)
      {
         JsonEscape escape = mandatoryEscapes[escapeIndex];
         if (escape.characterToEscape == byte)
         {
            dest[writeOffset] = REVERSE_SOLIDUS;
            dest[writeOffset + 1] = escape.escape;
            writeOffset += 2;
            isEscape = true;
         }
      }

      if (!isEscape)
      {
         dest[writeOffset] = source[readOffset];
         writeOffset++;
      }

      readOffset++;

      byte = source[readOffset];
   }

   dest[writeOffset] = '\0';
}

Value *getMemberValueOfObject(Value *object, char *key, Arena *arena)
{
   if (object->type != ValueType_Object)
   {
      die(__FILE__, __LINE__, 0, "Cannot get member of non-object value. trying to get %s", key);
   }

   size_t keySize = strlen(key);

   char *buffer = arenaAllocate(arena, (keySize * 2));

   escapeMandatory(key, buffer);

   for (size_t memberIndex = 0; memberIndex < object->payload.object->count; memberIndex++)
   {
      Member member = object->payload.object->members[memberIndex];

      if (stringEqualsCstring(member.key, buffer))
      {

         freeLastAllocation(arena);
         return member.value;
      }
   }

   die(__FILE__, __LINE__, 0, "No element named %s", key);

   return NULL;
}

double getAsNumber(Value *number)
{

   if (number->type != ValueType_Number)
   {
      die(__FILE__, __LINE__, 0, "Cannot get non-number as number");
   }

   return number->payload.number;
}

Value *getElementOfArray(Value *array, size_t index)
{
   if (array->type != ValueType_Array)
   {
      die(__FILE__, __LINE__, 0, "Cannot get element of non-array value");
   }

   if (array->payload.array->count > index)
   {
      return array->payload.array->elements[index];
   }
   else
   {
      die(__FILE__, __LINE__, 0, "Index out of bount %zu", index);
      return NULL;
   }
}

size_t getElementCount(Value *array)
{
   if (array->type != ValueType_Array)
   {
      die(__FILE__, __LINE__, 0, "Cannot get element count of non-array value");
   }

   return array->payload.array->count;
}

Value *parseElement(Parser *parser)
{
   assert(parser != NULL);
   Value *result = (Value *)arenaAllocate(parser->arena, sizeof(Value));
   skipWhitespace(parser);

   if (isDoubleQuotes(parser))
   {
      result->payload.string = parseString(parser);
      result->type = ValueType_String;
      // printString(result.payload.string);
   }
   else if (isNumberStart(parser))
   {
      result->payload.number = parseNumber(parser);
      result->type = ValueType_Number;
      // printf("%f", result->payload.number);
   }
   else if (isLeftBrace(parser))
   {
      result->payload.object = parseObject(parser);
      result->type = ValueType_Object;
   }
   else if (isLeftBracket(parser))
   {
      result->payload.array = parseArray(parser);
      result->type = ValueType_Array;
   }
   else if (isTrueLiteral(parser))
   {
      skipCodepoints(parser, trueLiteral.size);
      result->type = ValueType_True;
   }
   else if (isFalseLiteral(parser))
   {
      skipCodepoints(parser, falseLiteral.size);
      result->type = ValueType_False;
   }
   else if (isNullLiteral(parser))
   {
      skipCodepoints(parser, nullLiteral.size);
      result->type = ValueType_Null;
   }
   else
   {
      if (hasNext(parser))
      {
         String column = next(parser);
         die(__FILE__, __LINE__, 0, ", expected JSON value, found \"%.*s\" (%zu:%zu)\n", column.size, column.data, parser->line + 1, parser->column + 1);
      }
      {
         die(__FILE__, __LINE__, 0, "expected JSON value, found end of file(%zu:%zu)\n", parser->line + 1, parser->column + 1);
      }
   }

   skipWhitespace(parser);

   return result;
}

Value *parseJson(Parser *parser)
{
   TIME_FUNCTION
   Value *json = parseElement(parser);

   STOP_COUNTER

   return json;
}

#endif