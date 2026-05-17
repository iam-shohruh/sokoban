#ifndef EVAL_H
#define EVAL_H

/*
 * Evaluation entry point (mirrors Python eval.py)
 *
 * Usage:
 *   ./sokoban_eval --dataset microban --solver bfs
 *   ./sokoban_eval --dataset microban --solver a_star
 */

#include "state.h"

/* ------------------------------------------------------------------ */
/* Dataset paths (mirrors Python DATASETS dict)                        */
/* ------------------------------------------------------------------ */

extern const char *DATASETS[][2]; /* { name, path } pairs, NULL-terminated */
extern const char *SOLVERS[];     /* solver names, NULL-terminated */

/* ------------------------------------------------------------------ */
/* Level loading (mirrors Python load_levels())                        */
/* ------------------------------------------------------------------ */

/*
 * Loads Sokoban levels from a JSON file.
 * Returns a heap-allocated array of Level; *out_count is set to the count.
 * Caller must free each Level with level_free() and free the array.
 */
Level *load_levels(const char *dataset_path, int *out_count);

/* ------------------------------------------------------------------ */
/* Evaluation (mirrors Python evaluate())                              */
/* ------------------------------------------------------------------ */

void evaluate(const char *dataset, const char *solver);

/* ------------------------------------------------------------------ */
/* (Private helpers matching Python _save / _summarize – stubs)       */
/* ------------------------------------------------------------------ */

void _save(const char *dataset, const char *solver);     /* stub */
void _summarize(void);                                   /* stub */

#endif /* EVAL_H */