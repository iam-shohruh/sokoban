#ifndef A_STAR_H
#define A_STAR_H

/*
 * A* search for Sokoban (mirrors Python search/a_star.py)
 *
 * Heuristics:
 *   manhattan_heuristic          – sum of min Manhattan distances box→nearest goal
 *   hungarian_heuristic          – placeholder
 *   pattern_database_heuristic   – placeholder
 *   hybrid_heuristic             – placeholder
 *
 * Solver:
 *   a_star_solver                – returns SolverResult
 */

#include "../state.h"
#include "../env.h"

/* ------------------------------------------------------------------ */
/* Heuristics                                                           */
/* ------------------------------------------------------------------ */

int manhattan_heuristic(const State *state, const Level *level);
int hungarian_heuristic(const State *state, const Level *level);     /* placeholder */
int pattern_database_heuristic(const State *state, const Level *level); /* placeholder */
int hybrid_heuristic(const State *state, const Level *level);        /* placeholder */

/* ------------------------------------------------------------------ */
/* Result type                                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    bool  solved;
    Push *path;       /* caller must free() */
    int   path_len;
} SolverResult;

void solver_result_free(SolverResult *r);

/* ------------------------------------------------------------------ */
/* A* solver                                                            */
/* ------------------------------------------------------------------ */

/*
 * Solves a Sokoban level using A*.
 * debug flag is accepted but step collection is omitted in this C port
 * (the Python debug/steps_out feature is a Python-specific introspection aid).
 */
SolverResult a_star_solver(const Level *level, bool debug);

#endif /* A_STAR_H */