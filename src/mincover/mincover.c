#include "mincover.h"
#include "closure.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* -----------------------------------------------------------------------------
   PASSO 1 — Decompor o lado direito (RHS)
   Uma dependência X -> ABC precisa ser transformada em três dependências:
       X -> A
       X -> B
       X -> C
   Isso garante que cada FD tenha apenas 1 atributo no RHS.
 ----------------------------------------------------------------------------- */
static FD *decomposeRhs(FD *fds, int fdCount, int *outCount)
{
  int rhsAtomCount = 0;

  /* Conta quantos atributos individuais existem no RHS de todas as FDs */
  for (int i = 0; i < fdCount; ++i)
    for (int b = 0; b < 26; ++b)
      if (fds[i].rhs & (1u << b))
        rhsAtomCount++;

  if (rhsAtomCount == 0)
  {
    *outCount = 0;
    return NULL;
  }

  /* Aloca espaço para todas as dependências unitárias */
  FD *unitaryFds = malloc(sizeof(FD) * rhsAtomCount);
  if (!unitaryFds)
  {
    *outCount = 0;
    return NULL;
  }

  int index = 0;

  /* Cria as dependências X -> A, X -> B, X -> C... */
  for (int i = 0; i < fdCount; ++i)
  {
    for (int b = 0; b < 26; ++b)
    {
      if (fds[i].rhs & (1u << b))
      {
        unitaryFds[index].lhs = fds[i].lhs;
        unitaryFds[index].rhs = (1u << b);
        index++;
      }
    }
  }

  *outCount = rhsAtomCount;
  return unitaryFds;
}

/* -----------------------------------------------------------------------------
   PASSO 2 — Verificar se um atributo do LHS é estranho (redundante)
   Um atributo é estranho se, ao removê-lo, o fecho ainda determinar o RHS.
 ----------------------------------------------------------------------------- */
static int lhsAttributeIsRedundant(FD *fdSet, int fdCount, int targetIndex, int bit)
{
  attrset originalLhs = fdSet[targetIndex].lhs;

  /* Não pode reduzir se só há um atributo no LHS */
  if (__builtin_popcount(originalLhs) <= 1)
    return 0;

  /* Remove o atributo candidato e calcula fecho */
  attrset reducedLhs = originalLhs & ~(1u << bit);
  attrset closure = reducedLhs;

  int changed = 1;
  while (changed)
  {
    changed = 0;

    for (int i = 0; i < fdCount; ++i)
    {
      attrset lhs = fdSet[i].lhs;

      if ((closure & lhs) == lhs)
      {
        attrset newAttributes = fdSet[i].rhs & ~closure;
        if (newAttributes)
        {
          closure |= newAttributes;
          changed = 1;
        }
      }
    }
  }

  /* Se o fecho ainda determina o RHS, o atributo é estranho */
  return (closure & fdSet[targetIndex].rhs) == fdSet[targetIndex].rhs;
}

/* -----------------------------------------------------------------------------
   PASSO 3 — Verificar se uma FD é redundante
   Testa se X -> A pode ser removida sem alterar o conjunto de implicações.
   Usa a máscara "keepMask" para ignorar dependências já removidas.
 ----------------------------------------------------------------------------- */
static int fdIsRedundant(FD *fdSet, int fdCount, int targetIndex, const char *keepMask)
{
  attrset closure = fdSet[targetIndex].lhs;

  int changed = 1;
  while (changed)
  {
    changed = 0;

    for (int j = 0; j < fdCount; ++j)
    {
      if (j == targetIndex)
        continue;
      if (!keepMask[j])
        continue;

      if ((closure & fdSet[j].lhs) == fdSet[j].lhs)
      {
        attrset newAttributes = fdSet[j].rhs & ~closure;
        if (newAttributes)
        {
          closure |= newAttributes;
          changed = 1;
        }
      }
    }
  }

  /* Se o fecho obtiver o RHS, então a FD era redundante */
  return (closure & fdSet[targetIndex].rhs) == fdSet[targetIndex].rhs;
}

/* -----------------------------------------------------------------------------
   ALGORITMO COMPLETO — Cobertura Mínima
   1) Decompor RHS
   2) Remover atributos estranhos do LHS
   3) Remover dependências redundantes
 ----------------------------------------------------------------------------- */
FD *computeMinimumCover(FD *fds, int fdCount, int *outCount)
{
  /* 1) Decompor RHS */
  int unitaryCount = 0;
  FD *unitaryFds = decomposeRhs(fds, fdCount, &unitaryCount);

  if (unitaryCount == 0 || unitaryFds == NULL)
  {
    *outCount = 0;
    if (unitaryFds)
      free(unitaryFds);
    return NULL;
  }

  /* 2) Remover atributos estranhos do LHS */
  int changed = 1;
  while (changed)
  {
    changed = 0;

    for (int i = 0; i < unitaryCount; ++i)
    {
      attrset lhs = unitaryFds[i].lhs;

      for (int b = 0; b < 26; ++b)
      {
        if (lhs & (1u << b))
        {
          if (lhsAttributeIsRedundant(unitaryFds, unitaryCount, i, b))
          {
            unitaryFds[i].lhs &= ~(1u << b);
            changed = 1;
            lhs = unitaryFds[i].lhs;
          }
        }
      }
    }
  }

  /* 3) Remover dependências redundantes */
  char *keepMask = malloc(unitaryCount);
  if (!keepMask)
  {
    free(unitaryFds);
    *outCount = 0;
    return NULL;
  }

  for (int i = 0; i < unitaryCount; ++i)
    keepMask[i] = 1;

  for (int i = 0; i < unitaryCount; ++i)
  {
    if (!keepMask[i])
      continue;

    if (fdIsRedundant(unitaryFds, unitaryCount, i, keepMask))
      keepMask[i] = 0;
  }

  /* Conta quantas dependências restaram */
  int keptCount = 0;
  for (int i = 0; i < unitaryCount; ++i)
    if (keepMask[i])
      keptCount++;

  FD *result = malloc(sizeof(FD) * keptCount);
  if (!result)
  {
    free(unitaryFds);
    free(keepMask);
    *outCount = 0;
    return NULL;
  }

  /* Constrói a cobertura mínima final */
  int index = 0;
  for (int i = 0; i < unitaryCount; ++i)
  {
    if (keepMask[i])
      result[index++] = unitaryFds[i];
  }

  free(unitaryFds);
  free(keepMask);

  *outCount = keptCount;
  return result;
}
