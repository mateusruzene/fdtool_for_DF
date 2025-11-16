#include "closure.h"
#include <stdio.h>

// Retorna o fecho X+ de X sob F
attrset computeClosure(attrset X, FD *fds, int nfds)
{
  attrset closure = X;
  int changed = 1;

  while (changed)
  {
    changed = 0;

    for (int i = 0; i < nfds; ++i)
    {
      // Se LHS está contido no fecho
      if ((closure & fds[i].lhs) == fds[i].lhs)
      {
        // atributos de RHS que ainda não foram incluídos no fecho
        attrset missing = fds[i].rhs & (~closure);

        if (missing != 0)
        {
          closure |= missing;
          changed = 1;
        }
      }
    }
  }

  return closure;
}