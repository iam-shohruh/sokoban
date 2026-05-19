#ifndef SOKOBAN_STATE_H
#define SOKOBAN_STATE_H
#include <stdbool.h>

typedef struct { int row, col; } Position;
typedef struct { Position position; char direction[6]; } Push;
typedef struct { Position player; Position *boxes; int num_boxes; } State;
typedef struct {
    int game_id, width, height;
    bool *walls, *goals;
    State init_state;
} Level;
typedef struct { bool solved; Push *path; int path_len; } SolverResult;

SolverResult bfs_solver(Level *level);
SolverResult a_star_solver(Level *level);

#endif
