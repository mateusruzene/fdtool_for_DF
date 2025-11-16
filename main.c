#include "parser.h"
#include "closure.h"
#include "mincover.h"
#include "keys.h"
#include "normalform.h"
#include <stdio.h>
#include <string.h>

static void print_usage(const char *prog)
{
  fprintf(stderr,
          "Usage:\n"
          "  %s closure    --fds <file.fds> --X <ATTRS>\n"
          "  %s mincover   --fds <file.fds>\n"
          "  %s keys       --fds <file.fds>\n"
          "  %s normalform --fds <file.fds>\n",
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

  /* -------------------------------------------------- */
  /* Fecho */
  /* -------------------------------------------------- */
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
    attrset Xplus = computeClosure(X, fds, nfds);
    print_attrset_compact(Xplus);
    printf("\n");
    return 0;
  }

  /* -------------------------------------------------- */
  /* Cobertura mínima */
  /* -------------------------------------------------- */
  else if (strcmp(cmd, "mincover") == 0)
  {
    const char *fds_path = NULL;

    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fds_path = argv[++i];
      else
      {
        print_usage(argv[0]);
        return 1;
      }
    }

    attrset U = 0;
    int nfds = 0;
    FD *fds = parse_fds_file(fds_path, &U, &nfds);
    if (!fds)
      return 1;

    int nmin = 0;
    FD *min = compute_minimum_cover(fds, nfds, &nmin);

    for (int i = 0; i < nmin; ++i)
    {
      print_attrset_compact(min[i].lhs);
      printf("->");
      print_attrset_compact(min[i].rhs);
      printf("\n");
    }
    return 0;
  }

  /* -------------------------------------------------- */
  /* Chaves */
  /* -------------------------------------------------- */
  else if (strcmp(cmd, "keys") == 0)
  {
    const char *fds_path = NULL;

    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fds_path = argv[++i];
      else
      {
        print_usage(argv[0]);
        return 1;
      }
    }

    attrset U = 0;
    int nfds = 0;
    FD *fds = parse_fds_file(fds_path, &U, &nfds);
    if (!fds)
      return 1;

    int nmin = 0;
    FD *min = compute_minimum_cover(fds, nfds, &nmin);

    int nkeys = 0;
    attrset *keys = compute_candidate_keys(U, min, nmin, &nkeys);

    for (int i = 0; i < nkeys; ++i)
    {
      print_attrset_compact(keys[i]);
      printf("\n");
    }
    return 0;
  }

  /* -------------------------------------------------- */
  /* Forma normal */
  /* -------------------------------------------------- */
  else if (strcmp(cmd, "normalform") == 0)
  {
    const char *fds_path = NULL;

    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fds_path = argv[++i];
      else
      {
        print_usage(argv[0]);
        return 1;
      }
    }

    attrset U = 0;
    int nfds = 0;
    FD *fds = parse_fds_file(fds_path, &U, &nfds);
    if (!fds)
      return 1;

    check_normal_forms(U, fds, nfds);
    return 0;
  }

  /* -------------------------------------------------- */
  /* Comando desconhecido */
  /* -------------------------------------------------- */
  print_usage(argv[0]);
  return 1;
}