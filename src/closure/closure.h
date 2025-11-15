#ifndef CLOSURE_H
#define CLOSURE_H

#include <stdint.h>
#include "parser.h"

// Computes the attribute closure X+ under the set of FDs
attrset compute_closure(attrset X, FD *fds, int nfds);

#endif