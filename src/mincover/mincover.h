#ifndef MINCOVER_H
#define MINCOVER_H

#include "parser.h" // for attrset and FD

// Computes the minimum cover of a set of functional dependencies.
// Returns a newly allocated array of FDs.
// The result has unitary RHS, reduced LHS, and no redundant FDs.
FD *compute_minimum_cover(FD *fds, int nfds, int *out_n);

#endif