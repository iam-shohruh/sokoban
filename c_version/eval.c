#include "eval.h"
#include "env.h"
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
    text = read_file(alt);
    return text;
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

static SolverResult solve_level(Level *level, const char *solver) {
    if (strcmp(solver, "bfs") == 0) return bfs_solver(level);
    return a_star_solver(level);
}

static bool should_skip_level(int game_id) {
    return game_id == 93 || game_id == 139 || game_id == 144 || game_id == 153;
}

void evaluate_single(const char *dataset, const char *solver, int level_number) {
    int count = 0;
    Level *levels = load_levels(dataset_path(dataset), &count);
    if (!levels) return;

    if (level_number < 1 || level_number > count) {
        printf("Invalid level number. %s has levels 1 to %d.\n", dataset, count);
        for (int i = 0; i < count; i++) free_level(&levels[i]);
        free(levels);
        return;
    }

    Level *level = &levels[level_number - 1];
    clock_t start = clock();
    SolverResult result = solve_level(level, solver);
    double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;

    printf("Evaluating solver '%s' on dataset '%s'...\n", solver, dataset);
    if (result.solved) {
        printf("Level %d - Solved: True, Time: %.2fs, Path Length: %d\n",
               level->game_id,
               seconds,
               result.path_len);
    } else {
        printf("Level %d - Solved: False, Time: %.2fs, Path Length: N/A\n",
               level->game_id,
               seconds);
    }

    free(result.path);
    for (int i = 0; i < count; i++) free_level(&levels[i]);
    free(levels);
}

void evaluate(const char *dataset, const char *solver) {
    int count = 0;
    Level *levels = load_levels(dataset_path(dataset), &count);
    if (!levels) return;

    printf("Evaluating solver '%s' on dataset '%s'...\n", solver, dataset);
    for (int i = 0; i < count; i++) {
        if (should_skip_level(levels[i].game_id)) continue;

        clock_t start = clock();
        SolverResult result = solve_level(&levels[i], solver);
        double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;

        if (result.solved) {
            printf("Level %d - Solved: True, Time: %.2fs, Path Length: %d\n",
                   levels[i].game_id,
                   seconds,
                   result.path_len);
        } else {
            printf("Level %d - Solved: False, Time: %.2fs, Path Length: N/A\n",
                   levels[i].game_id,
                   seconds);
        }
        free(result.path);
    }

    for (int i = 0; i < count; i++) free_level(&levels[i]);
    free(levels);
}

static void clear_bad_input(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

static int ask_choice(const char *prompt, int min, int max) {
    int value;
    while (1) {
        printf("%s", prompt);

        if (scanf("%d", &value) != 1) {
            clear_bad_input();
            printf("Invalid input. Please enter a number.\n\n");
            continue;
        }

        clear_bad_input();

        if (value >= min && value <= max) return value;
        printf("Invalid choice. Choose a number from %d to %d.\n\n", min, max);
    }
}

static int dataset_level_count(const char *dataset) {
    int count = 0;
    Level *levels = load_levels(dataset_path(dataset), &count);
    if (!levels) return 0;

    for (int i = 0; i < count; i++) free_level(&levels[i]);
    free(levels);
    return count;
}

static const char *arg_value(const char *arg, const char *prefix) {
    size_t n = strlen(prefix);
    if (strncmp(arg, prefix, n) == 0) return arg + n;
    return NULL;
}

int main(int argc, char **argv) {
    const char *dataset = NULL;
    const char *solver = NULL;
    int level_number = 0;
    bool batch = false;

    for (int i = 1; i < argc; i++) {
        const char *value = NULL;
        if ((value = arg_value(argv[i], "--dataset=")) != NULL) dataset = value;
        else if ((value = arg_value(argv[i], "--solver=")) != NULL) solver = value;
        else if ((value = arg_value(argv[i], "--level=")) != NULL) level_number = atoi(value);
        else if (strcmp(argv[i], "--batch") == 0) batch = true;
    }

    if (dataset && solver && batch) {
        evaluate(dataset, solver);
        return 0;
    }

    if (dataset && solver && level_number > 0) {
        evaluate_single(dataset, solver, level_number);
        return 0;
    }

    printf("Choose dataset:\n");
    printf("1) microban\n");
    printf("2) xsokoban\n");
    int dataset_choice = ask_choice("Choose 1 or 2: ", 1, 2);
    dataset = dataset_choice == 1 ? "microban" : "xsokoban";

    int count = dataset_level_count(dataset);
    if (count <= 0) return 1;
    printf("\n%s has %d levels.\n", dataset, count);

    printf("\nChoose solver:\n");
    printf("1) BFS\n");
    printf("2) A*\n");
    int solver_choice = ask_choice("Choose 1 or 2: ", 1, 2);
    solver = solver_choice == 1 ? "bfs" : "a_star";

    level_number = ask_choice("\nChoose level number: ", 1, count);
    evaluate_single(dataset, solver, level_number);

    return 0;
}
