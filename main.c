#include "parser.h"
#include "closure.h"
#include "mincover.h"
#include "keys.h"
#include "normalform.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------
   Exibe instruções de uso do programa
------------------------------------------------------------ */
static void printUsage(const char *programName)
{
  fprintf(stderr,
          "Usage:\n"
          "  %s closure    --fds <file.fds> --X <ATTRS>\n"
          "  %s mincover   --fds <file.fds>\n"
          "  %s keys       --fds <file.fds>\n"
          "  %s normalform --fds <file.fds>\n",
          programName, programName, programName, programName);
}

/* ------------------------------------------------------------
   Função principal: interpreta comandos e chama os módulos
------------------------------------------------------------ */
int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printUsage(argv[0]);
    return 1;
  }

  const char *command = argv[1];

  /* --------------------------------------------------------
     Comando: CLOSURE
  -------------------------------------------------------- */
  if (strcmp(command, "closure") == 0)
  {
    const char *fdsPath = NULL;
    const char *xString = NULL;

    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fdsPath = argv[++i];
      else if (strcmp(argv[i], "--X") == 0 && i + 1 < argc)
        xString = argv[++i];
      else
      {
        printUsage(argv[0]);
        return 1;
      }
    }

    if (!fdsPath || !xString)
    {
      printUsage(argv[0]);
      return 1;
    }

    attrset universe = 0;
    int fdCount = 0;
    FD *fds = parseFdsFile(fdsPath, &universe, &fdCount);
    if (!fds)
      return 1;

    attrset X = attrsetFromString(xString);
    attrset closure = computeClosure(X, fds, fdCount);

    printAttrsetCompact(closure);
    printf("\n");
    return 0;
  }

  /* --------------------------------------------------------
     Comando: MINCOVER
  -------------------------------------------------------- */
  else if (strcmp(command, "mincover") == 0)
  {
    const char *fdsPath = NULL;

    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fdsPath = argv[++i];
      else
      {
        printUsage(argv[0]);
        return 1;
      }
    }

    attrset universe = 0;
    int fdCount = 0;
    FD *fds = parseFdsFile(fdsPath, &universe, &fdCount);
    if (!fds)
      return 1;

    int minCount = 0;
    FD *minCover = computeMinimumCover(fds, fdCount, &minCount);

    for (int i = 0; i < minCount; ++i)
    {
      printAttrsetCompact(minCover[i].lhs);
      printf("->");
      printAttrsetCompact(minCover[i].rhs);
      printf("\n");
    }
    return 0;
  }

  /* --------------------------------------------------------
     Comando: KEYS
  -------------------------------------------------------- */
  else if (strcmp(command, "keys") == 0)
  {
    const char *fdsPath = NULL;

    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fdsPath = argv[++i];
      else
      {
        printUsage(argv[0]);
        return 1;
      }
    }

    attrset universe = 0;
    int fdCount = 0;
    FD *fds = parseFdsFile(fdsPath, &universe, &fdCount);
    if (!fds)
      return 1;

    int minCount = 0;
    FD *minCover = computeMinimumCover(fds, fdCount, &minCount);

    int keyCount = 0;
    attrset *keys = computeCandidateKeys(universe, minCover, minCount, &keyCount);

    for (int i = 0; i < keyCount; ++i)
    {
      printAttrsetCompact(keys[i]);
      printf("\n");
    }
    return 0;
  }

  /* --------------------------------------------------------
     Comando: NORMALFORM
  -------------------------------------------------------- */
  else if (strcmp(command, "normalform") == 0)
  {
    const char *fdsPath = NULL;

    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], "--fds") == 0 && i + 1 < argc)
        fdsPath = argv[++i];
      else
      {
        printUsage(argv[0]);
        return 1;
      }
    }

    attrset universe = 0;
    int fdCount = 0;
    FD *fds = parseFdsFile(fdsPath, &universe, &fdCount);
    if (!fds)
      return 1;

    checkNormalForms(universe, fds, fdCount);
    return 0;
  }

  /* --------------------------------------------------------
     Comando desconhecido
  -------------------------------------------------------- */
  printUsage(argv[0]);
  return 1;
}
