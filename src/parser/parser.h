#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

typedef uint32_t attrset;

typedef struct
{
  attrset lhs;
  attrset rhs;
} FD;

attrset attrsetFromString(const char *s);

void printAttrsetCompact(attrset set);

FD *parseFdsFile(const char *path, attrset *outU, int *outFdCount);

#endif