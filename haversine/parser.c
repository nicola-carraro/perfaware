#ifndef PARSER_C
#define PARSER_C
#include "common.c"
#include "stdbool.h"
#include "stdint.h"

#define DOUBLE_QUOTES '"'
#define REVERSE_SOLIDUS '\\'

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
   ValueType_Invalid,
   ValueType_Object,
   ValueType_Array,
   ValueType_String,
   ValueType_Number,
   ValueType_True,
   ValueType_Null
} ValueType;

typedef struct
{
   ValueType type;
   union
   {
      String string;
      double number;
   } payload;
} Value;

char peekByte(Parser *parser);
bool hasNext(Parser *parser);

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

   char byte = peekByte(parser);

   if (byte > 0x7f)
   {
      die(__FILE__, __LINE__, 0, "Non ASCII parsing not implemented: found %#02X (line %zu, column %zu)", byte, parser->line, parser->column);
   }

   bool isCarriageReturn = byte == '\r';

   bool isNewLine = byte == '\n';

   parser->offset++;

   if (isCarriageReturn && hasNext(parser) && peekByte(parser) == '\n')
   {
      parser->offset++;
      nextLine(parser);

      result.size = 2;
   }
   else
   {
      result.size = 1;
      if (isNewLine)
      {
         nextLine(parser);
      }
      else
      {
         parser->column++;
      }
   }

   return result;
}

char peekByte(Parser *parser)
{
   char result = parser->text.data[parser->offset];

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
   if (!hasNext(parser))
   {
      return false;
   }
   char byte = peekByte(parser);

   return byte == DOUBLE_QUOTES;
}

bool isReverseSolidus(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }
   char byte = peekByte(parser);

   return byte == REVERSE_SOLIDUS;
}

bool isControlCharacter(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }
   char byte = peekByte(parser);

   return byte < 0x20;
}

String parseString(Parser *parser)
{
   assert(parser != NULL);
   assert(parser->text.data != NULL);
   assert(hasNext(parser));
   assert(isDoubleQuotes(parser));

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
         char illegalCharacter = peekByte(parser);
         die(__FILE__, __LINE__, 0, "illegal control character in string: found %#02X (%zu:%zu)\n", illegalCharacter, parser->line + 1, parser->column + 1);
      }

      if (isReverseSolidus(parser))
      {
         die(__FILE__, __LINE__, 0, "escape sequences in strings not implemented (%zu:%zu)\n", parser->line + 1, parser->column + 1);
      }

      String column = next(parser);

      result.size += column.size;

      assert(column.size > 0);
   }

   next(parser);

   return result;
}

bool isMinusSign(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }

   char byte = peekByte(parser);

   return byte == '-';
}

bool isZero(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }
   char byte = peekByte(parser);

   return byte == '0';
}

bool isDot(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }
   char byte = peekByte(parser);

   return byte == '.';
}

bool isExponentStart(Parser *parser)
{
   if (!hasNext(parser))
   {
      return false;
   }
   char byte = peekByte(parser);

   return byte == 'e' || byte == 'E';
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
      char byte = peekByte(parser);
      die(__FILE__, __LINE__, 0, "expected digit, found %c (%zu:%zu)\n", byte, parser->line + 1, parser->column + 1);
   }
   else
   {
      offset += integerDigits;

      if (isDot(parser))
      {
         next(parser);
         offset++;
         size_t fractionDigits = countIntegerDigits(parser);
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
         size_t exponentDigits = countIntegerDigits(parser);

         if (hasMinusSign && exponentDigits == 0)
         {
            char byte = peekByte(parser);
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

   assert(end - buffer > 0);

   assert((end - buffer) == (int64_t)offset);

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
   printf("%.*s", bytesToPrint, string.data);
}

Value parseElement(Parser *parser)
{

   assert(parser != NULL);
   Value result = {0};
   skipWhitespace(parser);

   if (isDoubleQuotes(parser))
   {
      result.payload.string = parseString(parser);
      result.type = ValueType_String;
      // printString(result.payload.string);
   }
   else if (isNumberStart(parser))
   {
      result.payload.number = parseNumber(parser);
      result.type = ValueType_Number;
      printf("%f", result.payload.number);
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