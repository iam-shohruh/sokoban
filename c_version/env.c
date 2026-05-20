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

static int hungarian_min_cost(const int *cost, int rows, int cols) {
    if (rows <= 0 || cols <= 0) return 0;

    if (rows > cols) {
        int *transposed = malloc((size_t)rows * (size_t)cols * sizeof(int));
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                transposed[c * rows + r] = cost[r * cols + c];
            }
        }
        int out = hungarian_min_cost(transposed, cols, rows);
        free(transposed);
        return out;
    }

    int *u = calloc((size_t)(rows + 1), sizeof(int));
    int *v = calloc((size_t)(cols + 1), sizeof(int));
    int *p = calloc((size_t)(cols + 1), sizeof(int));
    int *way = calloc((size_t)(cols + 1), sizeof(int));
    int *minv = malloc((size_t)(cols + 1) * sizeof(int));
    bool *used = malloc((size_t)(cols + 1) * sizeof(bool));

    for (int i = 1; i <= rows; i++) {
        p[0] = i;
        for (int j = 0; j <= cols; j++) {
            minv[j] = INT_MAX / 4;
            used[j] = false;
        }

        int j0 = 0;
        while (1) {
            used[j0] = true;
            int i0 = p[j0];
            int delta = INT_MAX / 4;
            int j1 = 0;

            for (int j = 1; j <= cols; j++) {
                if (used[j]) continue;
                int cur = cost[(i0 - 1) * cols + (j - 1)] - u[i0] - v[j];
                if (cur < minv[j]) {
                    minv[j] = cur;
                    way[j] = j0;
                }
                if (minv[j] < delta) {
                    delta = minv[j];
                    j1 = j;
                }
            }

            for (int j = 0; j <= cols; j++) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }

            j0 = j1;
            if (p[j0] == 0) break;
        }

        while (1) {
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
            if (j0 == 0) break;
        }
    }

    int assignment_cost = 0;
    for (int j = 1; j <= cols; j++) {
        int i = p[j];
        if (i != 0) assignment_cost += cost[(i - 1) * cols + (j - 1)];
    }

    free(u);
    free(v);
    free(p);
    free(way);
    free(minv);
    free(used);
    return assignment_cost;
}

int hungarian_heuristic(State *state, Level *level) {
    int num_boxes = state->num_boxes;
    if (num_boxes == 0) return 0;

    int max_goals = level->width * level->height;
    Position *goals = malloc((size_t)max_goals * sizeof(Position));
    int goal_count = 0;
    for (int r = 0; r < level->height; r++) {
        for (int c = 0; c < level->width; c++) {
            Position p = {r, c};
            if (level->goals[pos_index(level, p)]) goals[goal_count++] = p;
        }
    }

    if (goal_count == 0) {
        free(goals);
        return 0;
    }

    int *cost = malloc((size_t)num_boxes * (size_t)goal_count * sizeof(int));
    for (int i = 0; i < num_boxes; i++) {
        Position box = state->boxes[i];
        for (int j = 0; j < goal_count; j++) {
            Position goal = goals[j];
            int dist = abs(box.row - goal.row) + abs(box.col - goal.col);
            cost[i * goal_count + j] = dist;
        }
    }

    int out = hungarian_min_cost(cost, num_boxes, goal_count);
    free(cost);
    free(goals);
    return out;
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
