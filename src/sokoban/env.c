#include "env.h"
#include <stdlib.h>
#include <string.h>

static bool is_wall(const Level* level, int r, int c) {
    if (r < 0 || r >= level->height || c < 0 || c >= level->width) return true;
    return level->walls[r * level->width + c];
}

static bool is_goal(const Level* level, int r, int c) {
    if (r < 0 || r >= level->height || c < 0 || c >= level->width) return false;
    return level->goals[r * level->width + c];
}

static bool is_box_at(int r, int c, const State* state) {
    for (int i = 0; i < state->num_boxes; i++) {
        if (state->boxes[i].row == r && state->boxes[i].col == c) return true;
    }
    return false;
}

State reset(const Level* level) {
    State init_copy;
    init_copy.player = level->init_state.player;
    init_copy.num_boxes = level->init_state.num_boxes;
    
    init_copy.boxes = malloc(init_copy.num_boxes * sizeof(Position));
    memcpy(init_copy.boxes, level->init_state.boxes, init_copy.num_boxes * sizeof(Position));
    
    return init_copy;
}

State step(const State* state, Push push) {
    State next_state;
    next_state.player = push.position;
    next_state.num_boxes = state->num_boxes;
    
    next_state.boxes = malloc(state->num_boxes * sizeof(Position));
    memcpy(next_state.boxes, state->boxes, state->num_boxes * sizeof(Position));

    int dr = 0, dc = 0;
    if (strcmp(push.direction, "UP") == 0) dr = -1;
    else if (strcmp(push.direction, "DOWN") == 0) dr = 1;
    else if (strcmp(push.direction, "LEFT") == 0) dc = -1;
    else if (strcmp(push.direction, "RIGHT") == 0) dc = 1;

    for (int i = 0; i < next_state.num_boxes; i++) {
        if (next_state.boxes[i].row == push.position.row && next_state.boxes[i].col == push.position.col) {
            next_state.boxes[i].row += dr;
            next_state.boxes[i].col += dc;
            break;
        }
    }
    return next_state;
}

bool is_goal_state(const State* state, const Level* level) {
    for (int i = 0; i < state->num_boxes; i++) {
        if (!is_goal(level, state->boxes[i].row, state->boxes[i].col)) {
            return false;
        }
    }
    return true;
}

bool is_deadlock(const State* state, const Level* level) {
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    for (int b = 0; b < state->num_boxes; b++) {
        int r = state->boxes[b].row;
        int c = state->boxes[b].col;
        
        if (is_goal(level, r, c)) continue;

        bool vert_blocked = false, horz_blocked = false;
        for (int i = 0; i < 4; i++) {
            if (is_wall(level, r + dr[i], c + dc[i]) || is_box_at(r + dr[i], c + dc[i], state)) {
                if (i < 2) vert_blocked = true;
                else horz_blocked = true;
            }
        }
        if (horz_blocked && vert_blocked) return true;

        int offsets[4][4][2] = {
            {{0,0}, {0,1}, {1,0}, {1,1}},   {{0,0}, {0,-1}, {1,0}, {1,-1}},
            {{0,0}, {0,1}, {-1,0}, {-1,1}}, {{0,0}, {0,-1}, {-1,0}, {-1,-1}}
        };
        for (int i = 0; i < 4; i++) {
            bool quad_blocked = true, goal_in_quad = false;
            for (int j = 0; j < 4; j++) {
                int qr = r + offsets[i][j][0];
                int qc = c + offsets[i][j][1];
                
                bool is_w = is_wall(level, qr, qc);
                bool is_b = is_box_at(qr, qc, state);
                
                if (!is_w && !is_b) { quad_blocked = false; break; }
                if (is_b && is_goal(level, qr, qc)) goal_in_quad = true;
            }
            if (quad_blocked && !goal_in_quad) return true;
        }

        int wall_checks[4][6] = {
            {-1, 0,   0, -1,   0, 1},
            { 1, 0,   0, -1,   0, 1},
            { 0,-1,  -1,  0,   1, 0},
            { 0, 1,  -1,  0,   1, 0}
        };

        for (int i = 0; i < 4; i++) {
            int w_dr = wall_checks[i][0];
            int w_dc = wall_checks[i][1];

            if (is_wall(level, r + w_dr, c + w_dc)) {
                bool is_trapped = true;

                for (int scan = 0; scan < 2; scan++) {
                    int s_dr = wall_checks[i][2 + (scan * 2)];
                    int s_dc = wall_checks[i][3 + (scan * 2)];
                    
                    int curr_r = r;
                    int curr_c = c;
                    bool safe_in_this_dir = false;

                    while (true) {
                        curr_r += s_dr;
                        curr_c += s_dc;

                        if (is_wall(level, curr_r, curr_c)) {
                            break; 
                        }

                        if (is_goal(level, curr_r, curr_c)) {
                            safe_in_this_dir = true;
                            break;
                        }

                        if (!is_wall(level, curr_r + w_dr, curr_c + w_dc)) {
                            safe_in_this_dir = true;
                            break;
                        }
                    }

                    if (safe_in_this_dir) {
                        is_trapped = false;
                        break; 
                    }
                }

                if (is_trapped) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

Push* get_valid_pushes(const State* state, const Level* level, int* out_count) {
    int grid_size = level->width * level->height;
    
    bool* visited = calloc(grid_size, sizeof(bool));
    Position* queue = malloc(grid_size * sizeof(Position));
    Push* valid_pushes = malloc(state->num_boxes * 4 * sizeof(Push));
    
    *out_count = 0;
    int head = 0, tail = 0;
    
    queue[tail++] = state->player;
    visited[state->player.row * level->width + state->player.col] = true;

    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    const char* dirs[] = {"UP", "DOWN", "LEFT", "RIGHT"};

    while (head < tail) {
        Position curr = queue[head++];
        
        for (int i = 0; i < 4; i++) {
            int nr = curr.row + dr[i];
            int nc = curr.col + dc[i];
            
            if (nr < 0 || nr >= level->height || nc < 0 || nc >= level->width) continue;
            if (is_wall(level, nr, nc) || visited[nr * level->width + nc]) continue;

            if (is_box_at(nr, nc, state)) {
                int b_nr = nr + dr[i];
                int b_nc = nc + dc[i];
                
                if (!is_wall(level, b_nr, b_nc) && !is_box_at(b_nr, b_nc, state)) {
                    valid_pushes[*out_count].position.row = nr;
                    valid_pushes[*out_count].position.col = nc;
                    valid_pushes[*out_count].direction = dirs[i];
                    (*out_count)++;
                }
            } else {
                visited[nr * level->width + nc] = true;
                queue[tail++] = (Position){nr, nc};
            }
        }
    }
    
    free(visited);
    free(queue);
    
    return valid_pushes;
}