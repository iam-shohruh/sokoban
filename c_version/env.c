#include "env.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

static const char *DIRS[] = {"UP", "DOWN", "LEFT", "RIGHT"};
static const int DR[] = {-1, 1, 0, 0};
static const int DC[] = {0, 0, -1, 1};

int pos_index(Level *level, Position p) { return p.row * level->width + p.col; }
bool same_pos(Position a, Position b) { return a.row == b.row && a.col == b.col; }

static bool in_bounds(Level *level, Position p) {
    return p.row >= 0 && p.row < level->height && p.col >= 0 && p.col < level->width;
}

static bool is_wall(Level *level, Position p) {
    return !in_bounds(level, p) || level->walls[pos_index(level, p)];
}

bool has_box(State *s, Position p) {
    for (int i = 0; i < s->num_boxes; i++) if (same_pos(s->boxes[i], p)) return true;
    return false;
}

static void sort_boxes(Position *boxes, int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (boxes[j].row < boxes[i].row || (boxes[j].row == boxes[i].row && boxes[j].col < boxes[i].col)) {
                Position t = boxes[i]; boxes[i] = boxes[j]; boxes[j] = t;
            }
}

State copy_state(State *s) {
    State out = {s->player, NULL, s->num_boxes};
    out.boxes = malloc((size_t)s->num_boxes * sizeof(Position));
    memcpy(out.boxes, s->boxes, (size_t)s->num_boxes * sizeof(Position));
    return out;
}

void free_state(State *s) { free(s->boxes); s->boxes = NULL; }

State step_state(State *state, Push push) {
    State out = copy_state(state);
    int dir = 0;
    for (int i = 0; i < 4; i++) if (strcmp(push.direction, DIRS[i]) == 0) dir = i;
    Position new_box = {push.position.row + DR[dir], push.position.col + DC[dir]};
    for (int i = 0; i < out.num_boxes; i++) {
        if (same_pos(out.boxes[i], push.position)) { out.boxes[i] = new_box; break; }
    }
    out.player = push.position;
    sort_boxes(out.boxes, out.num_boxes);
    return out;
}

bool is_deadlock(Position box, Level *level, State *state) {
    (void)state;

    if (in_bounds(level, box) && level->goals[pos_index(level, box)]) {
        return false;
    }

    bool up = is_wall(level, (Position){box.row - 1, box.col});
    bool down = is_wall(level, (Position){box.row + 1, box.col});
    bool left = is_wall(level, (Position){box.row, box.col - 1});
    bool right = is_wall(level, (Position){box.row, box.col + 1});

    if ((up || down) && (left || right)) {
        return true;
    }

    return false;
}

bool is_goal_state(State *state, Level *level) {
    for (int i = 0; i < state->num_boxes; i++)
        if (!in_bounds(level, state->boxes[i]) || !level->goals[pos_index(level, state->boxes[i])]) return false;
    return true;
}

Push *get_valid_pushes(State *state, Level *level, int *count) {
    int cells = level->width * level->height;
    bool *seen = calloc((size_t)cells, sizeof(bool));
    Position *queue = malloc((size_t)cells * sizeof(Position));
    Push *pushes = malloc((size_t)(state->num_boxes * 4 + 1) * sizeof(Push));
    int front = 0, back = 0, n = 0;
    if (in_bounds(level, state->player)) {
        seen[pos_index(level, state->player)] = true;
        queue[back++] = state->player;
    }
    while (front < back) {
        Position cur = queue[front++];
        for (int i = 0; i < 4; i++) {
            Position next = {cur.row + DR[i], cur.col + DC[i]};
            if (is_wall(level, next)) continue;
            int idx = pos_index(level, next);
            if (seen[idx]) continue;
            if (has_box(state, next)) {
                Position after = {next.row + DR[i], next.col + DC[i]};
                if (!is_wall(level, after) && !has_box(state, after)) {
                    pushes[n].position = next;
                    strcpy(pushes[n].direction, DIRS[i]);
                    n++;
                }
            } else {
                seen[idx] = true;
                queue[back++] = next;
            }
        }
    }
    free(seen); free(queue);
    *count = n;
    return pushes;
}

int manhattan_heuristic(State *state, Level *level) {
    int total = 0;
    bool *taken = calloc((size_t)(level->width * level->height), sizeof(bool));
    for (int i = 0; i < state->num_boxes; i++) {
        int best = INT_MAX, best_idx = -1;
        for (int r = 0; r < level->height; r++) for (int c = 0; c < level->width; c++) {
            Position g = {r, c}; int idx = pos_index(level, g);
            if (level->goals[idx] && !taken[idx]) {
                int d = abs(state->boxes[i].row - r) + abs(state->boxes[i].col - c);
                if (d < best) { best = d; best_idx = idx; }
            }
        }
        if (best != INT_MAX) total += best;
        if (best_idx >= 0) taken[best_idx] = true;
    }
    free(taken);
    return total;
}

char *state_key(State *state) {
    State tmp = copy_state(state);
    sort_boxes(tmp.boxes, tmp.num_boxes);
    size_t size = 32 + (size_t)tmp.num_boxes * 24;
    char *key = malloc(size); int used = snprintf(key, size, "%d,%d:", tmp.player.row, tmp.player.col);
    for (int i = 0; i < tmp.num_boxes; i++)
        used += snprintf(key + used, size - (size_t)used, "%d,%d;", tmp.boxes[i].row, tmp.boxes[i].col);
    free_state(&tmp);
    return key;
}
