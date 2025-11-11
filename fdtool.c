/*
 * fdtool.c
 * Ferramenta de linha de comando para FDs:
 * - Parser .fds (U = {...}, F = {...})
 * - closure (fecho X+)
 * - mincover (cobertura mínima)
 * - keys (listar chaves candidatas mínimas)
 * - normalform (verificar BCNF e 3NF)
 *
 * Compilar: make
 *
 * Observações:
 * - Representamos atributos A..Z como bits 0..25 em uint32_t (attrset).
 * - Parser aceita atributos concatenados (ABC) ou com vírgulas (A,B,C).
 * - Mincover usa RHS unitário + redução de LHS + remoção de redundâncias.
 * - Keys: busca por BFS/geração incremental com poda (começando por atributos essenciais).
 * - Normalform: usa mincover unitário para analisar FDs.
 *
 * Uso resumido:
 * ./fdtool closure --fds <file> --X <attrs>
 * ./fdtool mincover --fds <file>
 * ./fdtool keys --fds <file>
 * ./fdtool normalform --fds <file>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

typedef uint32_t attrset; // 26 atributos A..Z

typedef struct
{
  attrset lhs;
  attrset rhs; // may be multi-bit for input; unitary in mincover/out
} FD;

/* ----------------- utilitários ----------------- */

static int letter_to_index(char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a';
  return -1;
}

static attrset attrset_from_string(const char *s)
{
  attrset res = 0;
  for (size_t i = 0; s[i]; ++i)
  {
    int idx = letter_to_index(s[i]);
    if (idx >= 0 && idx < 26)
      res |= (1u << idx);
  }
  return res;
}

static void print_attrset_compact(attrset s)
{
  if (s == 0)
  {
    printf("(vazio)");
    return;
  }
  for (int i = 0; i < 26; ++i)
    if (s & (1u << i))
      putchar('A' + i);
}

static void print_attrset_spaced(attrset s)
{
  int first = 1;
  for (int i = 0; i < 26; ++i)
  {
    if (s & (1u << i))
    {
      if (!first)
        putchar(' ');
      putchar('A' + i);
      first = 0;
    }
  }
  if (first)
    printf("(vazio)");
}

static int contains(attrset sup, attrset sub) { return (sup & sub) == sub; }

/* ----------------- I/O / parser ----------------- */

static char *read_file_to_buffer(const char *path)
{
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = malloc(sz + 1);
  if (!buf)
  {
    fclose(f);
    return NULL;
  }
  if (fread(buf, 1, sz, f) != (size_t)sz)
  {
    free(buf);
    fclose(f);
    return NULL;
  }
  buf[sz] = '\0';
  fclose(f);
  return buf;
}

static char *strip_whitespace(const char *s)
{
  size_t n = strlen(s);
  char *out = malloc(n + 1);
  if (!out)
    return NULL;
  size_t j = 0;
  for (size_t i = 0; i < n; ++i)
  {
    char c = s[i];
    if (!isspace((unsigned char)c))
      out[j++] = c;
  }
  out[j] = '\0';
  return out;
}

static char *extract_braced_content(const char *buf, const char *prefix)
{
  const char *p = strstr(buf, prefix);
  if (!p)
    return NULL;
  p += strlen(prefix);
  while (*p && *p != '{')
    ++p;
  if (!*p)
    return NULL;
  ++p;
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
  if (!out)
    return NULL;
  memcpy(out, p, len);
  out[len] = '\0';
  return out;
}

static char **split_fds_list(const char *s, int *count)
{
  int capacity = 8;
  char **arr = malloc(sizeof(char *) * capacity);
  *count = 0;
  size_t i = 0;
  size_t n = strlen(s);
  size_t start = 0;
  for (i = 0; i <= n; ++i)
  {
    if (s[i] == ',' || s[i] == ';' || s[i] == '\0')
    {
      size_t len = i - start;
      // trim sides
      while (len > 0 && isspace((unsigned char)s[start + len - 1]))
        len--;
      size_t st = start;
      while (len > 0 && isspace((unsigned char)s[st]))
      {
        st++;
        len--;
      }
      if (len > 0)
      {
        if (*count >= capacity)
        {
          capacity *= 2;
          arr = realloc(arr, sizeof(char *) * capacity);
        }
        char *tok = malloc(len + 1);
        memcpy(tok, s + st, len);
        tok[len] = '\0';
        arr[(*count)++] = tok;
      }
      start = i + 1;
    }
  }
  arr = realloc(arr, sizeof(char *) * ((*count) + 1));
  arr[*count] = NULL;
  return arr;
}

static int parse_fd_token(const char *tok, attrset *lhs, attrset *rhs)
{
  const char *arrow = strstr(tok, "->");
  if (!arrow)
    return -1;
  size_t l_len = arrow - tok;
  size_t r_len = strlen(tok) - (l_len + 2);
  if (l_len == 0 || r_len == 0)
    return -1;
  char *lstr = malloc(l_len + 1);
  char *rstr = malloc(r_len + 1);
  if (!lstr || !rstr)
  {
    free(lstr);
    free(rstr);
    return -1;
  }
  memcpy(lstr, tok, l_len);
  lstr[l_len] = '\0';
  memcpy(rstr, arrow + 2, r_len);
  rstr[r_len] = '\0';
  *lhs = attrset_from_string(lstr);
  *rhs = attrset_from_string(rstr);
  free(lstr);
  free(rstr);
  return 0;
}

static FD *parse_fds_file(const char *path, attrset *out_universe, int *out_nfds)
{
  char *buf = read_file_to_buffer(path);
  if (!buf)
  {
    fprintf(stderr, "Erro: não foi possível ler '%s'\n", path);
    return NULL;
  }
  char *s = strip_whitespace(buf);
  free(buf);
  if (!s)
  {
    fprintf(stderr, "Erro: memória\n");
    return NULL;
  }
  char *ucontent = extract_braced_content(s, "U=");
  attrset universe = 0;
  if (ucontent)
  {
    universe = attrset_from_string(ucontent);
    free(ucontent);
  }
  char *fcontent = extract_braced_content(s, "F=");
  if (!fcontent)
  {
    fprintf(stderr, "Erro: arquivo não contém F= {...}\n");
    free(s);
    return NULL;
  }
  int token_count = 0;
  char **tokens = split_fds_list(fcontent, &token_count);
  free(fcontent);
  free(s);
  FD *fds = malloc(sizeof(FD) * token_count);
  int nfds = 0;
  for (int i = 0; i < token_count; ++i)
  {
    attrset lhs = 0, rhs = 0;
    if (parse_fd_token(tokens[i], &lhs, &rhs) == 0)
    {
      fds[nfds].lhs = lhs;
      fds[nfds].rhs = rhs;
      nfds++;
      universe |= lhs | rhs;
    }
    else
    {
      fprintf(stderr, "Aviso: token FD inválido: %s\n", tokens[i]);
    }
    free(tokens[i]);
  }
  free(tokens);
  *out_universe = universe;
  *out_nfds = nfds;
  return fds;
}

/* ----------------- closure ----------------- */

static attrset closure(attrset X, FD *fds, int nfds)
{
  attrset res = X;
  int changed = 1;
  while (changed)
  {
    changed = 0;
    for (int i = 0; i < nfds; ++i)
    {
      if (contains(res, fds[i].lhs))
      {
        attrset news = fds[i].rhs & ~res;
        if (news)
        {
          res |= news;
          changed = 1;
        }
      }
    }
  }
  return res;
}

/* ----------------- mincover ----------------- */

static FD *decompose_rhs_unitary(FD *fds, int nfds, int *nf_out)
{
  int count = 0;
  for (int i = 0; i < nfds; ++i)
  {
    attrset r = fds[i].rhs;
    for (int b = 0; b < 26; ++b)
      if (r & (1u << b))
        count++;
  }
  FD *out = malloc(sizeof(FD) * (count));
  int k = 0;
  for (int i = 0; i < nfds; ++i)
  {
    attrset r = fds[i].rhs;
    for (int b = 0; b < 26; ++b)
    {
      if (r & (1u << b))
      {
        out[k].lhs = fds[i].lhs;
        out[k].rhs = (1u << b);
        k++;
      }
    }
  }
  *nf_out = k;
  return out;
}

static int lhs_can_be_reduced(FD *unit, int nunit, int idx, int attr_bit)
{
  attrset original_lhs = unit[idx].lhs;
  attrset test_lhs = original_lhs & ~(1u << attr_bit);
  attrset res = test_lhs;
  int changed = 1;
  while (changed)
  {
    changed = 0;
    for (int j = 0; j < nunit; ++j)
    {
      attrset lhs_j = unit[j].lhs;
      if (j == idx)
        lhs_j = test_lhs;
      if (contains(res, lhs_j))
      {
        attrset news = unit[j].rhs & ~res;
        if (news)
        {
          res |= news;
          changed = 1;
        }
      }
    }
  }
  return contains(res, unit[idx].rhs);
}

static int fd_is_redundant(FD *unit, int nunit, int idx)
{
  attrset res = unit[idx].lhs;
  int changed = 1;
  while (changed)
  {
    changed = 0;
    for (int j = 0; j < nunit; ++j)
    {
      if (j == idx)
        continue;
      if (contains(res, unit[j].lhs))
      {
        attrset news = unit[j].rhs & ~res;
        if (news)
        {
          res |= news;
          changed = 1;
        }
      }
    }
  }
  return contains(res, unit[idx].rhs);
}

static FD *compute_mincover(FD *fds, int nfds, int *nf_out)
{
  int nunit = 0;
  FD *unit = decompose_rhs_unitary(fds, nfds, &nunit);
  if (nunit == 0)
  {
    *nf_out = 0;
    return NULL;
  }

  // reduzir LHS
  for (int i = 0; i < nunit; ++i)
  {
    for (int b = 0; b < 26; ++b)
    {
      if (unit[i].lhs & (1u << b))
      {
        if (lhs_can_be_reduced(unit, nunit, i, b))
        {
          unit[i].lhs &= ~(1u << b);
        }
      }
    }
  }

  // remover redundantes
  char *to_keep = malloc(nunit);
  int keepcount = 0;
  for (int i = 0; i < nunit; ++i)
    to_keep[i] = 1;
  for (int i = 0; i < nunit; ++i)
  {
    if (fd_is_redundant(unit, nunit, i))
    {
      to_keep[i] = 0;
    }
  }
  for (int i = 0; i < nunit; ++i)
    if (to_keep[i])
      keepcount++;
  FD *out = malloc(sizeof(FD) * keepcount);
  int k = 0;
  for (int i = 0; i < nunit; ++i)
    if (to_keep[i])
    {
      out[k++] = unit[i];
    }

  free(unit);
  free(to_keep);
  *nf_out = keepcount;
  return out;
}

/* ----------------- keys (chaves candidatas) ----------------- */

// retorna array malloc'd de attrset (terminado por 0?) -> We'll return vector and nkeys via pointer.
static attrset *compute_candidate_keys(attrset U, FD *fds, int nfds, int *out_nkeys)
{
  // atributos essenciais: aqueles que nunca aparecem em RHS (então obrigatórios)
  attrset all_rhs = 0;
  for (int i = 0; i < nfds; ++i)
    all_rhs |= fds[i].rhs;
  attrset essentials = U & ~all_rhs;

  // lista de todos atributos em U
  attrset attrs = U;
  // prepare candidate attributes to try adding (attrs_not_essentials)
  attrset remaining = U & ~essentials;

  // simple BFS over sets starting from essentials
  // We'll represent queue as dynamic array of attrsets
  int qcap = 256;
  attrset *queue = malloc(sizeof(attrset) * qcap);
  int qhead = 0, qtail = 0;
  // discovered sets to avoid duplicates: naive - use map with visited bitset keyed by attrset; since 26 bits, use hash table via chained list
  // For simplicity and safety, we'll store discovered sets in dynamic array and check linear (may be slower but simpler).
  attrset *discovered = malloc(sizeof(attrset) * 1024);
  int disc_cap = 1024, disc_n = 0;

  // push essentials
  queue[qtail++] = essentials;
  discovered[disc_n++] = essentials;

  // store found keys
  attrset *keys = malloc(sizeof(attrset) * 1024);
  int keys_cap = 1024, keys_n = 0;

  while (qhead < qtail)
  {
    attrset cur = queue[qhead++];
    // compute closure
    attrset curplus = closure(cur, fds, nfds);
    if (contains(curplus, U))
    {
      // cur is a superkey, now check minimality: no subset of cur in keys
      int minimal = 1;
      for (int k = 0; k < keys_n; ++k)
        if (contains(cur, keys[k]))
        {
          minimal = 0;
          break;
        }
      if (minimal)
      {
        // remove any existing keys that are supersets of cur
        int w = 0;
        for (int k = 0; k < keys_n; ++k)
        {
          if (contains(keys[k], cur))
          {
            // keys[k] is superset => remove
          }
          else
          {
            keys[w++] = keys[k];
          }
        }
        keys_n = w;
        if (keys_n + 1 >= keys_cap)
        {
          keys_cap *= 2;
          keys = realloc(keys, sizeof(attrset) * keys_cap);
        }
        keys[keys_n++] = cur;
      }
      continue; // do not expand supersets of a superkey
    }
    // else expand by adding one attribute (try attributes in remaining)
    for (int b = 0; b < 26; ++b)
    {
      attrset bit = (1u << b);
      if ((remaining & bit) == 0)
        continue; // not allowed to add (either essential or not in U)
      if (cur & bit)
        continue; // already present
      attrset nxt = cur | bit;
      // check if discovered
      int seen = 0;
      for (int d = 0; d < disc_n; ++d)
        if (discovered[d] == nxt)
        {
          seen = 1;
          break;
        }
      if (seen)
        continue;
      if (disc_n + 1 >= disc_cap)
      {
        disc_cap *= 2;
        discovered = realloc(discovered, sizeof(attrset) * disc_cap);
      }
      discovered[disc_n++] = nxt;
      if (qtail + 1 >= qcap)
      {
        qcap *= 2;
        queue = realloc(queue, sizeof(attrset) * qcap);
      }
      queue[qtail++] = nxt;
    }
  }

  free(queue);
  free(discovered);

  *out_nkeys = keys_n;
  // shrink keys
  keys = realloc(keys, sizeof(attrset) * keys_n);
  return keys;
}

/* ----------------- normalform (BCNF / 3NF) ----------------- */

static void check_normal_forms(attrset U, FD *fds, int nfds)
{
  // use mincover unitary
  int nmin = 0;
  FD *min = compute_mincover(fds, nfds, &nmin);
  if (nmin == 0)
  {
    printf("Mincover vazio (sem FDs)\\nBCNF: OK\\n3NF: OK\\n");
    if (min)
      free(min);
    return;
  }

  // compute keys and prime attributes
  int nkeys = 0;
  attrset *keys = compute_candidate_keys(U, min, nmin, &nkeys);
  attrset primes = 0;
  for (int i = 0; i < nkeys; ++i)
    primes |= keys[i];

  // BCNF check: for each FD L->A (non-trivial), L must be superkey
  int bcnf_ok = 1;
  int th_bcnf = 0;
  // 3NF check: for each FD L->A non-trivial, either L is superkey or A prime
  int th_3nf = 0;
  int nf3_ok = 1;

  // Print violations lists collected for readability
  // We'll print lines "VIOLATION L->A : reason"
  for (int i = 0; i < nmin; ++i)
  {
    attrset L = min[i].lhs;
    attrset A = min[i].rhs;
    if (contains(L, A))
      continue; // trivial
    // is L a superkey?
    attrset Lplus = closure(L, min, nmin);
    int L_is_super = contains(Lplus, U);
    if (!L_is_super)
    {
      bcnf_ok = 0;
      th_bcnf++;
    }
    // 3NF?
    int A_is_prime = contains(primes, A);
    if (!L_is_super && !A_is_prime)
    {
      nf3_ok = 0;
      th_3nf++;
    }
  }

  if (bcnf_ok)
    printf("BCNF: OK\n");
  else
  {
    printf("BCNF: VIOLATIONS (%d)\n", th_bcnf);
    for (int i = 0; i < nmin; ++i)
    {
      attrset L = min[i].lhs;
      attrset A = min[i].rhs;
      if (contains(L, A))
        continue;
      attrset Lplus = closure(L, min, nmin);
      if (!contains(Lplus, U))
      {
        print_attrset_compact(L);
        printf("->");
        print_attrset_compact(A);
        printf("    (L não é superchave)\n");
      }
    }
  }

  if (nf3_ok)
    printf("3NF: OK\n");
  else
  {
    printf("3NF: VIOLATIONS (%d)\n", th_3nf);
    for (int i = 0; i < nmin; ++i)
    {
      attrset L = min[i].lhs;
      attrset A = min[i].rhs;
      if (contains(L, A))
        continue;
      attrset Lplus = closure(L, min, nmin);
      int L_is_super = contains(Lplus, U);
      int A_is_prime = contains(primes, A);
      if (!L_is_super && !A_is_prime)
      {
        print_attrset_compact(L);
        printf("->");
        print_attrset_compact(A);
        printf("    (L não é superchave e A não é primo)\n");
      }
    }
  }

  // free
  if (min)
    free(min);
  if (keys)
    free(keys);
}

/* ----------------- CLI ----------------- */

static void print_usage(const char *prog)
{
  fprintf(stderr,
          "Uso:\n"
          "  %s closure --fds <arquivo.fds> --X <ATRIBUTOS>\n"
          "  %s mincover --fds <arquivo.fds>\n"
          "  %s keys --fds <arquivo.fds>\n"
          "  %s normalform --fds <arquivo.fds>\n",
          prog, prog, prog, prog);
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    print_usage(argv[0]);
    return 1;
  }
  const char *cmd = argv[1];

  if (strcmp(cmd, "closure") == 0)
  {
    const char *fds_path = NULL;
    const char *xstr = NULL;
    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fds_path = argv[++i];
      else if (strcmp(argv[i], "--X") == 0 && i + 1 < argc)
        xstr = argv[++i];
      else
      {
        fprintf(stderr, "Argumento desconhecido: %s\n", argv[i]);
        print_usage(argv[0]);
        return 1;
      }
    }
    if (!fds_path || !xstr)
    {
      print_usage(argv[0]);
      return 1;
    }
    attrset U = 0;
    int nfds = 0;
    FD *fds = parse_fds_file(fds_path, &U, &nfds);
    if (!fds)
      return 1;
    attrset X = attrset_from_string(xstr);
    attrset Xplus = closure(X, fds, nfds);
    printf("X = ");
    print_attrset_spaced(X);
    printf("\n");
    printf("X+ = ");
    print_attrset_spaced(Xplus);
    printf("\n");
    if (U != 0 && contains(Xplus, U))
      printf("Observação: X é superchave (X+ == U)\n");
    free(fds);
    return 0;
  }
  else if (strcmp(cmd, "mincover") == 0)
  {
    const char *fds_path = NULL;
    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fds_path = argv[++i];
      else
      {
        fprintf(stderr, "Argumento desconhecido: %s\n", argv[i]);
        print_usage(argv[0]);
        return 1;
      }
    }
    if (!fds_path)
    {
      print_usage(argv[0]);
      return 1;
    }
    attrset U = 0;
    int nfds = 0;
    FD *fds = parse_fds_file(fds_path, &U, &nfds);
    if (!fds)
      return 1;
    int nmin = 0;
    FD *min = compute_mincover(fds, nfds, &nmin);
    for (int i = 0; i < nmin; ++i)
    {
      print_attrset_compact(min[i].lhs);
      printf("->");
      print_attrset_compact(min[i].rhs);
      printf("\n");
    }
    free(fds);
    if (min)
      free(min);
    return 0;
  }
  else if (strcmp(cmd, "keys") == 0)
  {
    const char *fds_path = NULL;
    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fds_path = argv[++i];
      else
      {
        fprintf(stderr, "Argumento desconhecido: %s\n", argv[i]);
        print_usage(argv[0]);
        return 1;
      }
    }
    if (!fds_path)
    {
      print_usage(argv[0]);
      return 1;
    }
    attrset U = 0;
    int nfds = 0;
    FD *fds = parse_fds_file(fds_path, &U, &nfds);
    if (!fds)
      return 1;
    // use mincover (unitary) to compute keys (more stable)
    int nmin = 0;
    FD *min = compute_mincover(fds, nfds, &nmin);
    if (!min)
    {
      printf("(nenhuma FD)\\n");
      free(fds);
      return 0;
    }
    int nkeys = 0;
    attrset *keys = compute_candidate_keys(U, min, nmin, &nkeys);
    printf("Chaves candidatas (%d):\n", nkeys);
    for (int i = 0; i < nkeys; ++i)
    {
      print_attrset_spaced(keys[i]);
      printf("\n");
    }
    free(fds);
    free(min);
    if (keys)
      free(keys);
    return 0;
  }
  else if (strcmp(cmd, "normalform") == 0)
  {
    const char *fds_path = NULL;
    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fds_path = argv[++i];
      else
      {
        fprintf(stderr, "Argumento desconhecido: %s\n", argv[i]);
        print_usage(argv[0]);
        return 1;
      }
    }
    if (!fds_path)
    {
      print_usage(argv[0]);
      return 1;
    }
    attrset U = 0;
    int nfds = 0;
    FD *fds = parse_fds_file(fds_path, &U, &nfds);
    if (!fds)
      return 1;
    check_normal_forms(U, fds, nfds);
    free(fds);
    return 0;
  }
  else
  {
    print_usage(argv[0]);
    return 1;
  }
}
