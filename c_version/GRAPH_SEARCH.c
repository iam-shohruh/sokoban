#include "GRAPH_SEARCH.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static SolverResult solve_level(Level *level, const char *solver) {
    clock_t start = clock();
    SolverResult result;

    if (strcmp(solver, "bfs") == 0) {
        result = First_InsertFrontier_Search_TREE(level);
    } else {
        result = Insert_Priority_Queue_GENERALIZED_A_Star(level);
    }

    result.solve_time_sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    return result;
}

static void print_single_human(const char *dataset, const char *solver, int count, Level *level, SolverResult *result) {
    printf("\nDataset: %s (%d levels)\n", dataset, count);
    printf("Solver: %s\n", strcmp(solver, "bfs") == 0 ? "BFS" : "A*");
    printf("Level %d: %s\n", level->game_id, result->solved ? "Solved" : "Failed");
    printf("Time to solve: %.4fs\n", result->solve_time_sec);
    printf("Path length: %d pushes\n", result->path_len);
    printf("Nodes visited: %lld\n", result->nodes_visited);
    printf("Deadlocks detected: %lld\n", result->deadlocks_detected);
    printf("Deadlock by type: corner=%lld freeze=%lld tunnel=%lld other=%lld\n",
           result->deadlock_corner_detected,
           result->deadlock_freeze_detected,
           result->deadlock_tunnel_detected,
           result->deadlock_other_detected);
}

static void print_csv_header(void) {
    printf("solver,level_id,solved,path_len,solve_time_sec,nodes_visited,deadlocks_detected,deadlock_corner_detected,deadlock_freeze_detected,deadlock_tunnel_detected,deadlock_other_detected\n");
}

static void print_csv_row(const char *solver, int level_id, SolverResult *result) {
    printf("%s,%d,%d,%d,%.6f,%lld,%lld,%lld,%lld,%lld,%lld\n",
           solver,
           level_id,
           result->solved ? 1 : 0,
           result->path_len,
           result->solve_time_sec,
           result->nodes_visited,
           result->deadlocks_detected,
           result->deadlock_corner_detected,
           result->deadlock_freeze_detected,
           result->deadlock_tunnel_detected,
           result->deadlock_other_detected);
}

void evaluate_single(const char *dataset, const char *solver, int level_number, bool animate) {
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
    SolverResult result = solve_level(level, solver);

    if (result.solved && animate) {
        animate_solution(level, &result, 500);
    }

    print_single_human(dataset, solver, count, level, &result);

    free(result.path);
    for (int i = 0; i < count; i++) free_level(&levels[i]);
    free(levels);
}

void evaluate(const char *dataset, const char *solver) {
    int count = 0;
    Level *levels = load_levels(dataset_path(dataset), &count);
    if (!levels) return;

    printf("Evaluating %s with %s (%d levels).\n", dataset, strcmp(solver, "bfs") == 0 ? "BFS" : "A*", count);

    for (int i = 0; i < count; i++) {
        SolverResult result = solve_level(&levels[i], solver);

        printf("Level %d: %s | %.4fs | %d pushes | visited=%lld deadlocks=%lld (corner=%lld freeze=%lld tunnel=%lld other=%lld)\n",
               levels[i].game_id,
               result.solved ? "Solved" : "Failed",
               result.solve_time_sec,
               result.path_len,
               result.nodes_visited,
               result.deadlocks_detected,
               result.deadlock_corner_detected,
               result.deadlock_freeze_detected,
               result.deadlock_tunnel_detected,
               result.deadlock_other_detected);
        free(result.path);
    }

    for (int i = 0; i < count; i++) free_level(&levels[i]);
    free(levels);
}

void evaluate_csv(const char *dataset, const char *solver) {
    int count = 0;
    Level *levels = load_levels(dataset_path(dataset), &count);
    if (!levels) return;

    print_csv_header();
    for (int i = 0; i < count; i++) {
        SolverResult result = solve_level(&levels[i], solver);
        print_csv_row(solver, levels[i].game_id, &result);
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

        if (value >= min && value <= max) {
            return value;
        }

        printf("Invalid choice. Choose a number from %d to %d.\n\n", min, max);
    }
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
    bool animate = true;
    bool batch = false;
    bool csv = false;

    for (int i = 1; i < argc; i++) {
        const char *value = NULL;
        if ((value = arg_value(argv[i], "--dataset=")) != NULL) dataset = value;
        else if ((value = arg_value(argv[i], "--solver=")) != NULL) solver = value;
        else if ((value = arg_value(argv[i], "--level=")) != NULL) level_number = atoi(value);
        else if (strcmp(argv[i], "--no-animate") == 0) animate = false;
        else if (strcmp(argv[i], "--batch") == 0) batch = true;
        else if (strcmp(argv[i], "--csv") == 0) csv = true;
    }

    if (dataset && solver && batch && csv) {
        evaluate_csv(dataset, solver);
        return 0;
    }

    if (dataset && solver && batch) {
        evaluate(dataset, solver);
        return 0;
    }

    if (dataset && solver && level_number > 0) {
        evaluate_single(dataset, solver, level_number, animate);
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
    evaluate_single(dataset, solver, level_number, animate);

    return 0;
}
