#include "env.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* SokobanRules                                                         */
/* ------------------------------------------------------------------ */

void sokoban_rules_init(SokobanRules *rules) {
    /* mirrors Python: self.moves = {"UP":(-1,0),"DOWN":(1,0),"LEFT":(0,-1),"RIGHT":(0,1)} */
    strcpy(rules->moves[0].name, "UP");    rules->moves[0].dy = -1; rules->moves[0].dx =  0;
    strcpy(rules->moves[1].name, "DOWN");  rules->moves[1].dy =  1; rules->moves[1].dx =  0;
    strcpy(rules->moves[2].name, "LEFT");  rules->moves[2].dy =  0; rules->moves[2].dx = -1;
    strcpy(rules->moves[3].name, "RIGHT"); rules->moves[3].dy =  0; rules->moves[3].dx =  1;
}

State sokoban_rules_step(const SokobanRules *rules,
                         const State        *state,
                         const Push         *push) {
    /*
     * Python:
     *   boxes = set(state.boxes)
     *   box_pos, direction = push
     *   new_box_pos = Position(box_pos[0]+dy, box_pos[1]+dx)
     *   new_player_pos = box_pos
     *   boxes.remove(box_pos); boxes.add(new_box_pos)
     *   return State(player=new_player_pos, boxes=frozenset(boxes))
     */
    int dy = 0, dx = 0;
    for (int i = 0; i < 4; i++) {
        if (strcmp(rules->moves[i].name, push->direction) == 0) {
            dy = rules->moves[i].dy;
            dx = rules->moves[i].dx;
            break;
        }
    }

    Position box_pos     = push->position;
    Position new_box_pos = (Position){ box_pos.row + dy, box_pos.col + dx };
    Position new_player_pos = box_pos;

    State new_state;
    new_state.player    = new_player_pos;
    new_state.num_boxes = state->num_boxes;
    new_state.boxes     = (Position *)malloc(state->num_boxes * sizeof(Position));

    int idx = 0;
    for (int i = 0; i < state->num_boxes; i++) {
        if (!position_equals(state->boxes[i], box_pos)) {
            new_state.boxes[idx++] = state->boxes[i];
        }
    }
    new_state.boxes[idx] = new_box_pos;

    state_sort_boxes(&new_state);
    return new_state;
}

bool sokoban_rules_is_deadlock(const SokobanRules *rules,
                               Position            new_box_pos,
                               const Level        *level) {
    /*
     * Python:
     *   if new_box_pos in level.goals: return False
     *   horz_blocked = vert_blocked = False
     *   for direction,(dy,dx) in self.moves.items():
     *       if Position(y+dy,x+dx) in level.walls:
     *           if UP/DOWN: vert_blocked = True
     *           if LEFT/RIGHT: horz_blocked = True
     *   return horz_blocked and vert_blocked
     */

    /* Check if box is on a goal – not deadlocked */
    for (int g = 0; g < level->num_goals; g++) {
        if (position_equals(new_box_pos, level->goals[g])) return false;
    }

    int y = new_box_pos.row;
    int x = new_box_pos.col;

    bool horz_blocked = false;
    bool vert_blocked = false;

    for (int i = 0; i < 4; i++) {
        Position neighbor = { y + rules->moves[i].dy, x + rules->moves[i].dx };
        for (int w = 0; w < level->num_walls; w++) {
            if (position_equals(neighbor, level->walls[w])) {
                if (strcmp(rules->moves[i].name, "UP")   == 0 ||
                    strcmp(rules->moves[i].name, "DOWN")  == 0) {
                    vert_blocked = true;
                } else {
                    horz_blocked = true;
                }
                break;
            }
        }
    }

    return horz_blocked && vert_blocked;
}

Push *sokoban_rules_get_valid_pushes(const SokobanRules *rules,
                                     const State        *state,
                                     const Level        *level,
                                     int                *out_count) {
    /*
     * Python:
     *   valid_pushes = []
     *   visited = {player}; queue = [player]
     *   while queue:
     *       current_pos = queue.pop()
     *       for direction,(dy,dx) in self.moves.items():
     *           next_pos = Position(current_pos[0]+dy, current_pos[1]+dx)
     *           if next_pos in walls or next_pos in visited: continue
     *           elif next_pos in boxes:
     *               box_next_pos = Position(next_pos[0]+dy, next_pos[1]+dx)
     *               if box_next_pos not in walls and box_next_pos not in boxes:
     *                   valid_pushes.append(Push(next_pos, direction))
     *           else:
     *               visited.add(next_pos); queue.append(next_pos)
     *   return valid_pushes
     */

    Position player = state->player;

    /* Build wall and box sets for O(1) lookup */
    PositionSet walls_set = position_set_from_array(level->walls, level->num_walls);
    PositionSet boxes_set = position_set_from_array(state->boxes, state->num_boxes);

    /* visited set */
    PositionSet visited = position_set_create(64);
    position_set_add(&visited, player);

    /* queue (simple dynamic array stack – Python uses list.pop() which is LIFO) */
    int       queue_cap  = 64;
    int       queue_size = 0;
    Position *queue      = (Position *)malloc(queue_cap * sizeof(Position));
    queue[queue_size++]  = player;

    /* result pushes */
    int   pushes_cap  = 16;
    int   pushes_size = 0;
    Push *pushes      = (Push *)malloc(pushes_cap * sizeof(Push));

    while (queue_size > 0) {
        Position current_pos = queue[--queue_size]; /* pop from end like Python list.pop() */

        for (int i = 0; i < 4; i++) {
            Position next_pos = {
                current_pos.row + rules->moves[i].dy,
                current_pos.col + rules->moves[i].dx
            };

            if (position_set_contains(&walls_set, next_pos)) continue;
            if (position_set_contains(&visited,   next_pos)) continue;

            if (position_set_contains(&boxes_set, next_pos)) {
                Position box_next_pos = {
                    next_pos.row + rules->moves[i].dy,
                    next_pos.col + rules->moves[i].dx
                };
                if (!position_set_contains(&walls_set, box_next_pos) &&
                    !position_set_contains(&boxes_set, box_next_pos)) {
                    if (pushes_size == pushes_cap) {
                        pushes_cap *= 2;
                        pushes = (Push *)realloc(pushes, pushes_cap * sizeof(Push));
                    }
                    pushes[pushes_size].position = next_pos;
                    strcpy(pushes[pushes_size].direction, rules->moves[i].name);
                    pushes_size++;
                }
            } else {
                position_set_add(&visited, next_pos);
                if (queue_size == queue_cap) {
                    queue_cap *= 2;
                    queue = (Position *)realloc(queue, queue_cap * sizeof(Position));
                }
                queue[queue_size++] = next_pos;
            }
        }
    }

    free(queue);
    position_set_free(&walls_set);
    position_set_free(&boxes_set);
    position_set_free(&visited);

    *out_count = pushes_size;
    return pushes;
}

bool sokoban_rules_is_goal_state(const State *state, const Level *level) {
    /*
     * Python: return all(box in level.goals for box in state.boxes)
     */
    for (int b = 0; b < state->num_boxes; b++) {
        bool found = false;
        for (int g = 0; g < level->num_goals; g++) {
            if (position_equals(state->boxes[b], level->goals[g])) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* SokobanEnv                                                           */
/* ------------------------------------------------------------------ */

void sokoban_env_init(SokobanEnv *env, const Level *level) {
    sokoban_rules_init(&env->rules);
    env->level      = level;
    env->walls_set  = position_set_from_array(level->walls, level->num_walls);
    env->goals_set  = position_set_from_array(level->goals, level->num_goals);
}

void sokoban_env_free(SokobanEnv *env) {
    position_set_free(&env->walls_set);
    position_set_free(&env->goals_set);
}

State sokoban_env_reset(const SokobanEnv *env) {
    return state_copy(&env->level->init_state);
}

State sokoban_env_step(const SokobanEnv *env, const State *state, const Push *push) {
    return sokoban_rules_step(&env->rules, state, push);
}

Push *sokoban_env_get_valid_pushes(const SokobanEnv *env, const State *state, int *out_count) {
    return sokoban_rules_get_valid_pushes(&env->rules, state, env->level, out_count);
}

bool sokoban_env_is_deadlock(const SokobanEnv *env, const State *state) {
    /*
     * Python:
     *   for box_pos in state.boxes:
     *       if self.rules.is_deadlock(box_pos, self.level): return True
     *   return False
     */
    for (int b = 0; b < state->num_boxes; b++) {
        if (sokoban_rules_is_deadlock(&env->rules, state->boxes[b], env->level))
            return true;
    }
    return false;
}

bool sokoban_env_is_goal_state(const SokobanEnv *env, const State *state) {
    return sokoban_rules_is_goal_state(state, env->level);
}