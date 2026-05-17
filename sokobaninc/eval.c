#include "eval.h"
#include "search/a_star.h"
#include "search/bfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* DATASETS / SOLVERS tables  (mirrors Python dicts)                   */
/* ------------------------------------------------------------------ */

const char *DATASETS[][2] = {
    { "microban", "data/parsed/microban/levels.json" },
    { "xsokoban", "data/parsed/xsokoban/levels.json" },
    { NULL,        NULL }
};

const char *SOLVERS[] = { "bfs", "a_star", NULL };

/* ------------------------------------------------------------------ */
/* Minimal JSON helpers for level loading                              */
/* ------------------------------------------------------------------ */

/* Skip whitespace */
static const char *json_skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

/* Read a JSON integer */
static const char *json_read_int(const char *p, int *out) {
    p = json_skip_ws(p);
    int sign = 1;
    if (*p == '-') { sign = -1; p++; }
    *out = 0;
    while (*p >= '0' && *p <= '9') { *out = *out * 10 + (*p - '0'); p++; }
    *out *= sign;
    return p;
}

/* Read a JSON string into buf (up to buf_size-1 chars) */
static const char *json_read_string(const char *p, char *buf, int buf_size) {
    p = json_skip_ws(p);
    if (*p != '"') { buf[0] = '\0'; return p; }
    p++; /* skip opening quote */
    int i = 0;
    while (*p && *p != '"') {
        if (*p == '\\') { p++; } /* skip escape */
        if (i < buf_size - 1) buf[i++] = *p;
        p++;
    }
    buf[i] = '\0';
    if (*p == '"') p++;
    return p;
}

/* Find the next occurrence of key:"..." or key: in the JSON */
static const char *json_find_key(const char *p, const char *key) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *found = strstr(p, search);
    if (!found) return NULL;
    found += strlen(search);
    found = json_skip_ws(found);
    if (*found == ':') found++;
    return json_skip_ws(found);
}

/* ------------------------------------------------------------------ */
/* load_levels                                                          */
/* ------------------------------------------------------------------ */

Level *load_levels(const char *dataset_path, int *out_count) {
    /*
     * Python:
     *   with open(dataset) as f: dataset_json = json.load(f)
     *   for level_data in dataset_json["levels"]:
     *       level_dict = {"walls":[],"goals":[],"player":None,"boxes":[]}
     *       for j,row in enumerate(level_data["grid"]):
     *           for i,cell in enumerate(row):
     *               if cell=="#": walls.append(Position(j,i))
     *               elif cell=="@": player = Position(j,i)
     *               elif cell=="+": goals.append(...); player=...
     *               elif cell=="$": boxes.append(...)
     *               elif cell=="*": goals.append(...); boxes.append(...)
     *               elif cell==".": goals.append(...)
     *       level = Level(game_id=..., width=..., height=..., walls=frozenset(...),
     *                     goals=frozenset(...), init_state=State(player=...,boxes=frozenset(...)))
     */

    FILE *f = fopen(dataset_path, "r");
    if (!f) {
        fprintf(stderr, "Cannot open '%s'\n", dataset_path);
        *out_count = 0;
        return NULL;
    }

    /* Read entire file */
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);
    char *buf = (char *)malloc(fsize + 1);
    fread(buf, 1, fsize, f);
    buf[fsize] = '\0';
    fclose(f);

    /* Count levels */
    int   levels_cap  = 256;
    int   levels_size = 0;
    Level *levels     = (Level *)malloc(levels_cap * sizeof(Level));

    /* Find "levels" array */
    const char *p = json_find_key(buf, "levels");
    if (!p || *p != '[') { free(buf); *out_count = 0; return levels; }
    p++; /* skip '[' */

    while (1) {
        p = json_skip_ws(p);
        if (*p == ']') break;
        if (*p == ',') { p++; p = json_skip_ws(p); }
        if (*p != '{') break;

        /* Temp storage for one level */
        int        game_id = 0, width = 0, height = 0;
        Position   walls_tmp[4096],  goals_tmp[256],  boxes_tmp[256];
        int        num_walls = 0, num_goals = 0, num_boxes = 0;
        Position   player = {0, 0};

        /* Find "id" */
        const char *id_p = json_find_key(p, "id");
        if (id_p) json_read_int(id_p, &game_id);

        /* Find "width" */
        const char *w_p = json_find_key(p, "width");
        if (w_p) json_read_int(w_p, &width);

        /* Find "height" */
        const char *h_p = json_find_key(p, "height");
        if (h_p) json_read_int(h_p, &height);

        /* Find "grid" array */
        const char *grid_p = json_find_key(p, "grid");
        if (grid_p && *grid_p == '[') {
            grid_p++;
            int row_idx = 0;
            while (*grid_p && *grid_p != ']') {
                grid_p = json_skip_ws(grid_p);
                if (*grid_p == ',') { grid_p++; continue; }
                if (*grid_p == ']') break;
                char row_buf[256];
                grid_p = json_read_string(grid_p, row_buf, sizeof(row_buf));
                int col_idx = 0;
                for (int ci = 0; row_buf[ci]; ci++) {
                    char cell = row_buf[ci];
                    if (cell == '#') {
                        walls_tmp[num_walls++] = (Position){row_idx, col_idx};
                    } else if (cell == '@') {
                        player = (Position){row_idx, col_idx};
                    } else if (cell == '+') {
                        goals_tmp[num_goals++] = (Position){row_idx, col_idx};
                        player = (Position){row_idx, col_idx};
                    } else if (cell == '$') {
                        boxes_tmp[num_boxes++] = (Position){row_idx, col_idx};
                    } else if (cell == '*') {
                        goals_tmp[num_goals++] = (Position){row_idx, col_idx};
                        boxes_tmp[num_boxes++] = (Position){row_idx, col_idx};
                    } else if (cell == '.') {
                        goals_tmp[num_goals++] = (Position){row_idx, col_idx};
                    }
                    col_idx++;
                }
                row_idx++;
            }
        }

        /* Find closing brace of this level object */
        int depth = 1;
        while (*p && depth > 0) {
            if      (*p == '{') depth++;
            else if (*p == '}') depth--;
            p++;
        }

        /* Build Level */
        Level lvl;
        lvl.game_id   = game_id;
        lvl.width     = width;
        lvl.height    = height;
        lvl.num_walls = num_walls;
        lvl.num_goals = num_goals;
        lvl.walls     = (Position *)malloc(num_walls * sizeof(Position));
        lvl.goals     = (Position *)malloc(num_goals * sizeof(Position));
        memcpy(lvl.walls, walls_tmp, num_walls * sizeof(Position));
        memcpy(lvl.goals, goals_tmp, num_goals * sizeof(Position));

        State init_state;
        init_state.player    = player;
        init_state.num_boxes = num_boxes;
        init_state.boxes     = (Position *)malloc(num_boxes * sizeof(Position));
        memcpy(init_state.boxes, boxes_tmp, num_boxes * sizeof(Position));
        state_sort_boxes(&init_state);

        lvl.init_state = init_state;

        if (levels_size == levels_cap) {
            levels_cap *= 2;
            levels = (Level *)realloc(levels, levels_cap * sizeof(Level));
        }
        levels[levels_size++] = lvl;
    }

    free(buf);
    *out_count = levels_size;
    return levels;
}

/* ------------------------------------------------------------------ */
/* Stubs mirroring Python placeholders                                 */
/* ------------------------------------------------------------------ */

void _save(const char *dataset, const char *solver) {
    /* placeholder – mirrors Python pass */
    (void)dataset; (void)solver;
}

void _summarize(void) {
    /* placeholder – mirrors Python return {} */
}

/* ------------------------------------------------------------------ */
/* evaluate                                                             */
/* ------------------------------------------------------------------ */

void evaluate(const char *dataset, const char *solver) {
    /*
     * Python:
     *   if dataset not in DATASETS: print(...)
     *   if solver not in SOLVERS: print(...)
     *   levels = load_levels(DATASETS[dataset])
     *   solver_fn = SOLVERS[solver]
     *   results = []
     *   for level in levels:
     *       start = time.time()
     *       result = solver_fn(level)
     *       end = time.time()
     *       result["time"] = end - start
     *       results.append(result)
     */

    /* Find dataset path */
    const char *dataset_path = NULL;
    for (int i = 0; DATASETS[i][0] != NULL; i++) {
        if (strcmp(DATASETS[i][0], dataset) == 0) {
            dataset_path = DATASETS[i][1];
            break;
        }
    }
    if (!dataset_path) {
        fprintf(stderr, "Dataset '%s' not found.\n", dataset);
        return;
    }

    /* Check solver name */
    bool solver_valid = false;
    for (int i = 0; SOLVERS[i] != NULL; i++) {
        if (strcmp(SOLVERS[i], solver) == 0) { solver_valid = true; break; }
    }
    if (!solver_valid) {
        fprintf(stderr, "Solver '%s' not found.\n", solver);
        return;
    }

    printf("Evaluating solver '%s' on dataset '%s'...\n", solver, dataset);

    int    num_levels = 0;
    Level *levels     = load_levels(dataset_path, &num_levels);

    for (int i = 0; i < num_levels; i++) {
        clock_t start = clock();

        SolverResult result;
        if (strcmp(solver, "bfs") == 0) {
            result = bfs_solver(&levels[i]);
        } else {
            result = a_star_solver(&levels[i], false);
        }

        clock_t end  = clock();
        double  secs = (double)(end - start) / CLOCKS_PER_SEC;
        printf("Level %d: solved=%s time=%.3fs path_len=%d\n",
               levels[i].game_id,
               result.solved ? "true" : "false",
               secs,
               result.path_len);

        solver_result_free(&result);
        level_free(&levels[i]);
    }

    free(levels);
}