#include "mincover.h"
#include "closure.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

static int fd_redundant_with_keep(FD *set, int n, int idx, const char *keep)
{
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
        continue;

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

FD *compute_minimum_cover(FD *fds, int nfds, int *out_n)
{
  int nunit = 0;
  FD *unit = decompose_rhs(fds, nfds, &nunit);
  if (nunit == 0 || unit == NULL)
  {
    *out_n = 0;
    if (unit)
      free(unit);
    return NULL;
  }

  int loop_changed = 1;
  while (loop_changed)
  {
    loop_changed = 0;
    for (int i = 0; i < nunit; ++i)
    {
      attrset lhs = unit[i].lhs;
      for (int b = 0; b < 26; ++b)
      {
        if (lhs & (1u << b))
        {
          if (lhs_reducible(unit, nunit, i, b))
          {
            unit[i].lhs &= ~(1u << b);
            loop_changed = 1;
            lhs = unit[i].lhs;
          }
        }
      }
    }
  }

  char *keep = malloc(nunit);
  if (!keep)
  {
    free(unit);
    *out_n = 0;
    return NULL;
  }
  for (int i = 0; i < nunit; ++i)
    keep[i] = 1;

  for (int i = 0; i < nunit; ++i)
  {
    if (!keep[i])
      continue;
    if (fd_redundant_with_keep(unit, nunit, i, keep))
    {
      keep[i] = 0;
    }
  }

  int kept = 0;
  for (int i = 0; i < nunit; ++i)
    if (keep[i])
      kept++;

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
