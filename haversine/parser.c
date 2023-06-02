#ifndef PARSER_C
#define PARSER_C
#include "common.c"

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
} Value;

Parser initParser(String text, Arena *arena)
{
   Parser parser = {0};
   parser.arena = arena;
   parser.text = text;
}

#endif