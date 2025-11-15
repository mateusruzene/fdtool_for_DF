#ifndef KEYS_H
#define KEYS_H

#include <stdint.h>
#include "parser.h"

// Computes all minimal candidate keys for the universe U given a set of FDs.
// Returns a dynamically allocated array of attrsets and writes the count to out_n.
attrset *compute_candidate_keys(attrset U, FD *fds, int nfds, int *out_n);

#endif