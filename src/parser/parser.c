#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Converte um caractere (A–Z ou a–z) em índice 0–25
static int charToIndex(char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a';
  return -1;
}

// Converte uma string (ex: "ABC") em um attrset (bitmask)
attrset attrsetFromString(const char *s)
{
  attrset result = 0;

  for (size_t i = 0; s[i]; ++i)
  {
    int idx = charToIndex(s[i]);
    if (idx >= 0 && idx < 26)
      result |= (1u << idx);
  }

  return result;
}

// Imprime attrset na forma compacta (ex: 0101 -> AC)
void printAttrsetCompact(attrset set)
{
  for (int i = 0; i < 26; ++i)
    if (set & (1u << i))
      putchar('A' + i);
}

// Lê um arquivo inteiro para memória
static char *readFile(const char *path)
{
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buffer = malloc(size + 1);
  if (!buffer)
  {
    fclose(f);
    return NULL;
  }

  fread(buffer, 1, size, f);
  buffer[size] = '\0';

  fclose(f);
  return buffer;
}

// Remove todos os espaços em branco (útil para parsing)
static char *removeWhitespace(const char *s)
{
  size_t length = strlen(s);
  char *clean = malloc(length + 1);
  size_t j = 0;

  for (size_t i = 0; i < length; ++i)
    if (!isspace((unsigned char)s[i]))
      clean[j++] = s[i];

  clean[j] = '\0';
  return clean;
}

/* ---------------------------------------------------------------
   Extrai o conteúdo de algo no formato PREFIX { conteúdo }
   Ex: U={ABC} → retorna "ABC"
---------------------------------------------------------------- */
static char *extractBraced(const char *buffer, const char *prefix)
{
  const char *p = strstr(buffer, prefix);
  if (!p)
    return NULL;

  p += strlen(prefix);

  /* Avança até encontrar '{' */
  while (*p && *p != '{')
    p++;

  if (!*p)
    return NULL;

  p++; /* pula '{' */

  const char *q = p;
  int depth = 1;

  /* Encontra o '}' correspondente */
  while (*q && depth > 0)
  {
    if (*q == '{')
      depth++;
    else if (*q == '}')
      depth--;
    if (depth > 0)
      q++;
  }

  if (depth != 0)
    return NULL;

  size_t len = q - p;
  char *out = malloc(len + 1);
  memcpy(out, p, len);
  out[len] = '\0';

  return out;
}

// Divide F={A->B,CD->E,...} em tokens ["A->B", "CD->E", ...]
static char **splitFds(const char *s, int *outCount)
{
  int capacity = 8;
  char **array = malloc(sizeof(char *) * capacity);
  *outCount = 0;

  size_t length = strlen(s);
  size_t start = 0;

  for (size_t i = 0; i <= length; ++i)
  {
    if (s[i] == ',' || s[i] == ';' || s[i] == '\0')
    {
      size_t tokenLen = i - start;

      /* Remove espaços nos extremos */
      while (tokenLen > 0 && isspace((unsigned char)s[start + tokenLen - 1]))
        tokenLen--;
      while (tokenLen > 0 && isspace((unsigned char)s[start]))
      {
        start++;
        tokenLen--;
      }

      if (tokenLen > 0)
      {
        if (*outCount >= capacity)
        {
          capacity *= 2;
          array = realloc(array, sizeof(char *) * capacity);
        }

        char *token = malloc(tokenLen + 1);
        memcpy(token, s + start, tokenLen);
        token[tokenLen] = '\0';

        array[(*outCount)++] = token;
      }

      start = i + 1;
    }
  }

  return array;
}

// Converte um token "XY->Z" para lhs e rhs
static int parseFdToken(const char *token, attrset *lhs, attrset *rhs)
{
  const char *arrow = strstr(token, "->");
  if (!arrow)
    return -1;

  size_t lhsLen = arrow - token;
  size_t rhsLen = strlen(token) - lhsLen - 2;

  char *lhsStr = malloc(lhsLen + 1);
  char *rhsStr = malloc(rhsLen + 1);

  memcpy(lhsStr, token, lhsLen);
  lhsStr[lhsLen] = '\0';

  memcpy(rhsStr, arrow + 2, rhsLen);
  rhsStr[rhsLen] = '\0';

  *lhs = attrsetFromString(lhsStr);
  *rhs = attrsetFromString(rhsStr);

  free(lhsStr);
  free(rhsStr);

  return 0;
}

/* ---------------------------------------------------------------
   Função principal — lê arquivo e produz:
      U (universo)
      F (conjunto de FDs)
---------------------------------------------------------------- */
FD *parseFdsFile(const char *path, attrset *outU, int *outFdCount)
{
  char *buffer = readFile(path);
  if (!buffer)
  {
    fprintf(stderr, "Error: cannot read %s\n", path);
    return NULL;
  }

  char *clean = removeWhitespace(buffer);
  free(buffer);

  /* ------------------ Lê U={...} ------------------ */
  char *universeString = extractBraced(clean, "U=");
  attrset universe = universeString ? attrsetFromString(universeString) : 0;
  free(universeString);

  /* ------------------ Lê F={...} ------------------ */
  char *fdsString = extractBraced(clean, "F=");
  if (!fdsString)
  {
    fprintf(stderr, "Error: file missing F={...}\n");
    free(clean);
    return NULL;
  }

  /* ------------------ Divide dependências ------------------ */
  int tokenCount = 0;
  char **tokens = splitFds(fdsString, &tokenCount);

  FD *fds = malloc(sizeof(FD) * tokenCount);
  int fdCount = 0;

  for (int i = 0; i < tokenCount; ++i)
  {
    attrset lhs, rhs;

    if (parseFdToken(tokens[i], &lhs, &rhs) == 0)
    {
      fds[fdCount].lhs = lhs;
      fds[fdCount].rhs = rhs;
      fdCount++;

      universe |= (lhs | rhs);
    }

    free(tokens[i]);
  }

  free(tokens);
  free(fdsString);
  free(clean);

  *outU = universe;
  *outFdCount = fdCount;

  return fds;
}
