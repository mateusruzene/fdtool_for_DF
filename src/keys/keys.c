#include "keys.h"
#include "closure.h"
#include <stdlib.h>
#include <stdio.h>

attrset *compute_candidate_keys(attrset U, FD *fds, int nfds, int *out_n)
{
  attrset all_rhs = 0;
  for (int i = 0; i < nfds; ++i)
    all_rhs |= fds[i].rhs;

  attrset essentials = U & ~all_rhs;
  attrset remaining = U & ~essentials;

  int qcap = 256;
  attrset *queue = malloc(sizeof(attrset) * qcap);
  int qhead = 0, qtail = 0;

  int vcap = 1024;
  attrset *visited = malloc(sizeof(attrset) * vcap);
  int vcount = 0;

  int kcap = 1024;
  attrset *keys = malloc(sizeof(attrset) * kcap);
  int kcount = 0;

  queue[qtail++] = essentials;
  visited[vcount++] = essentials;

  while (qhead < qtail)
  {
    attrset cur = queue[qhead++];

    attrset curplus = computeClosure(cur, fds, nfds);

    if ((curplus & U) == U)
    {
      int minimal = 1;
      for (int i = 0; i < kcount; ++i)
      {
        if ((cur & keys[i]) == keys[i])
        {
          minimal = 0;
          break;
        }
      }
      if (minimal)
      {
        int w = 0;
        for (int i = 0; i < kcount; ++i)
        {
          if ((keys[i] & cur) != cur)
          {
            keys[w++] = keys[i];
          }
        }
        kcount = w;

        if (kcount + 1 >= kcap)
        {
          kcap *= 2;
          keys = realloc(keys, sizeof(attrset) * kcap);
        }
        keys[kcount++] = cur;
      }
      continue;
    }

    for (int b = 0; b < 26; ++b)
    {
      attrset bit = (1u << b);
      if (!(remaining & bit))
        continue;
      if (cur & bit)
        continue;

      attrset nxt = cur | bit;

      int seen = 0;
      for (int i = 0; i < vcount; ++i)
        if (visited[i] == nxt)
        {
          seen = 1;
          break;
        }
      if (seen)
        continue;

      if (vcount + 1 >= vcap)
      {
        vcap *= 2;
        visited = realloc(visited, sizeof(attrset) * vcap);
      }
      visited[vcount++] = nxt;

      if (qtail + 1 >= qcap)
      {
        qcap *= 2;
        queue = realloc(queue, sizeof(attrset) * qcap);
      }
      queue[qtail++] = nxt;
    }
  }

  free(queue);
  free(visited);

  if (kcount == 0)
  {
    free(keys);
    *out_n = 0;
    return NULL;
  }
  keys = realloc(keys, sizeof(attrset) * kcount);
  *out_n = kcount;
  return keys;
}