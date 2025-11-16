#ifndef NORMALFORM_H
#define NORMALFORM_H

#include "parser.h"

// Checks BCNF and 3NF for a given schema U and a list of FDs.
// Prints the result directly (BCNF OK / violations, 3NF OK / violations).
void checkNormalForms(attrset U, FD *fds, int fdCount);

#endif