#ifndef PARSER_C
#define PARSER_C
#include "common.c"
#include "stdbool.h"

#define QUOTATION_MARKS '"'

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
      parser->offset++;
      byte = peekByte(parser);
   }
}

Value parseString(Parser *parser)
{
   Value result = {0};

   printf(parser->text.data + parser->offset);
   return result;
}

Value parseElement(Parser *parser)
{
   Value result = {0};
   skipWhitespace(parser);

   char byte = peekByte(parser);

   if (byte == QUOTATION_MARKS)
   {
      parseString(parser);
   }
   else
   {
      if (hasNext(parser))
      {
         String codepoint = next(parser);
         die(__FILE__, __LINE__, 0, "Expected JSON value, found \"%.*s\"\n", codepoint.size, codepoint.data);
      }
      {
         die(__FILE__, __LINE__, 0, "Expected JSON value, found end of file");
      }
   }

   skipWhitespace(parser);

   return result;
}

#endif