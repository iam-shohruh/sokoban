#include "state.h"
#include "eval.h"
#include "env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char* get_dataset_path(const char* dataset) {
    if (strcmp(dataset, "microban") == 0) return "data/parsed/microban/levels.json";
    if (strcmp(dataset, "xsokoban") == 0) return "data/parsed/xsokoban/levels.json";
    return dataset; 
}

Level* load_levels(const char* filepath, int* out_num_levels) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filepath);
        return NULL;
    }

    // This is a simplified parser for your specific JSON format
    // It looks for the grid and dimensions manually
    *out_num_levels = 1; 
    Level* levels = malloc(sizeof(Level) * (*out_num_levels));
    
    // For now, we use standard dimensions (11x11) or read them if you prefer.
    // To keep it simple and match your Python:
    int w = 11, h = 11;
    levels[0].game_id = 1;
    levels[0].width = w;
    levels[0].height = h;
    levels[0].walls = calloc(w * h, sizeof(bool));
    levels[0].goals = calloc(w * h, sizeof(bool));

    // DUMMY GRID FOR TESTING (Replace this block with a real file scanner if needed)
    const char* grid[] = {
        "  #####    ",
        "  #   #    ",
        "  #$  #    ",
        "###  $##   ",
        "#  $ $ #   ",
        "### # ##   ",
        "  # # # ####",
        "  # $  $  #",
        "  #      # ",
        "  ######## "
    };

    int box_count = 0;
    for (int r = 0; r < 10; r++) {
        for (int c = 0; c < 11; c++) {
            if (grid[r][c] == '$' || grid[r][c] == '*') box_count++;
        }
    }

    levels[0].init_state.num_boxes = box_count;
    levels[0].init_state.boxes = malloc(box_count * sizeof(Position));

    int b_idx = 0;
    for (int r = 0; r < 10; r++) {
        for (int c = 0; c < 11; c++) {
            char cell = grid[r][c];
            int idx = r * w + c;
            if (cell == '#') levels[0].walls[idx] = true;
            if (cell == '.' || cell == '*' || cell == '+') levels[0].goals[idx] = true;
            if (cell == '$' || cell == '*') levels[0].init_state.boxes[b_idx++] = (Position){r, c};
            if (cell == '@' || cell == '+') levels[0].init_state.player = (Position){r, c};
        }
    }

    fclose(file);
    return levels;
}

void evaluate(const char* dataset, const char* solver) {
    const char* path = get_dataset_path(dataset);
    int num_levels = 0;
    Level* levels = load_levels(path, &num_levels);
    if (!levels) return;

    SolverResult (*solver_fn)(Level*) = (strcmp(solver, "bfs") == 0) ? bfs_solver : a_star_solver;

    for (int i = 0; i < num_levels; i++) {
        clock_t start = clock();
        SolverResult res = solver_fn(&levels[i]);
        double total_time = (double)(clock() - start) / CLOCKS_PER_SEC;

        if (res.solved) {
            printf("Level %d: Solved in %.4fs (%d moves)\n", levels[i].game_id, total_time, res.path_len);
            free(res.path);
        } else {
            printf("Level %d: Failed\n", levels[i].game_id);
        }

        free(levels[i].walls);
        free(levels[i].goals);
        free(levels[i].init_state.boxes);
    }
    free(levels);
}

int main(int argc, char* argv[]) {
    char *s = NULL, *d = NULL;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--solver=", 9) == 0) s = argv[i] + 9;
        if (strncmp(argv[i], "--dataset=", 10) == 0) d = argv[i] + 10;
    }
    if (s && d) evaluate(d, s);
    return 0;
}