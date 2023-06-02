#ifndef PARSER_C
#define PARSER_C
#include "common.c"
#include "stdbool.h"

#define QUOTATION_MARKS '"'

typedef struct
{
   String text;
   size_t offset;
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

Parser initParser(String text, Arena *arena)
{
   Parser parser = {0};
   parser.arena = arena;
   parser.text = text;

   return parser;
}

bool isWhitespace(char byte)
{
   return byte == 0x20 || byte == 0x0a || byte == 0x0d || byte == 0x09;
}

char nextByte(Parser *parser)
{
   char result = parser->text.data[parser->offset];
   
   return result;
}

void skipWhitespace(Parser *parser)
{
   char byte = nextByte(parser);
   while (isWhitespace(byte)) {
       parser->offset++;
       byte = nextByte(parser);
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

   char byte = nextByte(parser);

   if (byte == QUOTATION_MARKS)
   {
      parseString(parser);
   }
   else
   {
      printf("%c", byte);
   }

   skipWhitespace(parser);

   return result;
}

#endif