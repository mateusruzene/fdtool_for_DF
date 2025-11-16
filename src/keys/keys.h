#ifndef KEYS_H
#define KEYS_H

#include <stdint.h>
#include "parser.h"

attrset *computeCandidateKeys(attrset U, FD *fds, int fdCount, int *outCount);

#endif