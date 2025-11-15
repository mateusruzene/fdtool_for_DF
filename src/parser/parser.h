#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

// Representa um conjunto de atributos A..Z como bits 0..25
typedef uint32_t attrset;

// Estrutura de uma dependência funcional LHS -> RHS
typedef struct
{
  attrset lhs;
  attrset rhs;
} FD;

// Converte string "ABC" para conjunto de bits
attrset attrset_from_string(const char *s);

// Imprime atributos como "ABC"
void print_attrset_compact(attrset s);

// Lê um arquivo .fds e retorna o conjunto de FDs encontrado
FD *parse_fds_file(const char *path, attrset *out_U, int *out_nfds);

#endif