#include "eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *dataset_path(const char *dataset) {
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
    if (!text) { fclose(f); return NULL; }
    fread(text, 1, (size_t)n, f);
    text[n] = '\0';
    fclose(f);
    return text;
}

static int json_int(char *from, const char *name) {
    char pat[32];
    snprintf(pat, sizeof(pat), "\"%s\"", name);
    char *p = strstr(from, pat);
    if (!p) return 0;
    p = strchr(p, ':');
    return p ? atoi(p + 1) : 0;
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
    for (int r = 0; r < rows; r++) for (int c = 0; grid[r][c]; c++)
        if (grid[r][c] == '$' || grid[r][c] == '*') box_count++;
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
    char *text = read_file(filepath);
    char alt[512];
    if (!text) {
        snprintf(alt, sizeof(alt), "../../../%s", filepath);
        text = read_file(alt);
    }
    if (!text) { printf("Error: could not open %s\n", filepath); return NULL; }
    int cap = 16, n = 0;
    Level *levels = malloc((size_t)cap * sizeof(Level));
    char *p = text;
    while ((p = strstr(p, "\"grid\"")) != NULL) {
        char *obj = p;
        while (obj > text && *obj != '{') obj--;
        if (n == cap) { cap *= 2; levels = realloc(levels, (size_t)cap * sizeof(Level)); }
        Level level = {0};
        level.game_id = json_int(obj, "id");
        level.width = json_int(obj, "width");
        level.height = json_int(obj, "height");
        p = strchr(p, '[');
        if (!p) break;
        p++;
        char **grid = calloc((size_t)(level.height + 1), sizeof(char*));
        int rows = 0;
        while (*p && *p != ']') {
            if (*p == '"') {
                char row[512];
                p = read_json_string(p, row, sizeof(row));
                grid[rows] = malloc(strlen(row) + 1); strcpy(grid[rows++], row);
            } else p++;
        }
        if (level.width == 0) for (int i = 0; i < rows; i++) { int len = (int)strlen(grid[i]); if (len > level.width) level.width = len; }
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
}

void evaluate(const char *dataset, const char *solver) {
    int count = 0;
    Level *levels = load_levels(dataset_path(dataset), &count);
    if (!levels) return;
    SolverResult (*solve)(Level *) = strcmp(solver, "bfs") == 0 ? bfs_solver : a_star_solver;
    printf("Evaluating solver '%s' on dataset '%s'...\n", solver, dataset);
    for (int i = 0; i < count; i++) {
        clock_t start = clock();
        SolverResult result = solve(&levels[i]);
        double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
        printf("Level %d: %s in %.4fs (%d moves)\n", levels[i].game_id, result.solved ? "Solved" : "Failed", seconds, result.path_len);
        free(result.path);
    }
    for (int i = 0; i < count; i++) free_level(&levels[i]);
    free(levels);
}

int main(int argc, char **argv) {
    const char *dataset = "microban";
    const char *solver = "a_star";
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--dataset=", 10) == 0) dataset = argv[i] + 10;
        else if (strncmp(argv[i], "--solver=", 9) == 0) solver = argv[i] + 9;
    }
    evaluate(dataset, solver);
    return 0;
}
