#include "closure.h"
#include <stdio.h>

attrset compute_closure(attrset X, FD *fds, int nfds)
{
  attrset closure = X;
  int changed = 1;

  while (changed)
  {
    changed = 0;

    for (int i = 0; i < nfds; ++i)
    {
      attrset L = fds[i].lhs;
      attrset R = fds[i].rhs;

      if ((closure & L) == L)
      {
        attrset new_bits = R & ~closure;
        if (new_bits)
        {
          closure |= new_bits;
          changed = 1;
        }
      }
    }
  }

  return closure;
}