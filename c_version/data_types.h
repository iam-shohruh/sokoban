#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <stdbool.h>

#define PREDETERMINED_GOAL_STATE 1
#define ACTIONS_NUMBER 4
#define MAX_SEARCHED_NODE 1000000

typedef enum ACTIONS {
    ACTION_UP = 0,
    ACTION_DOWN = 1,
    ACTION_LEFT = 2,
    ACTION_RIGHT = 3
} ACTIONS;

typedef struct Position {
    int row;
    int col;
} Position;

typedef struct Push {
    Position position;
    char direction[6];
} Push;

typedef struct State {
    Position player;
    Position *boxes;
    int num_boxes;
} State;

typedef struct Level {
    int game_id;
    int width;
    int height;
    bool *walls;
    bool *goals;
    State init_state;
} Level;

typedef struct SolverResult {
    bool solved;
    Push *path;
    int path_len;
} SolverResult;

#endif
