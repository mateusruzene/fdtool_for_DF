#ifndef CLOSURE_H
#define CLOSURE_H

#include <stdint.h>
#include "parser.h"

attrset computeClosure(attrset X, FD *fds, int nfds);

#endif