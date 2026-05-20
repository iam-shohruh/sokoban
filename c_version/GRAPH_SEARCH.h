#ifndef GRAPH_SEARCH_H
#define GRAPH_SEARCH_H

#include "data_types.h"
#include <stdbool.h>

Level *load_levels(const char *filepath, int *out_count);
void free_level(Level *level);
const char *dataset_path(const char *dataset);
int dataset_level_count(const char *dataset);

int pos_index(Level *level, Position p);
bool same_pos(Position a, Position b);
bool has_box(State *state, Position p);
State copy_state(State *state);
void free_state(State *state);
State step_state(State *state, Push push);
bool is_deadlock(Position box, Level *level, State *state);
bool is_goal_state(State *state, Level *level);
Push *get_valid_pushes(State *state, Level *level, int *count);
int manhattan_heuristic(State *state, Level *level);
char *state_key(State *state);
void animate_solution(Level *level, SolverResult *result, int delay_ms);

SolverResult First_InsertFrontier_Search_TREE(Level *level);
SolverResult Insert_Priority_Queue_GENERALIZED_A_Star(Level *level);
SolverResult bfs_solver(Level *level);
SolverResult a_star_solver(Level *level);

void evaluate_single(const char *dataset, const char *solver, int level_number, bool animate);
void evaluate(const char *dataset, const char *solver);

#endif
