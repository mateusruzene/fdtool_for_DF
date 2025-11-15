#include "keys.h"
#include "closure.h"
#include <stdlib.h>
#include <stdio.h>

// Breadth-first search starting from essential attributes (those not appearing in any RHS).
// We store discovered sets to avoid revisiting and collect minimal keys when their closure = U.
attrset *compute_candidate_keys(attrset U, FD *fds, int nfds, int *out_n)
{
  // attributes that appear in any RHS
  attrset all_rhs = 0;
  for (int i = 0; i < nfds; ++i)
    all_rhs |= fds[i].rhs;

  // essentials: attributes that never appear in RHS -> must be present in any key
  attrset essentials = U & ~all_rhs;
  // remaining attributes we may try to add
  attrset remaining = U & ~essentials;

  // queue for BFS
  int qcap = 256;
  attrset *queue = malloc(sizeof(attrset) * qcap);
  int qhead = 0, qtail = 0;

  // visited/discovered sets
  int vcap = 1024;
  attrset *visited = malloc(sizeof(attrset) * vcap);
  int vcount = 0;

  // keys container
  int kcap = 1024;
  attrset *keys = malloc(sizeof(attrset) * kcap);
  int kcount = 0;

  // push essentials
  queue[qtail++] = essentials;
  visited[vcount++] = essentials;

  while (qhead < qtail)
  {
    attrset cur = queue[qhead++];

    // compute closure
    attrset curplus = compute_closure(cur, fds, nfds);

    if ((curplus & U) == U)
    {
      // cur is a superkey; check minimality vs existing keys
      int minimal = 1;
      for (int i = 0; i < kcount; ++i)
      {
        if ((cur & keys[i]) == keys[i])
        { // existing key subset of cur
          minimal = 0;
          break;
        }
      }
      if (minimal)
      {
        // remove keys that are supersets of cur
        int w = 0;
        for (int i = 0; i < kcount; ++i)
        {
          if ((keys[i] & cur) == cur)
          {
            // keys[i] is superset -> drop
          }
          else
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
      continue; // do not expand supersets of a superkey
    }

    // expand by adding one attribute from remaining
    for (int b = 0; b < 26; ++b)
    {
      attrset bit = (1u << b);
      if (!(remaining & bit))
        continue; // cannot add
      if (cur & bit)
        continue; // already present

      attrset nxt = cur | bit;

      // check visited
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

  // shrink keys array to exact size
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