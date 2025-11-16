
#include "normalform.h"
#include "mincover.h"
#include "keys.h"
#include "closure.h"
#include <stdio.h>
#include <stdlib.h>

static int is_subset(attrset A, attrset B)
{
  return (A & B) == A;
}

static void print_attrset(attrset s)
{
  for (int i = 0; i < 26; ++i)
    if (s & (1u << i))
      putchar('A' + i);
}

void check_normal_forms(attrset U, FD *fds, int nfds)
{

  int nmin = 0;
  FD *min = compute_minimum_cover(fds, nfds, &nmin);

  if (nmin == 0)
  {
    printf("No functional dependencies given.\nBCNF: OK\n3NF: OK\n");
    return;
  }

  int nkeys = 0;
  attrset *keys = compute_candidate_keys(U, min, nmin, &nkeys);

  attrset prime_attrs = 0;
  for (int i = 0; i < nkeys; ++i)
    prime_attrs |= keys[i];

  int bcnf_ok = 1;
  int bcnf_viol_count = 0;

  int nf3_ok = 1;
  int nf3_viol_count = 0;

  for (int i = 0; i < nmin; ++i)
  {
    attrset L = min[i].lhs;
    attrset R = min[i].rhs;

    if (is_subset(R, L))
      continue;

    attrset Lplus = computeClosure(L, min, nmin);
    int L_superkey = is_subset(U, Lplus);
    int R_prime = is_subset(R, prime_attrs);

    if (!L_superkey)
      bcnf_ok = 0, bcnf_viol_count++;
    if (!L_superkey && !R_prime)
      nf3_ok = 0, nf3_viol_count++;
  }

  if (bcnf_ok)
  {
    printf("BCNF: OK\n");
  }
  else
  {
    printf("BCNF: Violations (%d)\n", bcnf_viol_count);
    for (int i = 0; i < nmin; ++i)
    {
      attrset L = min[i].lhs;
      attrset R = min[i].rhs;

      if (is_subset(R, L))
        continue;

      attrset Lplus = computeClosure(L, min, nmin);
      if (!is_subset(U, Lplus))
      {
        print_attrset(L);
        printf(" -> ");
        print_attrset(R);
        printf("   (LHS is not a superkey)\n");
      }
    }
  }

  if (nf3_ok)
  {
    printf("3NF: OK\n");
  }
  else
  {
    printf("3NF: Violations (%d)\n", nf3_viol_count);
    for (int i = 0; i < nmin; ++i)
    {
      attrset L = min[i].lhs;
      attrset R = min[i].rhs;

      if (is_subset(R, L))
        continue;

      attrset Lplus = computeClosure(L, min, nmin);
      int L_superkey = is_subset(U, Lplus);
      int R_prime = is_subset(R, prime_attrs);

      if (!L_superkey && !R_prime)
      {
        print_attrset(L);
        printf(" -> ");
        print_attrset(R);
        printf("   (Not superkey AND RHS is not prime)\n");
      }
    }
  }

  free(min);
  free(keys);
}