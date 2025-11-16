#include "normalform.h"
#include "mincover.h"
#include "keys.h"
#include "closure.h"
#include <stdio.h>
#include <stdlib.h>

// Verifica se A está contido em B  (A ⊆ B)
static int isSubset(attrset subset, attrset superset)
{
  return (subset & superset) == subset;
}

// Imprime um conjunto de atributos (bitmask -> ABC...)
static void printAttrset(attrset set)
{
  for (int i = 0; i < 26; ++i)
    if (set & (1u << i))
      putchar('A' + i);
}

// Verificação das Formas Normais (BCNF e 3NF)
void checkNormalForms(attrset U, FD *fds, int fdCount)
{
  /* ---------------------------------------------------------
     1) Gerar a cobertura mínima
  --------------------------------------------------------- */
  int minCount = 0;
  FD *minCover = computeMinimumCover(fds, fdCount, &minCount);

  if (minCount == 0)
  {
    printf("No functional dependencies given.\nBCNF: OK\n3NF: OK\n");
    return;
  }

  /* ---------------------------------------------------------
     2) Encontrar as chaves e os atributos primos
  --------------------------------------------------------- */
  int keyCount = 0;
  attrset *candidateKeys = computeCandidateKeys(U, minCover, minCount, &keyCount);

  attrset primeAttributes = 0;
  for (int i = 0; i < keyCount; ++i)
    primeAttributes |= candidateKeys[i];

  /* ---------------------------------------------------------
     3) Detectar violações de BCNF e 3NF
  --------------------------------------------------------- */
  int bcnfOk = 1, bcnfViolations = 0;
  int nf3Ok = 1, nf3Violations = 0;

  for (int i = 0; i < minCount; ++i)
  {
    attrset lhs = minCover[i].lhs;
    attrset rhs = minCover[i].rhs;

    /* Ignora dependências triviais */
    if (isSubset(rhs, lhs))
      continue;

    attrset lhsClosure = computeClosure(lhs, minCover, minCount);
    int lhsIsSuperkey = isSubset(U, lhsClosure);
    int rhsIsPrime = isSubset(rhs, primeAttributes);

    if (!lhsIsSuperkey)
    {
      bcnfOk = 0;
      bcnfViolations++;
    }
    if (!lhsIsSuperkey && !rhsIsPrime)
    {
      nf3Ok = 0;
      nf3Violations++;
    }
  }

  /* ---------------------------------------------------------
     4) Impressão das violações de BCNF
  --------------------------------------------------------- */
  if (bcnfOk)
  {
    printf("BCNF: OK\n");
  }
  else
  {
    printf("BCNF: Violations (%d)\n", bcnfViolations);

    for (int i = 0; i < minCount; ++i)
    {
      attrset lhs = minCover[i].lhs;
      attrset rhs = minCover[i].rhs;

      if (isSubset(rhs, lhs))
        continue;

      attrset lhsClosure = computeClosure(lhs, minCover, minCount);

      if (!isSubset(U, lhsClosure))
      {
        printAttrset(lhs);
        printf(" -> ");
        printAttrset(rhs);
        printf("   (LHS is not a superkey)\n");
      }
    }
  }

  /* ---------------------------------------------------------
     5) Impressão das violações de 3NF
  --------------------------------------------------------- */
  if (nf3Ok)
  {
    printf("3NF: OK\n");
  }
  else
  {
    printf("3NF: Violations (%d)\n", nf3Violations);

    for (int i = 0; i < minCount; ++i)
    {
      attrset lhs = minCover[i].lhs;
      attrset rhs = minCover[i].rhs;

      if (isSubset(rhs, lhs))
        continue;

      attrset lhsClosure = computeClosure(lhs, minCover, minCount);
      int lhsIsSuperkey = isSubset(U, lhsClosure);
      int rhsIsPrime = isSubset(rhs, primeAttributes);

      if (!lhsIsSuperkey && !rhsIsPrime)
      {
        printAttrset(lhs);
        printf(" -> ");
        printAttrset(rhs);
        printf("   (Not superkey AND RHS is not prime)\n");
      }
    }
  }

  free(minCover);
  free(candidateKeys);
}
