/*
 * SpecificToProblem.c — Sokoban Puzzle (Topic 8)
 *
 * All six compulsory functions are implemented here.
 * New helpers (bfs_reachable, hungarian_min_cost, sort_boxes) are added at the
 * bottom as permitted by the project specification.
 *
 * MAP ENCODING
 *   '#'  wall
 *   '.'  goal cell (empty)
 *   ' '  plain floor
 *   The board stored in G_board never contains '@' or '$'; those are overlaid
 *   only when printing.
 *
 * ACTION ENCODING
 *   action integer = cell_index * 4 + direction
 *   cell_index = row * G_COLS + col    (the box that will be pushed)
 *   direction: 0=UP 1=DOWN 2=LEFT 3=RIGHT
 */

#include "GRAPH_SEARCH.h"
#include "data_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

/* ===== GLOBAL MAP STATE ================================================ */

static int  G_ROWS      = 0;
static int  G_COLS      = 0;
static char G_board[MAX_BOARD_DIM][MAX_BOARD_DIM]; /* '#' '.' ' '          */
static int  G_num_goals = 0;
static int  G_goal_rows[MAX_BOXES];
static int  G_goal_cols[MAX_BOXES];

/* Direction tables (UP DOWN LEFT RIGHT) */
static const int DR[4] = {-1, 1,  0, 0};
static const int DC[4] = { 0, 0, -1, 1};
static const char *DIR_NAMES[4] = {"UP", "DOWN", "LEFT", "RIGHT"};

/* ===== TWO PUZZLE MAPS ================================================= */
/*
 * MAP 1  (5 rows × 7 cols, single box)
 *   #######
 *   #  .  #    goal at (1,3)
 *   # $   #    box  at (2,2)
 *   # @   #    player at (3,2)
 *   #######
 *
 * MAP 2  (6 rows × 8 cols, two boxes)
 *   ########
 *   # ..   #    goals at (1,2) and (1,3)
 *   # $$   #    boxes at (2,2) and (2,3)
 *   #  @   #    player at (3,3)
 *   #      #
 *   ########
 */
static const char *MAP1[] = {
    "#######",
    "#  .  #",
    "# $   #",
    "# @   #",
    "#######",
    NULL
};

static const char *MAP2[] = {
    "########",
    "# ..   #",
    "# $$   #",
    "#  @   #",
    "#      #",
    "########",
    NULL
};

/* ===== HELPER: sort box arrays by (row, col) for canonical key ========= */
static void sort_boxes(State *s)
{
    int i, j, tr, tc;
    for (i = 0; i < s->num_boxes - 1; i++)
        for (j = i + 1; j < s->num_boxes; j++)
            if (s->box_rows[j] < s->box_rows[i] ||
                (s->box_rows[j] == s->box_rows[i] && s->box_cols[j] < s->box_cols[i])) {
                tr = s->box_rows[i]; s->box_rows[i] = s->box_rows[j]; s->box_rows[j] = tr;
                tc = s->box_cols[i]; s->box_cols[i] = s->box_cols[j]; s->box_cols[j] = tc;
            }
}

/* ===== HELPER: BFS to check if player can reach (tr,tc) ================ */
/* The box at (skip_r, skip_c) is treated as passable (it's the one being pushed). */
static int bfs_reachable(const State *const s, int tr, int tc,
                          int skip_r, int skip_c)
{
    int visited[MAX_BOARD_DIM][MAX_BOARD_DIM];
    int qr[MAX_BOARD_DIM * MAX_BOARD_DIM];
    int qc[MAX_BOARD_DIM * MAX_BOARD_DIM];
    int front = 0, back = 0, d, b;
    int r, c, nr, nc;

    memset(visited, 0, sizeof(visited));

    qr[back] = s->player_row;
    qc[back] = s->player_col;
    back++;
    visited[s->player_row][s->player_col] = 1;

    while (front < back) {
        r = qr[front]; c = qc[front]; front++;
        if (r == tr && c == tc) return TRUE;
        for (d = 0; d < 4; d++) {
            nr = r + DR[d]; nc = c + DC[d];
            if (nr < 0 || nr >= G_ROWS || nc < 0 || nc >= G_COLS) continue;
            if (G_board[nr][nc] == '#') continue;
            if (visited[nr][nc]) continue;
            /* treat the target box as passable so the player can step there */
            if (!(nr == skip_r && nc == skip_c)) {
                /* check for other boxes */
                int blocked = 0;
                for (b = 0; b < s->num_boxes; b++)
                    if (s->box_rows[b] == nr && s->box_cols[b] == nc) { blocked = 1; break; }
                if (blocked) continue;
            }
            visited[nr][nc] = 1;
            qr[back] = nr; qc[back] = nc; back++;
        }
    }
    return FALSE;
}

/* ===== HELPER: Hungarian algorithm (minimum-cost assignment) =========== */
/* Assigns num_boxes boxes to num_goals goals minimising total Manhattan distance. */
static int hungarian_min_cost(const int *cost, int rows, int cols)
{
    int i, j, j0, j1, i0, delta, best;
    int *u, *v, *p, *way, *minv;
    bool *used;
    int result;

    if (rows <= 0 || cols <= 0) return 0;

    if (rows > cols) {
        /* transpose so rows <= cols */
        int *T = (int *)malloc((size_t)rows * (size_t)cols * sizeof(int));
        for (i = 0; i < rows; i++)
            for (j = 0; j < cols; j++)
                T[j * rows + i] = cost[i * cols + j];
        result = hungarian_min_cost(T, cols, rows);
        free(T);
        return result;
    }

    u    = (int *)calloc((size_t)(rows + 1), sizeof(int));
    v    = (int *)calloc((size_t)(cols + 1), sizeof(int));
    p    = (int *)calloc((size_t)(cols + 1), sizeof(int));
    way  = (int *)calloc((size_t)(cols + 1), sizeof(int));
    minv = (int *)malloc((size_t)(cols + 1) * sizeof(int));
    used = (bool *)malloc((size_t)(cols + 1) * sizeof(bool));

    for (i = 1; i <= rows; i++) {
        p[0] = i;
        for (j = 0; j <= cols; j++) { minv[j] = INT_MAX / 4; used[j] = false; }
        j0 = 0;
        while (1) {
            used[j0] = true;
            i0 = p[j0]; delta = INT_MAX / 4; j1 = 0;
            for (j = 1; j <= cols; j++) {
                if (used[j]) continue;
                int cur = cost[(i0 - 1) * cols + (j - 1)] - u[i0] - v[j];
                if (cur < minv[j]) { minv[j] = cur; way[j] = j0; }
                if (minv[j] < delta) { delta = minv[j]; j1 = j; }
            }
            for (j = 0; j <= cols; j++) {
                if (used[j]) { u[p[j]] += delta; v[j] -= delta; }
                else          minv[j]  -= delta;
            }
            j0 = j1;
            if (p[j0] == 0) break;
        }
        while (1) { j1 = way[j0]; p[j0] = p[j1]; j0 = j1; if (j0 == 0) break; }
    }

    best = 0;
    for (j = 1; j <= cols; j++) {
        i = p[j];
        if (i != 0) best += cost[(i - 1) * cols + (j - 1)];
    }
    free(u); free(v); free(p); free(way); free(minv); free(used);
    return best;
}

/* ===== HELPER: initialise global map from a string array =============== */
static State parse_map(const char **map_lines)
{
    int row, col;
    State init;
    init.num_boxes = 0;
    init.h_n       = 0.0f;
    G_num_goals    = 0;

    G_ROWS = 0;
    while (map_lines[G_ROWS] != NULL) G_ROWS++;
    G_COLS = (int)strlen(map_lines[0]);

    for (row = 0; row < G_ROWS; row++) {
        for (col = 0; col < G_COLS; col++) {
            char ch = map_lines[row][col];
            switch (ch) {
                case '#':
                    G_board[row][col] = '#'; break;
                case '.':
                    G_board[row][col] = '.';
                    G_goal_rows[G_num_goals] = row;
                    G_goal_cols[G_num_goals] = col;
                    G_num_goals++;
                    break;
                case '$':   /* box on floor */
                    G_board[row][col] = ' ';
                    init.box_rows[init.num_boxes] = row;
                    init.box_cols[init.num_boxes] = col;
                    init.num_boxes++;
                    break;
                case '*':   /* box on goal */
                    G_board[row][col] = '.';
                    G_goal_rows[G_num_goals] = row;
                    G_goal_cols[G_num_goals] = col;
                    G_num_goals++;
                    init.box_rows[init.num_boxes] = row;
                    init.box_cols[init.num_boxes] = col;
                    init.num_boxes++;
                    break;
                case '@':   /* player on floor */
                    G_board[row][col] = ' ';
                    init.player_row   = row;
                    init.player_col   = col;
                    break;
                case '+':   /* player on goal */
                    G_board[row][col] = '.';
                    G_goal_rows[G_num_goals] = row;
                    G_goal_cols[G_num_goals] = col;
                    G_num_goals++;
                    init.player_row = row;
                    init.player_col = col;
                    break;
                default:    /* space or unknown */
                    G_board[row][col] = ' '; break;
            }
        }
    }
    sort_boxes(&init);
    return init;
}

/* ======================================================================= */
/*                      COMPULSORY FRAMEWORK FUNCTIONS                      */
/* ======================================================================= */

/* --------------------------------------------------------------------------
 * Create_State
 *   Called once from main() to build the root state.
 *   Also initialises the global map (G_board, G_goal_*, etc.).
 * ------------------------------------------------------------------------ */
State *Create_State()
{
    int choice;
    State *state = (State *)malloc(sizeof(State));
    if (state == NULL) Warning_Memory_Allocation();

    printf("\nAvailable puzzle maps:\n\n");
    printf("  MAP 1 (single box):\n");
    for (int r = 0; MAP1[r] != NULL; r++) printf("    %s\n", MAP1[r]);
    printf("\n  MAP 2 (two boxes):\n");
    for (int r = 0; MAP2[r] != NULL; r++) printf("    %s\n", MAP2[r]);
    printf("\n  Legend:  # wall   @ player   $ box   . goal   * box-on-goal\n");

    do {
        printf("\nSelect map (1 or 2): ");
        scanf("%d", &choice);
    } while (choice != 1 && choice != 2);

    if (choice == 1)
        *state = parse_map(MAP1);
    else
        *state = parse_map(MAP2);

    printf("\nInitial board:\n");
    Print_State(state);
    printf("\n");

    return state;
}

/* --------------------------------------------------------------------------
 * Print_State
 *   Renders the board with player and box overlays.
 * ------------------------------------------------------------------------ */
void Print_State(const State *const state)
{
    int r, c, b;
    printf("\n");
    for (r = 0; r < G_ROWS; r++) {
        printf("  ");
        for (c = 0; c < G_COLS; c++) {
            int is_player = (state->player_row == r && state->player_col == c);
            int is_box    = 0;
            for (b = 0; b < state->num_boxes; b++)
                if (state->box_rows[b] == r && state->box_cols[b] == c) { is_box = 1; break; }

            char cell = G_board[r][c];
            if      (is_player && cell == '.') putchar('+');
            else if (is_player)                putchar('@');
            else if (is_box    && cell == '.') putchar('*');
            else if (is_box)                   putchar('$');
            else                               putchar(cell);
        }
        putchar('\n');
    }
}

/* --------------------------------------------------------------------------
 * Print_Action
 *   Decodes the integer action and prints a human-readable description.
 * ------------------------------------------------------------------------ */
void Print_Action(const enum ACTIONS action)
{
    int act  = (int)action;
    int cell = act / 4;
    int dir  = act % 4;
    if (G_COLS > 0)
        printf("Push box at (%d,%d) %s",
               cell / G_COLS, cell % G_COLS, DIR_NAMES[dir]);
    else
        printf("%s", DIR_NAMES[dir]);
}

/* --------------------------------------------------------------------------
 * Result
 *   Returns TRUE and fills trans_model if the given action is a legal push
 *   from parent_state, otherwise returns FALSE.
 *
 *   Action encoding: action = cell_index * 4 + dir
 *     cell_index = row * G_COLS + col   (position of the box to push)
 *     dir: 0=UP 1=DOWN 2=LEFT 3=RIGHT
 * ------------------------------------------------------------------------ */
int Result(const State *const parent_state, const enum ACTIONS action,
           Transition_Model *const trans_model)
{
    int act  = (int)action;
    int cell = act / 4;
    int dir  = act % 4;
    int brow = cell / G_COLS;
    int bcol = cell % G_COLS;
    int b, i;

    /* bounds check for the cell encoded in the action */
    if (G_COLS == 0 || G_ROWS == 0) return FALSE;
    if (brow < 0 || brow >= G_ROWS || bcol < 0 || bcol >= G_COLS) return FALSE;

    /* is there a box at (brow, bcol)? */
    int box_idx = -1;
    for (b = 0; b < parent_state->num_boxes; b++)
        if (parent_state->box_rows[b] == brow && parent_state->box_cols[b] == bcol) {
            box_idx = b; break;
        }
    if (box_idx < 0) return FALSE;

    /* push position: player must stand one step behind the box */
    int push_r = brow - DR[dir];
    int push_c = bcol - DC[dir];
    if (push_r < 0 || push_r >= G_ROWS || push_c < 0 || push_c >= G_COLS) return FALSE;
    if (G_board[push_r][push_c] == '#') return FALSE;

    /* another box at the push position? */
    for (b = 0; b < parent_state->num_boxes; b++)
        if (b != box_idx &&
            parent_state->box_rows[b] == push_r && parent_state->box_cols[b] == push_c)
            return FALSE;

    /* destination of the box after the push */
    int dest_r = brow + DR[dir];
    int dest_c = bcol + DC[dir];
    if (dest_r < 0 || dest_r >= G_ROWS || dest_c < 0 || dest_c >= G_COLS) return FALSE;
    if (G_board[dest_r][dest_c] == '#') return FALSE;

    /* another box already occupies the destination? */
    for (b = 0; b < parent_state->num_boxes; b++)
        if (b != box_idx &&
            parent_state->box_rows[b] == dest_r && parent_state->box_cols[b] == dest_c)
            return FALSE;

    /* can the player reach push_r, push_c?
       (skip the box being pushed so the BFS path can include its current cell) */
    int at_push = (parent_state->player_row == push_r && parent_state->player_col == push_c);
    if (!at_push && !bfs_reachable(parent_state, push_r, push_c, brow, bcol))
        return FALSE;

    /* deadlock detection: if destination is not a goal and is a corner
       (wall on one vertical side AND one horizontal side), reject */
    if (G_board[dest_r][dest_c] != '.') {
        int wall_u = (dest_r == 0        || G_board[dest_r-1][dest_c] == '#');
        int wall_d = (dest_r == G_ROWS-1 || G_board[dest_r+1][dest_c] == '#');
        int wall_l = (dest_c == 0        || G_board[dest_r][dest_c-1] == '#');
        int wall_r = (dest_c == G_COLS-1 || G_board[dest_r][dest_c+1] == '#');
        if ((wall_u || wall_d) && (wall_l || wall_r)) return FALSE;
    }

    /* build the new state */
    State ns = *parent_state;
    ns.box_rows[box_idx] = dest_r;
    ns.box_cols[box_idx] = dest_c;
    ns.player_row = brow;
    ns.player_col = bcol;
    ns.h_n = 0.0f;

    /* keep boxes in canonical (sorted) order so hash keys are consistent */
    /* bubble-sort the changed entry */
    for (i = 0; i < ns.num_boxes - 1; i++) {
        int j;
        for (j = i + 1; j < ns.num_boxes; j++) {
            if (ns.box_rows[j] < ns.box_rows[i] ||
                (ns.box_rows[j] == ns.box_rows[i] && ns.box_cols[j] < ns.box_cols[i])) {
                int tr = ns.box_rows[i]; ns.box_rows[i] = ns.box_rows[j]; ns.box_rows[j] = tr;
                int tc = ns.box_cols[i]; ns.box_cols[i] = ns.box_cols[j]; ns.box_cols[j] = tc;
            }
        }
    }

    trans_model->new_state = ns;
    trans_model->step_cost = 1.0f;
    return TRUE;
}

/* --------------------------------------------------------------------------
 * Compute_Heuristic_Function
 *   Returns the minimum total Manhattan distance from boxes to goals,
 *   computed via the Hungarian (optimal assignment) algorithm.
 *   goal_state is NULL when PREDETERMINED_GOAL_STATE == 0.
 * ------------------------------------------------------------------------ */
float Compute_Heuristic_Function(const State *const state, const State *const goal)
{
    int i, j;
    int cost[MAX_BOXES * MAX_BOXES];
    (void)goal; /* goal is map-determined; the parameter is unused */

    if (state->num_boxes == 0 || G_num_goals == 0) return 0.0f;

    for (i = 0; i < state->num_boxes; i++)
        for (j = 0; j < G_num_goals; j++) {
            int dr = state->box_rows[i] - G_goal_rows[j];
            int dc = state->box_cols[i] - G_goal_cols[j];
            cost[i * G_num_goals + j] = abs(dr) + abs(dc);
        }

    return (float)hungarian_min_cost(cost, state->num_boxes, G_num_goals);
}

/* --------------------------------------------------------------------------
 * Goal_Test
 *   Returns TRUE when every box is on a goal cell.
 *   goal_state is ignored (PREDETERMINED_GOAL_STATE == 0).
 * ------------------------------------------------------------------------ */
int Goal_Test(const State *const state, const State *const goal_state)
{
    int i, j;
    (void)goal_state;

    for (i = 0; i < state->num_boxes; i++) {
        int on_goal = FALSE;
        for (j = 0; j < G_num_goals; j++)
            if (state->box_rows[i] == G_goal_rows[j] &&
                state->box_cols[i] == G_goal_cols[j]) { on_goal = TRUE; break; }
        if (!on_goal) return FALSE;
    }
    return TRUE;
}
