#ifndef PARSER_C
#define PARSER_C
#include "common.c"
#include "stdbool.h"
#include "stdint.h"

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

#define MEMBERS_INITIAL_CAPACITY 100
typedef struct _Members
{
   size_t capacity;
   size_t count;
   Member *members;
} Members;

#define ELEMENTS_INITIAL_CAPACITY 100
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

Elements *initElements(Arena *arena);

Parser initParser(String text, Arena *arena)
{
   Parser parser = {0};
   parser.arena = arena;
   parser.text = text;

   return parser;
}

bool isWhitespace(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }

   char byte = parser->text.data[parser->offset];
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
      char byte = parser->text.data[parser->offset + byteIndex];

      if (byte != literal.data[byteIndex])
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

String next(Parser *parser)
{

   assert(hasNext(parser));
   String result = {0};
   result.data = parser->text.data + parser->offset;

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
         die(__FILE__, __LINE__, 0, "invalid continuation byte in UTF-8 codepoint, found : found %#02X (%zu:%zu)", continuationByte, parser->line, parser->column);
      }
   }

   if (isNewLine)
   {
      nextLine(parser);
   }
   else
   {
      parser->column++;
   }

   parser->offset += byteCount;

   result.size = byteCount;

   return result;
}

uint8_t peekByte(Parser *parser)
{
   uint8_t result = ((uint8_t *)(parser->text.data))[parser->offset];

   return result;
}

uint8_t peekBytePlusN(Parser *parser, size_t n)
{
   uint8_t result = ((uint8_t *)(parser->text.data + n))[parser->offset];

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

String parseString(Parser *parser)
{
   assert(parser != NULL);
   assert(parser->text.data != NULL);
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

   result.data = parser->text.data + parser->offset;

   while (!isDoubleQuotes(parser))
   {

      if (!hasNext(parser))
      {
         die(__FILE__, __LINE__, 0, "unclosed string: expected \"\"\", found end of file (%zu:%zu)\n", stringFirstLine + 1, stringFirstColumn + 1);
      }

      if (isControlCharacter(parser))
      {
         uint8_t illegalCharacter = peekByte(parser);
         die(__FILE__, __LINE__, 0, "illegal control character in string: found %#02X (%zu:%zu)\n", illegalCharacter, parser->line + 1, parser->column + 1);
      }

      if (isReverseSolidus(parser))
      {
         die(__FILE__, __LINE__, 0, "escape sequences in strings not implemented (%zu:%zu)\n", parser->line + 1, parser->column + 1);
      }

      String codepoint = next(parser);

      result.size += codepoint.size;

      assert(codepoint.size > 0);
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

   return (byte >= 'A' && byte <= 'F') || (byte >= 'a' && byte <= 'f');
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
   assert(parser->text.data != NULL);
   assert(hasNext(parser));
   assert(isNumberStart(parser));

   char *numberStart = parser->text.data + parser->offset;

   size_t offset = 0;

   if (isMinusSign(parser))
   {
      next(parser);
      offset++;
   }

   size_t integerDigits = countIntegerDigits(parser);

   if (integerDigits == 0)
   {
      uint8_t byte = peekByte(parser);
      die(__FILE__, __LINE__, 0, "expected digit, found %c (%zu:%zu)\n", byte, parser->line + 1, parser->column + 1);
   }
   else
   {
      offset += integerDigits;

      if (isDot(parser))
      {
         next(parser);
         offset++;
         size_t fractionDigits = countDigits(parser);
         offset += fractionDigits;
      }

      if (isExponentStart(parser))
      {
         bool hasMinusSign = false;
         if (isMinusSign(parser))
         {
            next(parser);
            offset++;
            hasMinusSign = true;
         }
         next(parser);
         offset++;
         size_t exponentDigits = countDigits(parser);

         if (hasMinusSign && exponentDigits == 0)
         {
            uint8_t byte = peekByte(parser);
            die(__FILE__, __LINE__, 0, "expected digit, found %c (%zu:%zu)\n", byte, parser->line + 1, parser->column + 1);
         }

         offset += exponentDigits;
      }
   }

   char *buffer = arenaAllocate(parser->arena, offset + 1);

   buffer[offset] = '\0';

   strncpy(buffer, numberStart, offset);

   char *end;

   double result = strtod(buffer, &end);

   if (offset <= PTRDIFF_MAX)
   {
      assert(end - buffer > 0);
      assert((end - buffer) == (ptrdiff_t)offset);
   }

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
      die(__FILE__, __LINE__, 0, "expected %c, found %.*s (%zu:%zu)\n", character, nextCodepoint.data, parser->line + 1, parser->column + 1);
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
   printf("\"%.*s\"", bytesToPrint, string.data);
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

      if (left.data[byteIndex] != right.data[byteIndex])
      {
         return false;
      }
   }

   return true;
}

bool stringEqualsCstring(String string, char *cString)
{

   assert(cString != NULL);
   assert(string.data != NULL);
   for (size_t byteIndex = 0; byteIndex < string.size; byteIndex++)
   {

      char byte = cString[byteIndex];
      if (byte == '\0' || byte != string.data[byteIndex])
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
      size_t newCapacity = members->capacity * 10;
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
      size_t newCapacity = elements->capacity * 10;
      Value **newElements = arenaAllocate(arena, newCapacity * sizeof(*elements->elements));
      memcpy(newElements, elements->elements, elements->capacity * sizeof(*elements->elements));
      elements->capacity = newCapacity;
      elements->elements = newElements;
   }

   elements->elements[elements->count] = element;
   elements->count++;
}

void skipCharacters(Parser *parser, size_t count)
{
   parser->offset += count;
}

Value *getMemberValueOfObject(Value *object, char *key)
{
   if (object->type != ValueType_Object)
   {
      die(__FILE__, __LINE__, 0, "Cannot get member of non-object value. trying to get %s", key);
   }

   for (size_t memberIndex = 0; memberIndex < object->payload.object->count; memberIndex++)
   {

      Member member = object->payload.object->members[memberIndex];

      if (stringEqualsCstring(member.key, key))
      {
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
      skipCharacters(parser, trueLiteral.size);
      result->type = ValueType_True;
   }
   else if (isFalseLiteral(parser))
   {
      skipCharacters(parser, falseLiteral.size);
      result->type = ValueType_False;
   }
   else if (isNullLiteral(parser))
   {
      skipCharacters(parser, nullLiteral.size);
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

#endif