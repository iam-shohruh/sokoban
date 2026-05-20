#include "../env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node { State state; int parent; Push action; int depth; } Node;

typedef struct { char **items; int count, cap; } KeySet;
static bool seen(KeySet *s, char *key) { for (int i = 0; i < s->count; i++) if (strcmp(s->items[i], key) == 0) return true; return false; }
static void add_key(KeySet *s, char *key) { if (s->count == s->cap) { s->cap = s->cap ? s->cap * 2 : 256; s->items = realloc(s->items, (size_t)s->cap * sizeof(char*)); } s->items[s->count++] = key; }
static void free_keys(KeySet *s) { for (int i = 0; i < s->count; i++) free(s->items[i]); free(s->items); }

static SolverResult make_result(Node *nodes, int idx) {
    SolverResult r = {true, NULL, nodes[idx].depth};
    r.path = malloc((size_t)r.path_len * sizeof(Push));
    for (int i = r.path_len - 1; i >= 0; i--) { r.path[i] = nodes[idx].action; idx = nodes[idx].parent; }
    return r;
}

SolverResult bfs_solver(Level *level) {
    int cap = 1024, front = 0, back = 0;
    Node *nodes = malloc((size_t)cap * sizeof(Node));
    KeySet visited = {0};
    nodes[back++] = (Node){copy_state(&level->init_state), -1, {{0,0}, ""}, 0};
    add_key(&visited, state_key(&level->init_state));

    while (front < back) {
        int cur = front++;
        if (is_goal_state(&nodes[cur].state, level)) {
            SolverResult out = make_result(nodes, cur);
            printf("Solved %d in %d moves\n", level->game_id, out.path_len);
            for (int i = 0; i < back; i++) free_state(&nodes[i].state);
            free(nodes); free_keys(&visited); return out;
        }
        int push_count = 0;
        Push *pushes = get_valid_pushes(&nodes[cur].state, level, &push_count);
        for (int i = 0; i < push_count; i++) {
            State ns = step_state(&nodes[cur].state, pushes[i]);
            char *key = state_key(&ns);
            bool dead = false;
            for (int b = 0; b < ns.num_boxes && !dead; b++) dead = is_deadlock(ns.boxes[b], level, &ns);
            if (!dead && !seen(&visited, key)) {
                if (back == cap) { cap *= 2; nodes = realloc(nodes, (size_t)cap * sizeof(Node)); }
                nodes[back++] = (Node){ns, cur, pushes[i], nodes[cur].depth + 1};
                add_key(&visited, key);
            } else { free(key); free_state(&ns); }
        }
        free(pushes);
    }
    printf("Failed to solve %d\n", level->game_id);
    for (int i = 0; i < back; i++) free_state(&nodes[i].state);
    free(nodes); free_keys(&visited);
    return (SolverResult){false, NULL, 0};
}
