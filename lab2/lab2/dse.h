#ifndef DSE_H
#define DSE_H
#include "function.h"

// returns the count of assignments which have been eliminated
int dead_code_elimination(Function &func);

#endif