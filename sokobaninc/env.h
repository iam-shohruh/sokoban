#ifndef ENV_H
#define ENV_H

/*
 * Sokoban environment implementation (mirrors Python env.py)
 *
 * SokobanRules  – pure rule logic (step, is_deadlock, get_valid_pushes, is_goal_state)
 * SokobanEnv    – wraps rules + level for convenience
 */

#include "state.h"

/* ------------------------------------------------------------------ */
/* Move table entry                                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    char name[8]; /* "UP" | "DOWN" | "LEFT" | "RIGHT" */
    int  dy;
    int  dx;
} Move;

/* ------------------------------------------------------------------ */
/* SokobanRules                                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    Move moves[4]; /* UP, DOWN, LEFT, RIGHT */
} SokobanRules;

/* Constructor */
void sokoban_rules_init(SokobanRules *rules);

/*
 * Applies the given push to the state and returns the resulting state.
 * Caller owns the returned State and must call state_free() on it.
 */
State sokoban_rules_step(const SokobanRules *rules,
                         const State        *state,
                         const Push         *push);

/*
 * Checks if moving a box to new_box_pos results in a corner deadlock.
 * Mirrors Python is_deadlock().
 */
bool sokoban_rules_is_deadlock(const SokobanRules *rules,
                               Position            new_box_pos,
                               const Level        *level);

/*
 * Returns an array of valid pushes for the given state.
 * Caller owns the returned array and must free() it.
 * *out_count is set to the number of pushes returned.
 */
Push *sokoban_rules_get_valid_pushes(const SokobanRules *rules,
                                     const State        *state,
                                     const Level        *level,
                                     int                *out_count);

/*
 * Checks if the given state is a goal state.
 */
bool sokoban_rules_is_goal_state(const State *state, const Level *level);

/* ------------------------------------------------------------------ */
/* SokobanEnv                                                           */
/* ------------------------------------------------------------------ */

typedef struct {
    SokobanRules rules;
    const Level *level;      /* not owned – caller keeps Level alive */
    PositionSet  walls_set;  /* prebuilt for O(1) lookup              */
    PositionSet  goals_set;  /* prebuilt for O(1) lookup              */
} SokobanEnv;

/* Constructor – builds internal PositionSets from level arrays. */
void sokoban_env_init(SokobanEnv *env, const Level *level);

/* Destructor – frees internal PositionSets (does NOT free level). */
void sokoban_env_free(SokobanEnv *env);

State  sokoban_env_reset(const SokobanEnv *env);
State  sokoban_env_step(const SokobanEnv *env, const State *state, const Push *push);
Push  *sokoban_env_get_valid_pushes(const SokobanEnv *env, const State *state, int *out_count);
bool   sokoban_env_is_deadlock(const SokobanEnv *env, const State *state);
bool   sokoban_env_is_goal_state(const SokobanEnv *env, const State *state);

#endif /* ENV_H */