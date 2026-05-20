#ifndef SOKOBAN_EVAL_H
#define SOKOBAN_EVAL_H

#include "state.h"
#include <stdbool.h>

Level *load_levels(const char *filepath, int *out_count);
void free_level(Level *level);
void evaluate(const char *dataset, const char *solver);
void evaluate_single(const char *dataset, const char *solver, int level_number, bool animate);

#endif
