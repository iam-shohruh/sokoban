#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#define MAX_BOXES     2
#define MAX_BOARD_DIM 12

/* Actions encode a push as: cell_index * 4 + direction
   direction: 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT
   cell_index = row * board_cols + col
   The enum values below are just symbolic names for the 4 directions;
   actual action integers go up to ACTIONS_NUMBER-1 (set in GRAPH_SEARCH.h). */
enum ACTIONS
{
    PUSH_UP = 0, PUSH_DOWN = 1, PUSH_LEFT = 2, PUSH_RIGHT = 3
};

typedef struct State
{
    int player_row, player_col;
    int box_rows[MAX_BOXES];
    int box_cols[MAX_BOXES];
    int num_boxes;
    float h_n;   /* heuristic value (required by the framework) */
} State;

/* ====== DO NOT CHANGE THIS PART ======================================== */

enum METHODS
{
    BreastFirstSearch = 1,   UniformCostSearch = 2,        DepthFirstSearch = 3,
    DepthLimitedSearch= 4,   IterativeDeepeningSearch = 5, GreedySearch = 6,
    AStarSearch = 7,         GeneralizedAStarSearch = 8
};

typedef struct Transition_Model
{
    State new_state;
    float step_cost;
} Transition_Model;

typedef struct Node
{
    State state;
    float path_cost;
    enum ACTIONS action;
    struct Node *parent;
    int Number_of_Child;
} Node;

typedef struct Queue
{
    Node *node;
    struct Queue *next;
} Queue;

#endif
