#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int char_to_index(char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a';
  return -1;
}

attrset attrset_from_string(const char *s)
{
  attrset result = 0;
  for (size_t i = 0; s[i]; ++i)
  {
    int idx = char_to_index(s[i]);
    if (idx >= 0 && idx < 26)
      result |= (1u << idx);
  }
  return result;
}

void print_attrset_compact(attrset s)
{
  for (int i = 0; i < 26; ++i)
  {
    if (s & (1u << i))
      putchar('A' + i);
  }
}

static char *read_file(const char *path)
{
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = malloc(size + 1);
  if (!buf)
  {
    fclose(f);
    return NULL;
  }

  fread(buf, 1, size, f);
  buf[size] = '\0';
  fclose(f);
  return buf;
}

static char *strip_ws(const char *s)
{
  size_t n = strlen(s);
  char *out = malloc(n + 1);
  size_t j = 0;

  for (size_t i = 0; i < n; ++i)
    if (!isspace((unsigned char)s[i]))
      out[j++] = s[i];

  out[j] = '\0';
  return out;
}

static char *extract_braced(const char *buf, const char *prefix)
{
  const char *p = strstr(buf, prefix);
  if (!p)
    return NULL;
  p += strlen(prefix);

  while (*p && *p != '{')
    p++;
  if (!*p)
    return NULL;
  p++; // skip '{'

  const char *q = p;
  int depth = 1;

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

static char **split_fds(const char *s, int *count)
{
  int cap = 8;
  char **arr = malloc(sizeof(char *) * cap);
  *count = 0;

  size_t n = strlen(s);
  size_t start = 0;

  for (size_t i = 0; i <= n; ++i)
  {
    if (s[i] == ',' || s[i] == ';' || s[i] == '\0')
    {
      size_t len = i - start;

      while (len > 0 && isspace((unsigned char)s[start + len - 1]))
        len--;
      while (len > 0 && isspace((unsigned char)s[start]))
      {
        start++;
        len--;
      }

      if (len > 0)
      {
        if (*count >= cap)
        {
          cap *= 2;
          arr = realloc(arr, sizeof(char *) * cap);
        }

        char *tok = malloc(len + 1);
        memcpy(tok, s + start, len);
        tok[len] = '\0';
        arr[(*count)++] = tok;
      }

      start = i + 1;
    }
  }

  return arr;
}

static int parse_fd_token(const char *tok, attrset *lhs, attrset *rhs)
{
  const char *arrow = strstr(tok, "->");
  if (!arrow)
    return -1;

  size_t lhs_len = arrow - tok;
  size_t rhs_len = strlen(tok) - lhs_len - 2;

  char *lhs_str = malloc(lhs_len + 1);
  char *rhs_str = malloc(rhs_len + 1);

  memcpy(lhs_str, tok, lhs_len);
  lhs_str[lhs_len] = '\0';

  memcpy(rhs_str, arrow + 2, rhs_len);
  rhs_str[rhs_len] = '\0';

  *lhs = attrset_from_string(lhs_str);
  *rhs = attrset_from_string(rhs_str);

  free(lhs_str);
  free(rhs_str);
  return 0;
}

FD *parse_fds_file(const char *path, attrset *out_U, int *out_nfds)
{
  char *buf = read_file(path);
  if (!buf)
  {
    fprintf(stderr, "Error: cannot read %s\n", path);
    return NULL;
  }

  char *clean = strip_ws(buf);
  free(buf);

  char *u_str = extract_braced(clean, "U=");
  attrset U = u_str ? attrset_from_string(u_str) : 0;
  free(u_str);

  char *f_str = extract_braced(clean, "F=");
  if (!f_str)
  {
    fprintf(stderr, "Error: file missing F={...}\n");
    free(clean);
    return NULL;
  }

  int count = 0;
  char **tokens = split_fds(f_str, &count);

  FD *fds = malloc(sizeof(FD) * count);
  int nfds = 0;

  for (int i = 0; i < count; ++i)
  {
    attrset L, R;
    if (parse_fd_token(tokens[i], &L, &R) == 0)
    {
      fds[nfds].lhs = L;
      fds[nfds].rhs = R;
      nfds++;
      U |= (L | R);
    }
    free(tokens[i]);
  }

  free(tokens);
  free(f_str);
  free(clean);

  *out_U = U;
  *out_nfds = nfds;
  return fds;
}