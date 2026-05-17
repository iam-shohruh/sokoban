#include "a_star.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>

/* ------------------------------------------------------------------ */
/* Heuristics                                                           */
/* ------------------------------------------------------------------ */

int manhattan_heuristic(const State *state, const Level *level) {
    /*
     * Python:
     *   total_distance = 0
     *   taken = set()
     *   for box in state.boxes:
     *       chosen_goal = None; min_distance = inf
     *       for goal in level.goals:
     *           if goal not in taken:
     *               distance = |box.row-goal.row| + |box.col-goal.col|
     *               if distance < min_distance:
     *                   min_distance = distance; chosen_goal = goal
     *       total_distance += min_distance
     *       taken.add(chosen_goal)
     *   return total_distance
     */
    int total_distance = 0;

    /* taken[] – tracks which goal indices have been chosen */
    bool *taken = (bool *)calloc(level->num_goals, sizeof(bool));

    for (int b = 0; b < state->num_boxes; b++) {
        int chosen_goal_idx = -1;
        int min_distance    = INT_MAX;

        for (int g = 0; g < level->num_goals; g++) {
            if (!taken[g]) {
                int distance = abs(state->boxes[b].row - level->goals[g].row)
                             + abs(state->boxes[b].col - level->goals[g].col);
                if (distance < min_distance) {
                    min_distance    = distance;
                    chosen_goal_idx = g;
                }
            }
        }

        if (chosen_goal_idx >= 0) {
            total_distance += min_distance;
            taken[chosen_goal_idx] = true;
        }
    }

    free(taken);
    return total_distance;
}

/* Placeholder – mirrors Python pass */
int hungarian_heuristic(const State *state, const Level *level) {
    (void)state; (void)level;
    return 0;
}

/* Placeholder – mirrors Python pass */
int pattern_database_heuristic(const State *state, const Level *level) {
    (void)state; (void)level;
    return 0;
}

/* Placeholder – mirrors Python pass */
int hybrid_heuristic(const State *state, const Level *level) {
    (void)state; (void)level;
    return 0;
}

/* ------------------------------------------------------------------ */
/* SolverResult                                                         */
/* ------------------------------------------------------------------ */

void solver_result_free(SolverResult *r) {
    free(r->path);
    r->path     = NULL;
    r->path_len = 0;
}

/* ------------------------------------------------------------------ */
/* Min-heap for A* priority queue                                       */
/* Heap element: (f_cost, counter, State)                              */
/* ------------------------------------------------------------------ */

typedef struct {
    int   f_cost;
    int   counter;  /* tie-breaker, mirrors Python heapq tuple ordering */
    State state;
} HeapNode;

typedef struct {
    HeapNode *data;
    int       size;
    int       capacity;
} MinHeap;

static MinHeap heap_create(int cap) {
    MinHeap h;
    h.capacity = cap;
    h.size     = 0;
    h.data     = (HeapNode *)malloc(cap * sizeof(HeapNode));
    return h;
}

static void heap_free(MinHeap *h) {
    for (int i = 0; i < h->size; i++) state_free(&h->data[i].state);
    free(h->data);
    h->data = NULL; h->size = h->capacity = 0;
}

static void heap_swap(MinHeap *h, int i, int j) {
    HeapNode tmp  = h->data[i];
    h->data[i]    = h->data[j];
    h->data[j]    = tmp;
}

static int heap_cmp(const HeapNode *a, const HeapNode *b) {
    if (a->f_cost != b->f_cost) return a->f_cost - b->f_cost;
    return a->counter - b->counter;
}

static void heap_push(MinHeap *h, int f_cost, int counter, const State *s) {
    if (h->size == h->capacity) {
        h->capacity *= 2;
        h->data = (HeapNode *)realloc(h->data, h->capacity * sizeof(HeapNode));
    }
    h->data[h->size].f_cost  = f_cost;
    h->data[h->size].counter = counter;
    h->data[h->size].state   = state_copy(s);
    int i = h->size++;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap_cmp(&h->data[i], &h->data[parent]) < 0) {
            heap_swap(h, i, parent);
            i = parent;
        } else break;
    }
}

static HeapNode heap_pop(MinHeap *h) {
    HeapNode top = h->data[0];
    h->data[0]   = h->data[--h->size];
    int i = 0;
    while (1) {
        int l = 2*i+1, r = 2*i+2, smallest = i;
        if (l < h->size && heap_cmp(&h->data[l], &h->data[smallest]) < 0) smallest = l;
        if (r < h->size && heap_cmp(&h->data[r], &h->data[smallest]) < 0) smallest = r;
        if (smallest == i) break;
        heap_swap(h, i, smallest);
        i = smallest;
    }
    return top;
}

/* ------------------------------------------------------------------ */
/* A* solver                                                            */
/* ------------------------------------------------------------------ */

SolverResult a_star_solver(const Level *level, bool debug) {
    /*
     * Python:
     *   env = SokobanEnv(level)
     *   start_state = level.init_state
     *   counter = 0
     *   queue = [(0, counter, start_state)]
     *   visited = set([start_state])
     *   parent = {start_state: None}
     *   g_costs = {start_state: 0}
     *   h_costs = {start_state: manhattan_heuristic(start_state, level)}
     *   while queue:
     *       _, _, state = heapq.heappop(queue)
     *       if env.is_goal_state(state): ... reconstruct path ...
     *       for push in env.get_valid_pushes(state):
     *           new_state = env.step(state, push)
     *           if env.is_deadlock(new_state): continue
     *           g_cost = g_costs[state] + 1
     *           h_cost = manhattan_heuristic(new_state, level)
     *           f_cost = g_cost + h_cost
     *           if new_state not in visited or g_cost < g_costs.get(new_state, inf):
     *               visited.add(new_state)
     *               parent[new_state] = (state, push)
     *               g_costs[new_state] = g_cost
     *               h_costs[new_state] = h_cost
     *               counter += 1
     *               heapq.heappush(queue, (f_cost, counter, new_state))
     *   return {"solved": False, "path": []}
     */
    (void)debug; /* debug/steps_out not ported */

    SokobanEnv env;
    sokoban_env_init(&env, level);

    State start_state = state_copy(&level->init_state);

    int counter = 0;

    MinHeap queue = heap_create(256);

    StateSet     visited = state_set_create(1024);
    state_set_add(&visited, &start_state);

    StateParentMap parent  = state_parent_map_create(1024);
    ParentEntry    no_parent = { false, {0}, {{0,0}, ""} };
    state_parent_map_set(&parent, &start_state, &no_parent);

    StateIntMap g_costs = state_int_map_create(1024);
    state_int_map_set(&g_costs, &start_state, 0);

    StateIntMap h_costs = state_int_map_create(1024);
    int h0 = manhattan_heuristic(&start_state, level);
    state_int_map_set(&h_costs, &start_state, h0);

    heap_push(&queue, h0, counter, &start_state);

    SolverResult result;
    result.solved   = false;
    result.path     = NULL;
    result.path_len = 0;

    while (queue.size > 0) {
        HeapNode node = heap_pop(&queue);
        State    state = node.state; /* takes ownership */

        if (sokoban_env_is_goal_state(&env, &state)) {
            /* Reconstruct path */
            int   path_cap  = 64;
            Push *pushes    = (Push *)malloc(path_cap * sizeof(Push));
            int   path_size = 0;

            State current = state_copy(&state);
            for (;;) {
                ParentEntry entry;
                if (!state_parent_map_get(&parent, &current, &entry)) break;
                if (!entry.has_parent) break;

                if (path_size == path_cap) {
                    path_cap *= 2;
                    pushes = (Push *)realloc(pushes, path_cap * sizeof(Push));
                }
                pushes[path_size++] = entry.push;
                State next = state_copy(&entry.parent_state);
                state_free(&current);
                current = next;
            }
            state_free(&current);

            /* Reverse */
            for (int i = 0, j = path_size-1; i < j; i++, j--) {
                Push tmp = pushes[i]; pushes[i] = pushes[j]; pushes[j] = tmp;
            }

            printf("Solved level %d in %d pushes\n", level->game_id, path_size);

            result.solved   = true;
            result.path     = pushes;
            result.path_len = path_size;

            state_free(&state);
            goto cleanup;
        }

        int num_pushes = 0;
        Push *valid_pushes = sokoban_env_get_valid_pushes(&env, &state, &num_pushes);

        int g_state;
        state_int_map_get(&g_costs, &state, &g_state);

        for (int p = 0; p < num_pushes; p++) {
            State new_state = sokoban_env_step(&env, &state, &valid_pushes[p]);

            if (sokoban_env_is_deadlock(&env, &new_state)) {
                state_free(&new_state);
                continue;
            }

            int g_cost = g_state + 1;
            int h_cost = manhattan_heuristic(&new_state, level);
            int f_cost = g_cost + h_cost;

            int existing_g = state_int_map_get_or_inf(&g_costs, &new_state);
            bool in_visited = state_set_contains(&visited, &new_state);

            if (!in_visited || g_cost < existing_g) {
                state_set_add(&visited, &new_state);

                ParentEntry entry;
                entry.has_parent    = true;
                entry.parent_state  = state;
                entry.push          = valid_pushes[p];
                state_parent_map_set(&parent, &new_state, &entry);

                state_int_map_set(&g_costs, &new_state, g_cost);
                state_int_map_set(&h_costs, &new_state, h_cost);

                counter++;
                heap_push(&queue, f_cost, counter, &new_state);
            }

            state_free(&new_state);
        }

        free(valid_pushes);
        state_free(&state);
    }

cleanup:
    heap_free(&queue);
    state_set_free(&visited);
    state_parent_map_free(&parent);
    state_int_map_free(&g_costs);
    state_int_map_free(&h_costs);
    state_free(&start_state);
    sokoban_env_free(&env);

    return result;
}