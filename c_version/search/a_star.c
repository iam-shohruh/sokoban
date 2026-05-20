#include "../env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE 262144

typedef struct Node {
    State state;
    int parent;
    Push action;
    int g;
    int f;
    int order;
} Node;

typedef struct Entry {
    char *key;
    struct Entry *next;
} Entry;

typedef struct {
    Entry **buckets;
} KeySet;

static unsigned long hash_key(const char *s) {
    unsigned long h = 5381;
    while (*s) h = ((h << 5) + h) + (unsigned char)(*s++);
    return h;
}

static void set_init(KeySet *set) {
    set->buckets = calloc(HASH_SIZE, sizeof(Entry *));
}

static bool set_has(KeySet *set, const char *key) {
    unsigned long h = hash_key(key) % HASH_SIZE;
    for (Entry *e = set->buckets[h]; e; e = e->next)
        if (strcmp(e->key, key) == 0) return true;
    return false;
}

static void set_add(KeySet *set, char *key) {
    unsigned long h = hash_key(key) % HASH_SIZE;
    Entry *e = malloc(sizeof(Entry));
    e->key = key;
    e->next = set->buckets[h];
    set->buckets[h] = e;
}

static void set_free(KeySet *set) {
    for (int i = 0; i < HASH_SIZE; i++) {
        Entry *e = set->buckets[i];
        while (e) {
            Entry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(set->buckets);
}

static bool node_less(Node *nodes, int a, int b) {
    if (nodes[a].f != nodes[b].f) return nodes[a].f < nodes[b].f;
    return nodes[a].order < nodes[b].order;
}

static void heap_push(int **heap, int *size, int *cap, Node *nodes, int idx) {
    if (*size == *cap) {
        *cap = *cap ? *cap * 2 : 1024;
        *heap = realloc(*heap, (size_t)(*cap) * sizeof(int));
    }
    int i = (*size)++;
    (*heap)[i] = idx;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (node_less(nodes, (*heap)[p], (*heap)[i])) break;
        int t = (*heap)[p]; (*heap)[p] = (*heap)[i]; (*heap)[i] = t;
        i = p;
    }
}

static int heap_pop(int *heap, int *size, Node *nodes) {
    int out = heap[0];
    heap[0] = heap[--(*size)];
    int i = 0;
    for (;;) {
        int l = i * 2 + 1, r = l + 1, best = i;
        if (l < *size && node_less(nodes, heap[l], heap[best])) best = l;
        if (r < *size && node_less(nodes, heap[r], heap[best])) best = r;
        if (best == i) break;
        int t = heap[i]; heap[i] = heap[best]; heap[best] = t;
        i = best;
    }
    return out;
}

static SolverResult build_result(Node *nodes, int idx) {
    SolverResult r = {true, NULL, nodes[idx].g};
    r.path = malloc((size_t)r.path_len * sizeof(Push));
    for (int i = r.path_len - 1; i >= 0; i--) {
        r.path[i] = nodes[idx].action;
        idx = nodes[idx].parent;
    }
    return r;
}

SolverResult a_star_solver(Level *level) {
    int node_cap = 1024, count = 0, order = 0;
    Node *nodes = malloc((size_t)node_cap * sizeof(Node));
    int *heap = NULL, heap_size = 0, heap_cap = 0;
    KeySet visited;
    set_init(&visited);

    State start = copy_state(&level->init_state);
    nodes[count] = (Node){start, -1, {{0, 0}, ""}, 0, hungarian_heuristic(&start, level), order++};
    heap_push(&heap, &heap_size, &heap_cap, nodes, count);
    char *start_key = state_key(&start);
    set_add(&visited, start_key);
    count++;

    while (heap_size > 0) {
        int cur = heap_pop(heap, &heap_size, nodes);

        if (is_goal_state(&nodes[cur].state, level)) {
            SolverResult out = build_result(nodes, cur);
            for (int i = 0; i < count; i++) free_state(&nodes[i].state);
            free(nodes);
            free(heap);
            set_free(&visited);
            return out;
        }

        int push_count = 0;
        Push *pushes = get_valid_pushes(&nodes[cur].state, level, &push_count);
        for (int i = 0; i < push_count; i++) {
            State ns = step_state(&nodes[cur].state, pushes[i]);
            bool dead = false;
            for (int b = 0; b < ns.num_boxes && !dead; b++) dead = is_deadlock(ns.boxes[b], level, &ns);
            if (dead) {
                free_state(&ns);
                continue;
            }

            char *key = state_key(&ns);
            if (set_has(&visited, key)) {
                free(key);
                free_state(&ns);
                continue;
            }

            if (count == node_cap) {
                node_cap *= 2;
                nodes = realloc(nodes, (size_t)node_cap * sizeof(Node));
            }
            int g = nodes[cur].g + 1;
            nodes[count] = (Node){ns, cur, pushes[i], g, g + hungarian_heuristic(&ns, level), order++};
            set_add(&visited, key);
            heap_push(&heap, &heap_size, &heap_cap, nodes, count);
            count++;
        }
        free(pushes);
    }

    for (int i = 0; i < count; i++) free_state(&nodes[i].state);
    free(nodes);
    free(heap);
    set_free(&visited);
    return (SolverResult){false, NULL, 0};
}
