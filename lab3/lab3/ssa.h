#ifndef SSA_H
#define SSA_H
#include "function.h"

void convert_all_to_ssa();

void convert_to_ssa(Function &func);

int ssa_constant_propagation(Function &func);

void convert_back_to_normal(Function &func);

void convert_all_back_to_normal();

#endif