#include "mincover.h"
#include "closure.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------
// Helper: Decompose RHS so that L -> ABC becomes:
//   L -> A
//   L -> B
//   L -> C
// ---------------------------------------------------------
static FD *decompose_rhs(FD *fds, int nfds, int *out_n)
{
  int count = 0;
  for (int i = 0; i < nfds; ++i)
    for (int b = 0; b < 26; ++b)
      if (fds[i].rhs & (1u << b))
        count++;

  if (count == 0)
  {
    *out_n = 0;
    return NULL;
  }

  FD *unit = malloc(sizeof(FD) * count);
  if (!unit)
  {
    *out_n = 0;
    return NULL;
  }
  int k = 0;

  for (int i = 0; i < nfds; ++i)
  {
    for (int b = 0; b < 26; ++b)
    {
      if (fds[i].rhs & (1u << b))
      {
        unit[k].lhs = fds[i].lhs;
        unit[k].rhs = (1u << b);
        k++;
      }
    }
  }

  *out_n = count;
  return unit;
}

// ---------------------------------------------------------
// Helper: Test if an attribute in the LHS can be removed
// (Do NOT use the FD itself during the test; skip idx)
// ---------------------------------------------------------
static int lhs_reducible(FD *set, int n, int idx, int bit)
{
  attrset original = set[idx].lhs;

  if (__builtin_popcount(original) <= 1)
    return 0;

  attrset reduced = original & ~(1u << bit);
  attrset closure = reduced;

  int changed = 1;
  while (changed)
  {
    changed = 0;
    for (int i = 0; i < n; ++i)
    {
      // DO NOT replace or skip the FD being tested; use all FDs as they are
      attrset L = set[i].lhs;

      if ((closure & L) == L)
      {
        attrset add = set[i].rhs & ~closure;
        if (add)
        {
          closure |= add;
          changed = 1;
        }
      }
    }
  }

  return (closure & set[idx].rhs) == set[idx].rhs;
}

// ---------------------------------------------------------
// Helper: Detect redundant FD by checking if F \\ {idx} implies set[idx].
// This version respects a 'keep' mask: only FDs with keep[j]==1 are considered.
// ---------------------------------------------------------
static int fd_redundant_with_keep(FD *set, int n, int idx, const char *keep)
{
  // start closure from the lhs of idx
  attrset closure = set[idx].lhs;
  int changed = 1;
  while (changed)
  {
    changed = 0;
    for (int j = 0; j < n; ++j)
    {
      if (j == idx)
        continue;
      if (!keep[j])
        continue; // skip removed FDs

      if ((closure & set[j].lhs) == set[j].lhs)
      {
        attrset add = set[j].rhs & ~closure;
        if (add)
        {
          closure |= add;
          changed = 1;
        }
      }
    }
  }
  return (closure & set[idx].rhs) == set[idx].rhs;
}

// ---------------------------------------------------------
// Full Minimum Cover Algorithm
// ---------------------------------------------------------
FD *compute_minimum_cover(FD *fds, int nfds, int *out_n)
{
  // 0) decompose RHS
  int nunit = 0;
  FD *unit = decompose_rhs(fds, nfds, &nunit);
  if (nunit == 0 || unit == NULL)
  {
    *out_n = 0;
    if (unit)
      free(unit);
    return NULL;
  }

  // 1) Reduce LHS attributes: repeat until no change
  int loop_changed = 1;
  while (loop_changed)
  {
    loop_changed = 0;
    for (int i = 0; i < nunit; ++i)
    {
      // collect bits in current lhs to iterate
      attrset lhs = unit[i].lhs;
      for (int b = 0; b < 26; ++b)
      {
        if (lhs & (1u << b))
        {
          if (lhs_reducible(unit, nunit, i, b))
          {
            unit[i].lhs &= ~(1u << b);
            loop_changed = 1;
            // update lhs local copy so we don't reconsider the removed bit
            lhs = unit[i].lhs;
            // continue testing other bits (the outer while will re-run if needed)
          }
        }
      }
    }
  }

  // 2) Remove redundant FDs: test each FD against the set of FDs that remain
  char *keep = malloc(nunit);
  if (!keep)
  {
    free(unit);
    *out_n = 0;
    return NULL;
  }
  for (int i = 0; i < nunit; ++i)
    keep[i] = 1;

  // We iterate through FDs and remove ones that are redundant "now".
  for (int i = 0; i < nunit; ++i)
  {
    if (!keep[i])
      continue;
    if (fd_redundant_with_keep(unit, nunit, i, keep))
    {
      keep[i] = 0;
      // do NOT break: continue checking next FDs in current set
    }
  }

  // Count remaining
  int kept = 0;
  for (int i = 0; i < nunit; ++i)
    if (keep[i])
      kept++;

  // Build result array
  FD *result = malloc(sizeof(FD) * kept);
  if (!result)
  {
    free(unit);
    free(keep);
    *out_n = 0;
    return NULL;
  }
  int k = 0;
  for (int i = 0; i < nunit; ++i)
  {
    if (keep[i])
    {
      result[k++] = unit[i];
    }
  }

  free(unit);
  free(keep);
  *out_n = kept;
  return result;
}
