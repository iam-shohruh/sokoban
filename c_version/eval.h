#ifndef SOKOBAN_EVAL_H
#define SOKOBAN_EVAL_H
#include "state.h"
Level *load_levels(const char *filepath, int *out_count);
void free_level(Level *level);
void evaluate(const char *dataset, const char *solver);
#endif
