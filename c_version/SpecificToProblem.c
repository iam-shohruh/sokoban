#include "GRAPH_SEARCH.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define CLEAR_SCREEN() system("cls")
static void sleep_ms(int ms) { Sleep(ms); }
#else
#define CLEAR_SCREEN() do { const char *term = getenv("TERM"); if (term && *term) system("clear"); } while (0)
static void sleep_ms(int ms) {
    clock_t start = clock();
    while ((double)(clock() - start) * 1000.0 / CLOCKS_PER_SEC < ms) {
    }
}
#endif

static const char *DIR_NAMES[ACTIONS_NUMBER] = {"UP", "DOWN", "LEFT", "RIGHT"};
static const int DR[ACTIONS_NUMBER] = {-1, 1, 0, 0};
static const int DC[ACTIONS_NUMBER] = {0, 0, -1, 1};

const char *dataset_path(const char *dataset) {
    if (strcmp(dataset, "microban") == 0) return "data/parsed/microban/levels.json";
    if (strcmp(dataset, "xsokoban") == 0) return "data/parsed/xsokoban/levels.json";
    return dataset;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);

    char *text = malloc((size_t)n + 1);
    if (!text) {
        fclose(f);
        return NULL;
    }

    if (fread(text, 1, (size_t)n, f) != (size_t)n) {
        free(text);
        fclose(f);
        return NULL;
    }

    text[n] = '\0';
    fclose(f);
    return text;
}

static char *read_dataset_file(const char *path) {
    char alt[512];
    char *text = read_file(path);
    if (text) return text;

    snprintf(alt, sizeof(alt), "../%s", path);
    text = read_file(alt);
    if (text) return text;

    snprintf(alt, sizeof(alt), "../../%s", path);
    text = read_file(alt);
    if (text) return text;

    snprintf(alt, sizeof(alt), "../../../%s", path);
    return read_file(alt);
}

static int json_int(char *from, const char *name) {
    char pat[32];
    snprintf(pat, sizeof(pat), "\"%s\"", name);

    char *p = strstr(from, pat);
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p) return 0;

    return atoi(p + 1);
}

static char *read_json_string(char *p, char *out, int max) {
    int n = 0;
    p++;

    while (*p && *p != '"') {
        if (*p == '\\' && p[1]) p++;
        if (n < max - 1) out[n++] = *p;
        p++;
    }

    out[n] = '\0';
    return *p == '"' ? p + 1 : p;
}

static void fill_level(Level *level, char **grid, int rows) {
    int box_count = 0;
    level->walls = calloc((size_t)(level->width * level->height), sizeof(bool));
    level->goals = calloc((size_t)(level->width * level->height), sizeof(bool));

    for (int r = 0; r < rows; r++) {
        for (int c = 0; grid[r][c]; c++) {
            if (grid[r][c] == '$' || grid[r][c] == '*') box_count++;
        }
    }

    level->init_state.num_boxes = box_count;
    level->init_state.boxes = malloc((size_t)box_count * sizeof(Position));

    int b = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; grid[r][c]; c++) {
            char ch = grid[r][c];
            int idx = r * level->width + c;

            if (ch == '#') level->walls[idx] = true;
            if (ch == '.' || ch == '*' || ch == '+') level->goals[idx] = true;
            if (ch == '$' || ch == '*') level->init_state.boxes[b++] = (Position){r, c};
            if (ch == '@' || ch == '+') level->init_state.player = (Position){r, c};
        }
    }
}

Level *load_levels(const char *filepath, int *out_count) {
    char *text = read_dataset_file(filepath);
    if (!text) {
        printf("Error: could not open %s\n", filepath);
        *out_count = 0;
        return NULL;
    }

    int cap = 16;
    int n = 0;
    Level *levels = malloc((size_t)cap * sizeof(Level));
    char *p = text;

    while ((p = strstr(p, "\"grid\"")) != NULL) {
        char *obj = p;
        while (obj > text && *obj != '{') obj--;

        if (n == cap) {
            cap *= 2;
            Level *new_levels = realloc(levels, (size_t)cap * sizeof(Level));
            if (!new_levels) break;
            levels = new_levels;
        }

        Level level = {0};
        level.game_id = json_int(obj, "id");
        level.width = json_int(obj, "width");
        level.height = json_int(obj, "height");

        p = strchr(p, '[');
        if (!p) break;
        p++;

        int grid_cap = level.height > 0 ? level.height + 1 : 32;
        char **grid = calloc((size_t)grid_cap, sizeof(char *));
        int rows = 0;

        while (*p && *p != ']') {
            if (*p == '"') {
                char row[512];
                p = read_json_string(p, row, sizeof(row));

                if (rows == grid_cap) {
                    grid_cap *= 2;
                    char **new_grid = realloc(grid, (size_t)grid_cap * sizeof(char *));
                    if (!new_grid) break;
                    grid = new_grid;
                }

                grid[rows] = malloc(strlen(row) + 1);
                strcpy(grid[rows], row);
                rows++;
            } else {
                p++;
            }
        }

        if (level.width == 0) {
            for (int i = 0; i < rows; i++) {
                int len = (int)strlen(grid[i]);
                if (len > level.width) level.width = len;
            }
        }

        if (level.height == 0) level.height = rows;

        fill_level(&level, grid, rows);
        levels[n++] = level;

        for (int i = 0; i < rows; i++) free(grid[i]);
        free(grid);
    }

    free(text);
    *out_count = n;
    return levels;
}

void free_level(Level *level) {
    free(level->walls);
    free(level->goals);
    free(level->init_state.boxes);
    level->walls = NULL;
    level->goals = NULL;
    level->init_state.boxes = NULL;
}

int dataset_level_count(const char *dataset) {
    int count = 0;
    Level *levels = load_levels(dataset_path(dataset), &count);
    if (!levels) return 0;

    for (int i = 0; i < count; i++) free_level(&levels[i]);
    free(levels);
    return count;
}

int pos_index(Level *level, Position p) {
    return p.row * level->width + p.col;
}

bool same_pos(Position a, Position b) {
    return a.row == b.row && a.col == b.col;
}

static bool in_bounds(Level *level, Position p) {
    return p.row >= 0 && p.row < level->height && p.col >= 0 && p.col < level->width;
}

static bool is_wall(Level *level, Position p) {
    return !in_bounds(level, p) || level->walls[pos_index(level, p)];
}

bool has_box(State *state, Position p) {
    for (int i = 0; i < state->num_boxes; i++) {
        if (same_pos(state->boxes[i], p)) return true;
    }
    return false;
}

static void sort_boxes(Position *boxes, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (boxes[j].row < boxes[i].row ||
                (boxes[j].row == boxes[i].row && boxes[j].col < boxes[i].col)) {
                Position t = boxes[i];
                boxes[i] = boxes[j];
                boxes[j] = t;
            }
        }
    }
}

State copy_state(State *state) {
    State out = {state->player, NULL, state->num_boxes};
    out.boxes = malloc((size_t)state->num_boxes * sizeof(Position));
    memcpy(out.boxes, state->boxes, (size_t)state->num_boxes * sizeof(Position));
    return out;
}

void free_state(State *state) {
    free(state->boxes);
    state->boxes = NULL;
}

State step_state(State *state, Push push) {
    State out = copy_state(state);
    int dir = 0;
    for (int i = 0; i < ACTIONS_NUMBER; i++) {
        if (strcmp(push.direction, DIR_NAMES[i]) == 0) dir = i;
    }

    Position new_box = {push.position.row + DR[dir], push.position.col + DC[dir]};
    for (int i = 0; i < out.num_boxes; i++) {
        if (same_pos(out.boxes[i], push.position)) {
            out.boxes[i] = new_box;
            break;
        }
    }

    out.player = push.position;
    sort_boxes(out.boxes, out.num_boxes);
    return out;
}

DeadlockType detect_deadlock_type(Position box, Level *level, State *state) {
    (void)state;

    if (in_bounds(level, box) && level->goals[pos_index(level, box)]) {
        return DEADLOCK_NONE;
    }

    bool up = is_wall(level, (Position){box.row - 1, box.col});
    bool down = is_wall(level, (Position){box.row + 1, box.col});
    bool left = is_wall(level, (Position){box.row, box.col - 1});
    bool right = is_wall(level, (Position){box.row, box.col + 1});

    if ((up || down) && (left || right)) {
        return DEADLOCK_CORNER;
    }

    return DEADLOCK_NONE;
}

bool is_deadlock(Position box, Level *level, State *state) {
    return detect_deadlock_type(box, level, state) != DEADLOCK_NONE;
}

bool is_goal_state(State *state, Level *level) {
    for (int i = 0; i < state->num_boxes; i++) {
        if (!in_bounds(level, state->boxes[i]) || !level->goals[pos_index(level, state->boxes[i])]) {
            return false;
        }
    }
    return true;
}

Push *get_valid_pushes(State *state, Level *level, int *count) {
    int cells = level->width * level->height;
    bool *seen = calloc((size_t)cells, sizeof(bool));
    Position *queue = malloc((size_t)cells * sizeof(Position));
    Push *pushes = malloc((size_t)(state->num_boxes * ACTIONS_NUMBER + 1) * sizeof(Push));
    int front = 0, back = 0, n = 0;

    if (in_bounds(level, state->player)) {
        seen[pos_index(level, state->player)] = true;
        queue[back++] = state->player;
    }

    while (front < back) {
        Position cur = queue[front++];
        for (int i = 0; i < ACTIONS_NUMBER; i++) {
            Position next = {cur.row + DR[i], cur.col + DC[i]};
            if (is_wall(level, next)) continue;

            int idx = pos_index(level, next);
            if (seen[idx]) continue;

            if (has_box(state, next)) {
                Position after = {next.row + DR[i], next.col + DC[i]};
                if (!is_wall(level, after) && !has_box(state, after)) {
                    pushes[n].position = next;
                    strcpy(pushes[n].direction, DIR_NAMES[i]);
                    n++;
                }
            } else {
                seen[idx] = true;
                queue[back++] = next;
            }
        }
    }

    free(seen);
    free(queue);
    *count = n;
    return pushes;
}

int manhattan_heuristic(State *state, Level *level) {
    int total = 0;
    bool *taken = calloc((size_t)(level->width * level->height), sizeof(bool));

    for (int i = 0; i < state->num_boxes; i++) {
        int best = INT_MAX;
        int best_idx = -1;

        for (int r = 0; r < level->height; r++) {
            for (int c = 0; c < level->width; c++) {
                Position goal = {r, c};
                int idx = pos_index(level, goal);
                if (level->goals[idx] && !taken[idx]) {
                    int d = abs(state->boxes[i].row - r) + abs(state->boxes[i].col - c);
                    if (d < best) {
                        best = d;
                        best_idx = idx;
                    }
                }
            }
        }

        if (best != INT_MAX) total += best;
        if (best_idx >= 0) taken[best_idx] = true;
    }

    free(taken);
    return total;
}

int hungarian_heuristic(State *state, Level *level) {
    int n = state->num_boxes;
    if (n <= 0) return 0;

    int goal_count = 0;
    for (int r = 0; r < level->height; r++) {
        for (int c = 0; c < level->width; c++) {
            if (level->goals[r * level->width + c]) goal_count++;
        }
    }

    if (goal_count < n) {
        return manhattan_heuristic(state, level);
    }

    Position *goals = malloc((size_t)goal_count * sizeof(Position));
    int gi = 0;
    for (int r = 0; r < level->height; r++) {
        for (int c = 0; c < level->width; c++) {
            if (level->goals[r * level->width + c]) {
                goals[gi++] = (Position){r, c};
            }
        }
    }

    int *u = calloc((size_t)(n + 1), sizeof(int));
    int *v = calloc((size_t)(goal_count + 1), sizeof(int));
    int *p = calloc((size_t)(goal_count + 1), sizeof(int));
    int *way = calloc((size_t)(goal_count + 1), sizeof(int));
    int *minv = malloc((size_t)(goal_count + 1) * sizeof(int));
    bool *used = calloc((size_t)(goal_count + 1), sizeof(bool));

    for (int i = 1; i <= n; i++) {
        p[0] = i;
        for (int j = 1; j <= goal_count; j++) {
            minv[j] = INT_MAX;
            used[j] = false;
        }
        used[0] = true;

        int j0 = 0;
        do {
            used[j0] = true;
            int i0 = p[j0];
            int delta = INT_MAX;
            int j1 = 0;

            for (int j = 1; j <= goal_count; j++) {
                if (used[j]) continue;

                int cur = abs(state->boxes[i0 - 1].row - goals[j - 1].row) +
                          abs(state->boxes[i0 - 1].col - goals[j - 1].col) -
                          u[i0] - v[j];
                if (cur < minv[j]) {
                    minv[j] = cur;
                    way[j] = j0;
                }
                if (minv[j] < delta) {
                    delta = minv[j];
                    j1 = j;
                }
            }

            for (int j = 0; j <= goal_count; j++) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }

            j0 = j1;
        } while (p[j0] != 0);

        do {
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    int total = 0;
    for (int j = 1; j <= goal_count; j++) {
        if (p[j] != 0) {
            total += abs(state->boxes[p[j] - 1].row - goals[j - 1].row) +
                     abs(state->boxes[p[j] - 1].col - goals[j - 1].col);
        }
    }

    free(goals);
    free(u);
    free(v);
    free(p);
    free(way);
    free(minv);
    free(used);

    return total;
}

char *state_key(State *state) {
    State tmp = copy_state(state);
    sort_boxes(tmp.boxes, tmp.num_boxes);
    size_t size = 32 + (size_t)tmp.num_boxes * 24;
    char *key = malloc(size);
    int used = snprintf(key, size, "%d,%d:", tmp.player.row, tmp.player.col);

    for (int i = 0; i < tmp.num_boxes; i++) {
        used += snprintf(key + used, size - (size_t)used, "%d,%d;", tmp.boxes[i].row, tmp.boxes[i].col);
    }

    free_state(&tmp);
    return key;
}

static void print_board(Level *level, State *state) {
    for (int r = 0; r < level->height; r++) {
        for (int c = 0; c < level->width; c++) {
            Position p = {r, c};
            int idx = pos_index(level, p);

            bool wall = level->walls[idx];
            bool goal = level->goals[idx];
            bool box = has_box(state, p);
            bool player = same_pos(state->player, p);

            if (wall) putchar('#');
            else if (player && goal) putchar('+');
            else if (player) putchar('@');
            else if (box && goal) putchar('*');
            else if (box) putchar('$');
            else if (goal) putchar('.');
            else putchar(' ');
        }
        putchar('\n');
    }
}

void animate_solution(Level *level, SolverResult *result, int delay_ms) {
    State current = copy_state(&level->init_state);

    CLEAR_SCREEN();
    printf("Computer solving level %d...\n", level->game_id);
    printf("Step 0 / %d\n\n", result->path_len);
    print_board(level, &current);
    sleep_ms(delay_ms);

    for (int i = 0; i < result->path_len; i++) {
        State next = step_state(&current, result->path[i]);
        free_state(&current);
        current = next;

        CLEAR_SCREEN();
        printf("Computer solving level %d...\n", level->game_id);
        printf("Step %d / %d | Push: %s\n\n", i + 1, result->path_len, result->path[i].direction);
        print_board(level, &current);
        sleep_ms(delay_ms);
    }

    free_state(&current);
}
