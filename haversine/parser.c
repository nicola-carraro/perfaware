#ifndef PARSER_C
#define PARSER_C
#include "common.c"
#include "stdbool.h"

#define DOUBLE_QUOTES '"'

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
   } payload;
} Value;

char peekByte(Parser *parser);

Parser initParser(String text, Arena *arena)
{
   Parser parser = {0};
   parser.arena = arena;
   parser.text = text;

   return parser;
}

bool isWhitespace(Parser *parser)
{

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

   result.data = parser->text.data + parser->offset;

   next(parser);

   while (!isDoubleQuotes(parser))
   {

      if (!hasNext(parser))
      {
         die(__FILE__, __LINE__, 0, "unclosed string: expected \"\"\", found end of file (%zu:%zu)\n", stringFirstLine + 1, stringFirstColumn + 1);
      }

      if (isControlCharacter(parser))
      {
         char illegalCharacter = peekByte(parser);
         die(__FILE__, __LINE__, 0, "illegal control character in string literal: found %#02X (%zu:%zu)\n", illegalCharacter, parser->line + 1, parser->column + 1);
      }

      String column = next(parser);

      assert(column.size > 0);
   }

   printf(parser->text.data + parser->offset);
   return result;
}

Value parseElement(Parser *parser)
{

   assert(parser != NULL);
   Value result = {0};
   skipWhitespace(parser);

   if (isDoubleQuotes(parser))
   {
      parseString(parser);
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