#ifndef SOKOBAN_ENV_H
#define SOKOBAN_ENV_H
#include "state.h"

int pos_index(Level *level, Position p);
bool same_pos(Position a, Position b);
bool has_box(State *s, Position p);
State copy_state(State *s);
void free_state(State *s);
State step_state(State *state, Push push);
bool is_deadlock(Position box, Level *level);
bool is_goal_state(State *state, Level *level);
Push *get_valid_pushes(State *state, Level *level, int *count);
int manhattan_heuristic(State *state, Level *level);
char *state_key(State *state);

#endif
