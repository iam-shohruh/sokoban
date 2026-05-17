#include "bfs.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Simple FIFO queue of States                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    State *data;
    int    head;
    int    tail;
    int    capacity;
} StateQueue;

static StateQueue queue_create(int cap) {
    StateQueue q;
    q.capacity = cap;
    q.head     = 0;
    q.tail     = 0;
    q.data     = (State *)malloc(cap * sizeof(State));
    return q;
}

static void queue_free(StateQueue *q) {
    /* States still in queue */
    while (q->head != q->tail) {
        state_free(&q->data[q->head]);
        q->head = (q->head + 1) % q->capacity;
    }
    free(q->data);
    q->data = NULL;
}

static bool queue_empty(const StateQueue *q) { return q->head == q->tail; }

static void queue_push(StateQueue *q, const State *s) {
    int next_tail = (q->tail + 1) % q->capacity;
    if (next_tail == q->head) {
        /* Grow */
        int new_cap = q->capacity * 2;
        State *new_data = (State *)malloc(new_cap * sizeof(State));
        int count = 0;
        while (q->head != q->tail) {
            new_data[count++] = q->data[q->head];
            q->head = (q->head + 1) % q->capacity;
        }
        free(q->data);
        q->data     = new_data;
        q->head     = 0;
        q->tail     = count;
        q->capacity = new_cap;
    }
    q->data[q->tail] = state_copy(s);
    q->tail          = (q->tail + 1) % q->capacity;
}

static State queue_pop(StateQueue *q) {
    State s   = q->data[q->head];
    q->head   = (q->head + 1) % q->capacity;
    return s;
}

/* ------------------------------------------------------------------ */
/* BFS solver                                                           */
/* ------------------------------------------------------------------ */

SolverResult bfs_solver(const Level *level) {
    /*
     * Python:
     *   env = SokobanEnv(level)
     *   start_state = level.init_state
     *   queue = deque([start_state])
     *   visited = set([start_state])
     *   parent = {start_state: None}
     *   while queue:
     *       state = queue.popleft()
     *       if env.is_goal_state(state):
     *           reconstruct path …
     *           return {"solved": True, "path": actions[::-1]}
     *       for action in env.get_valid_pushes(state):
     *           new_state = env.step(state, action)
     *           if new_state not in visited:
     *               if not env.get_deadlock_state(new_state):
     *                   visited.add(new_state)
     *                   queue.append(new_state)
     *                   parent[new_state] = (state, action)
     *   return {"solved": False, "path": []}
     *
     * Note: Python uses env.get_deadlock_state() which is env.is_deadlock() in env.py.
     */

    SokobanEnv env;
    sokoban_env_init(&env, level);

    State start_state = state_copy(&level->init_state);

    StateQueue     queue   = queue_create(256);
    StateSet       visited = state_set_create(1024);
    StateParentMap parent  = state_parent_map_create(1024);

    queue_push(&queue, &start_state);
    state_set_add(&visited, &start_state);

    ParentEntry no_parent = { false, {0}, {{0,0}, ""} };
    state_parent_map_set(&parent, &start_state, &no_parent);

    SolverResult result;
    result.solved   = false;
    result.path     = NULL;
    result.path_len = 0;

    while (!queue_empty(&queue)) {
        State state = queue_pop(&queue);

        if (sokoban_env_is_goal_state(&env, &state)) {
            /* Reconstruct actions (Python: actions[::-1]) */
            int   actions_cap  = 64;
            Push *actions      = (Push *)malloc(actions_cap * sizeof(Push));
            int   actions_size = 0;

            State current = state_copy(&state);
            for (;;) {
                ParentEntry entry;
                if (!state_parent_map_get(&parent, &current, &entry)) break;
                if (!entry.has_parent) break;

                if (actions_size == actions_cap) {
                    actions_cap *= 2;
                    actions = (Push *)realloc(actions, actions_cap * sizeof(Push));
                }
                actions[actions_size++] = entry.push;
                State next = state_copy(&entry.parent_state);
                state_free(&current);
                current = next;
            }
            state_free(&current);

            /* Reverse (mirrors Python actions[::-1]) */
            for (int i = 0, j = actions_size-1; i < j; i++, j--) {
                Push tmp = actions[i]; actions[i] = actions[j]; actions[j] = tmp;
            }

            printf("Solved %d in %d moves\n", level->game_id, actions_size);

            result.solved   = true;
            result.path     = actions;
            result.path_len = actions_size;

            state_free(&state);
            goto cleanup;
        }

        int   num_actions = 0;
        Push *valid_actions = sokoban_env_get_valid_pushes(&env, &state, &num_actions);

        for (int i = 0; i < num_actions; i++) {
            State new_state = sokoban_env_step(&env, &state, &valid_actions[i]);

            if (!state_set_contains(&visited, &new_state)) {
                if (!sokoban_env_is_deadlock(&env, &new_state)) {
                    state_set_add(&visited, &new_state);
                    queue_push(&queue, &new_state);

                    ParentEntry entry;
                    entry.has_parent   = true;
                    entry.parent_state = state;
                    entry.push         = valid_actions[i];
                    state_parent_map_set(&parent, &new_state, &entry);
                }
            }

            state_free(&new_state);
        }

        free(valid_actions);
        state_free(&state);
    }

    printf("Failed to solve %d\n", level->game_id);

cleanup:
    queue_free(&queue);
    state_set_free(&visited);
    state_parent_map_free(&parent);
    state_free(&start_state);
    sokoban_env_free(&env);

    return result;
}