#ifndef KEYS_H
#define KEYS_H

#include <stdint.h>
#include "parser.h"

attrset *computeCandidateKeys(attrset U, FD *fds, int nfds, int *out_n);

#endif