#include "GRAPH_SEARCH.h"
#include "HashTable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    State state;
    int parent;
    Push action;
    int depth;
    int g;
    int f;
    int order;
} Node;

static SolverResult build_result(Node *nodes, int idx, int length) {
    SolverResult result = {true, NULL, length, 0, 0, 0, 0, 0, 0, 0.0};
    result.path = malloc((size_t)result.path_len * sizeof(Push));

    for (int i = result.path_len - 1; i >= 0; i--) {
        result.path[i] = nodes[idx].action;
        idx = nodes[idx].parent;
    }

    return result;
}

SolverResult First_InsertFrontier_Search_TREE(Level *level) {
    int cap = 1024;
    int front = 0;
    int back = 0;
    Node *nodes = malloc((size_t)cap * sizeof(Node));
    HashTable visited;
    hash_table_init(&visited, HASH_TABLE_BASED_SIZE);
    long long nodes_visited = 0;
    long long deadlocks_detected = 0;
    long long deadlock_corner_detected = 0;
    long long deadlock_freeze_detected = 0;
    long long deadlock_tunnel_detected = 0;
    long long deadlock_other_detected = 0;

    nodes[back++] = (Node){copy_state(&level->init_state), -1, {{0, 0}, ""}, 0, 0, 0, 0};
    hash_table_insert(&visited, state_key(&level->init_state));

    while (front < back) {
        int cur = front++;
        nodes_visited++;

        if (is_goal_state(&nodes[cur].state, level)) {
            SolverResult out = build_result(nodes, cur, nodes[cur].depth);
            out.nodes_visited = nodes_visited;
            out.deadlocks_detected = deadlocks_detected;
            out.deadlock_corner_detected = deadlock_corner_detected;
            out.deadlock_freeze_detected = deadlock_freeze_detected;
            out.deadlock_tunnel_detected = deadlock_tunnel_detected;
            out.deadlock_other_detected = deadlock_other_detected;
            for (int i = 0; i < back; i++) free_state(&nodes[i].state);
            free(nodes);
            hash_table_free(&visited);
            return out;
        }

        int push_count = 0;
        Push *pushes = get_valid_pushes(&nodes[cur].state, level, &push_count);

        for (int i = 0; i < push_count; i++) {
            State ns = step_state(&nodes[cur].state, pushes[i]);
            DeadlockType deadlock_type = DEADLOCK_NONE;

            for (int b = 0; b < ns.num_boxes && deadlock_type == DEADLOCK_NONE; b++) {
                deadlock_type = detect_deadlock_type(ns.boxes[b], level, &ns);
            }

            char *key = state_key(&ns);

            if (deadlock_type == DEADLOCK_NONE && !hash_table_contains(&visited, key)) {
                if (back == cap) {
                    cap *= HASH_TABLE_INCREASING_RATE;
                    nodes = realloc(nodes, (size_t)cap * sizeof(Node));
                }

                nodes[back] = (Node){ns, cur, pushes[i], nodes[cur].depth + 1, 0, 0, 0};
                back++;
                hash_table_insert(&visited, key);
            } else {
                if (deadlock_type != DEADLOCK_NONE) {
                    deadlocks_detected++;
                    if (deadlock_type == DEADLOCK_CORNER) deadlock_corner_detected++;
                    else if (deadlock_type == DEADLOCK_FREEZE) deadlock_freeze_detected++;
                    else if (deadlock_type == DEADLOCK_TUNNEL) deadlock_tunnel_detected++;
                    else deadlock_other_detected++;
                }
                free(key);
                free_state(&ns);
            }
        }

        free(pushes);
    }

    for (int i = 0; i < back; i++) free_state(&nodes[i].state);
    free(nodes);
    hash_table_free(&visited);

    SolverResult out = (SolverResult){false, NULL, 0, 0, 0, 0, 0, 0, 0, 0.0};
    out.nodes_visited = nodes_visited;
    out.deadlocks_detected = deadlocks_detected;
    out.deadlock_corner_detected = deadlock_corner_detected;
    out.deadlock_freeze_detected = deadlock_freeze_detected;
    out.deadlock_tunnel_detected = deadlock_tunnel_detected;
    out.deadlock_other_detected = deadlock_other_detected;
    return out;
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
        int parent = (i - 1) / 2;
        if (node_less(nodes, (*heap)[parent], (*heap)[i])) break;

        int tmp = (*heap)[parent];
        (*heap)[parent] = (*heap)[i];
        (*heap)[i] = tmp;
        i = parent;
    }
}

static int heap_pop(int *heap, int *size, Node *nodes) {
    int out = heap[0];
    heap[0] = heap[--(*size)];

    int i = 0;
    while (1) {
        int left = i * 2 + 1;
        int right = left + 1;
        int best = i;

        if (left < *size && node_less(nodes, heap[left], heap[best])) best = left;
        if (right < *size && node_less(nodes, heap[right], heap[best])) best = right;
        if (best == i) break;

        int tmp = heap[i];
        heap[i] = heap[best];
        heap[best] = tmp;
        i = best;
    }

    return out;
}

SolverResult Insert_Priority_Queue_GENERALIZED_A_Star(Level *level) {
    int node_cap = 1024;
    int count = 0;
    int order = 0;
    Node *nodes = malloc((size_t)node_cap * sizeof(Node));
    int *heap = NULL;
    int heap_size = 0;
    int heap_cap = 0;
    HashTable visited;
    hash_table_init(&visited, HASH_TABLE_BASED_SIZE);
    long long nodes_visited = 0;
    long long deadlocks_detected = 0;
    long long deadlock_corner_detected = 0;
    long long deadlock_freeze_detected = 0;
    long long deadlock_tunnel_detected = 0;
    long long deadlock_other_detected = 0;

    State start = copy_state(&level->init_state);
    nodes[count] = (Node){start, -1, {{0, 0}, ""}, 0, 0, hungarian_heuristic(&start, level), order++};
    heap_push(&heap, &heap_size, &heap_cap, nodes, count);
    hash_table_insert(&visited, state_key(&start));
    count++;

    while (heap_size > 0) {
        int cur = heap_pop(heap, &heap_size, nodes);
        nodes_visited++;

        if (is_goal_state(&nodes[cur].state, level)) {
            SolverResult out = build_result(nodes, cur, nodes[cur].g);
            out.nodes_visited = nodes_visited;
            out.deadlocks_detected = deadlocks_detected;
            out.deadlock_corner_detected = deadlock_corner_detected;
            out.deadlock_freeze_detected = deadlock_freeze_detected;
            out.deadlock_tunnel_detected = deadlock_tunnel_detected;
            out.deadlock_other_detected = deadlock_other_detected;
            for (int i = 0; i < count; i++) free_state(&nodes[i].state);
            free(nodes);
            free(heap);
            hash_table_free(&visited);
            return out;
        }

        int push_count = 0;
        Push *pushes = get_valid_pushes(&nodes[cur].state, level, &push_count);

        for (int i = 0; i < push_count; i++) {
            State ns = step_state(&nodes[cur].state, pushes[i]);
            DeadlockType deadlock_type = DEADLOCK_NONE;

            for (int b = 0; b < ns.num_boxes && deadlock_type == DEADLOCK_NONE; b++) {
                deadlock_type = detect_deadlock_type(ns.boxes[b], level, &ns);
            }

            if (deadlock_type != DEADLOCK_NONE) {
                deadlocks_detected++;
                if (deadlock_type == DEADLOCK_CORNER) deadlock_corner_detected++;
                else if (deadlock_type == DEADLOCK_FREEZE) deadlock_freeze_detected++;
                else if (deadlock_type == DEADLOCK_TUNNEL) deadlock_tunnel_detected++;
                else deadlock_other_detected++;
                free_state(&ns);
                continue;
            }

            char *key = state_key(&ns);
            if (hash_table_contains(&visited, key)) {
                free(key);
                free_state(&ns);
                continue;
            }

            if (count == node_cap) {
                node_cap *= HASH_TABLE_INCREASING_RATE;
                nodes = realloc(nodes, (size_t)node_cap * sizeof(Node));
            }

            int g = nodes[cur].g + 1;
            nodes[count] = (Node){ns, cur, pushes[i], g, g, g + hungarian_heuristic(&ns, level), order++};
            hash_table_insert(&visited, key);
            heap_push(&heap, &heap_size, &heap_cap, nodes, count);
            count++;
        }

        free(pushes);
    }

    for (int i = 0; i < count; i++) free_state(&nodes[i].state);
    free(nodes);
    free(heap);
    hash_table_free(&visited);

    SolverResult out = (SolverResult){false, NULL, 0, 0, 0, 0, 0, 0, 0, 0.0};
    out.nodes_visited = nodes_visited;
    out.deadlocks_detected = deadlocks_detected;
    out.deadlock_corner_detected = deadlock_corner_detected;
    out.deadlock_freeze_detected = deadlock_freeze_detected;
    out.deadlock_tunnel_detected = deadlock_tunnel_detected;
    out.deadlock_other_detected = deadlock_other_detected;
    return out;
}

SolverResult bfs_solver(Level *level) {
    return First_InsertFrontier_Search_TREE(level);
}

SolverResult a_star_solver(Level *level) {
    return Insert_Priority_Queue_GENERALIZED_A_Star(level);
}
