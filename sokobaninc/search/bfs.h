#ifndef BFS_H
#define BFS_H

/*
 * BFS solver for Sokoban (mirrors Python search/bfs.py)
 */

#include "../state.h"
#include "../env.h"
#include "a_star.h"   /* re-use SolverResult */

SolverResult bfs_solver(const Level *level);

#endif /* BFS_H */