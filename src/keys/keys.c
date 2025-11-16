#include "keys.h"
#include "closure.h"
#include <stdlib.h>
#include <stdio.h>

attrset *computeCandidateKeys(attrset U, FD *fds, int fdCount, int *outCount)
{
  attrset allRhsAttributes = 0;
  for (int i = 0; i < fdCount; ++i)
    allRhsAttributes |= fds[i].rhs;

  attrset essentialAttributes = U & ~allRhsAttributes;
  attrset remainingAttributes = U & ~essentialAttributes;

  int queueCapacity = 256;
  attrset *queue = malloc(sizeof(attrset) * queueCapacity);
  int queueHead = 0, queueTail = 0;

  int visitedCapacity = 1024;
  attrset *visited = malloc(sizeof(attrset) * visitedCapacity);
  int visitedCount = 0;

  int keyCapacity = 1024;
  attrset *candidateKeys = malloc(sizeof(attrset) * keyCapacity);
  int keyCount = 0;

  /* Inicializa BFS com os atributos essenciais */
  queue[queueTail++] = essentialAttributes;
  visited[visitedCount++] = essentialAttributes;

  /* ------------------------------------------------------
     BFS para gerar candidatos e testar minimalidade
  ------------------------------------------------------ */
  while (queueHead < queueTail)
  {
    attrset currentSet = queue[queueHead++];
    attrset closureOfCurrent = computeClosure(currentSet, fds, fdCount);

    /* --------------------------------------------------
       Se o fecho é superchave → possível chave candidata
    -------------------------------------------------- */
    if ((closureOfCurrent & U) == U)
    {
      int isMinimal = 1;

      /* Verifica se alguma chave existente é subconjunto da atual */
      for (int i = 0; i < keyCount; ++i)
      {
        if ((currentSet & candidateKeys[i]) == candidateKeys[i])
        {
          isMinimal = 0;
          break;
        }
      }

      if (isMinimal)
      {
        /* Remove candidatos não minimal frente ao novo */
        int w = 0;
        for (int i = 0; i < keyCount; ++i)
        {
          if ((candidateKeys[i] & currentSet) != currentSet)
            candidateKeys[w++] = candidateKeys[i];
        }
        keyCount = w;

        /* Adiciona a nova chave */
        if (keyCount + 1 >= keyCapacity)
        {
          keyCapacity *= 2;
          candidateKeys = realloc(candidateKeys, sizeof(attrset) * keyCapacity);
        }

        candidateKeys[keyCount++] = currentSet;
      }

      continue;
    }

    /* --------------------------------------------------
       Expande o conjunto tentando adicionar atributos restantes
    -------------------------------------------------- */
    for (int b = 0; b < 26; ++b)
    {
      attrset bit = (1u << b);
      if (!(remainingAttributes & bit))
        continue;
      if (currentSet & bit)
        continue;

      attrset nextSet = currentSet | bit;

      /* Verifica se já foi visitado */
      int hasSeen = 0;
      for (int i = 0; i < visitedCount; ++i)
      {
        if (visited[i] == nextSet)
        {
          hasSeen = 1;
          break;
        }
      }

      if (hasSeen)
        continue;

      /* Registra como visitado */
      if (visitedCount + 1 >= visitedCapacity)
      {
        visitedCapacity *= 2;
        visited = realloc(visited, sizeof(attrset) * visitedCapacity);
      }
      visited[visitedCount++] = nextSet;

      /* Adiciona à fila */
      if (queueTail + 1 >= queueCapacity)
      {
        queueCapacity *= 2;
        queue = realloc(queue, sizeof(attrset) * queueCapacity);
      }
      queue[queueTail++] = nextSet;
    }
  }

  /* Limpeza de estruturas temporárias */
  free(queue);
  free(visited);

  /* Não encontrou nenhuma chave? */
  if (keyCount == 0)
  {
    free(candidateKeys);
    *outCount = 0;
    return NULL;
  }

  /* Ajusta tamanho final */
  candidateKeys = realloc(candidateKeys, sizeof(attrset) * keyCount);
  *outCount = keyCount;

  return candidateKeys;
}
