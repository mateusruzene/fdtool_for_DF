#include "closure.h"
#include <stdio.h>

// ---------------------------------------------------------
// compute_closure
// Computes X+ using the standard iterative algorithm:
// Keep adding RHS attributes when LHS is contained in the closure.
// ---------------------------------------------------------
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

      // If L ⊆ closure, add RHS
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