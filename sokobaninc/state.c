#include "state.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>

/* ------------------------------------------------------------------ */
/* Position utilities                                                   */
/* ------------------------------------------------------------------ */

bool position_equals(Position a, Position b) {
    return a.row == b.row && a.col == b.col;
}

int position_compare(const void *a, const void *b) {
    const Position *pa = (const Position *)a;
    const Position *pb = (const Position *)b;
    if (pa->row != pb->row) return pa->row - pb->row;
    return pa->col - pb->col;
}

/* ------------------------------------------------------------------ */
/* State utilities                                                      */
/* ------------------------------------------------------------------ */

void state_sort_boxes(State *s) {
    qsort(s->boxes, s->num_boxes, sizeof(Position), position_compare);
}

State state_copy(const State *s) {
    State copy;
    copy.player    = s->player;
    copy.num_boxes = s->num_boxes;
    copy.boxes     = (Position *)malloc(s->num_boxes * sizeof(Position));
    memcpy(copy.boxes, s->boxes, s->num_boxes * sizeof(Position));
    return copy;
}

void state_free(State *s) {
    free(s->boxes);
    s->boxes     = NULL;
    s->num_boxes = 0;
}

bool state_equals(const State *a, const State *b) {
    if (!position_equals(a->player, b->player)) return false;
    if (a->num_boxes != b->num_boxes)           return false;
    for (int i = 0; i < a->num_boxes; i++) {
        if (!position_equals(a->boxes[i], b->boxes[i])) return false;
    }
    return true;
}

/* FNV-1a 64-bit hash */
static uint64_t fnv1a_64(uint64_t hash, const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        hash ^= p[i];
        hash *= 0x00000100000001B3ULL;
    }
    return hash;
}

uint64_t state_hash(const State *s) {
    uint64_t h = 0xcbf29ce484222325ULL;
    h = fnv1a_64(h, &s->player, sizeof(Position));
    for (int i = 0; i < s->num_boxes; i++) {
        h = fnv1a_64(h, &s->boxes[i], sizeof(Position));
    }
    return h;
}

/* ------------------------------------------------------------------ */
/* Level utilities                                                      */
/* ------------------------------------------------------------------ */

void level_free(Level *l) {
    free(l->walls);
    free(l->goals);
    state_free(&l->init_state);
    l->walls    = NULL;
    l->goals    = NULL;
    l->num_walls = 0;
    l->num_goals = 0;
}

/* ------------------------------------------------------------------ */
/* PositionSet                                                          */
/* ------------------------------------------------------------------ */

static uint64_t position_hash(Position p) {
    uint64_t h = 0xcbf29ce484222325ULL;
    h = fnv1a_64(h, &p, sizeof(Position));
    return h;
}

PositionSet position_set_create(int initial_capacity) {
    PositionSet ps;
    ps.capacity  = initial_capacity > 0 ? initial_capacity : 16;
    ps.count     = 0;
    ps.entries   = (Position *)calloc(ps.capacity, sizeof(Position));
    ps.occupied  = (bool *)calloc(ps.capacity, sizeof(bool));
    return ps;
}

PositionSet position_set_from_array(const Position *arr, int count) {
    int cap = count * 2 + 16;
    PositionSet ps = position_set_create(cap);
    for (int i = 0; i < count; i++) {
        position_set_add(&ps, arr[i]);
    }
    return ps;
}

void position_set_free(PositionSet *ps) {
    free(ps->entries);
    free(ps->occupied);
    ps->entries  = NULL;
    ps->occupied = NULL;
    ps->capacity = ps->count = 0;
}

bool position_set_contains(const PositionSet *ps, Position p) {
    uint64_t h   = position_hash(p);
    int      idx = (int)(h % (uint64_t)ps->capacity);
    for (int i = 0; i < ps->capacity; i++) {
        int probe = (idx + i) % ps->capacity;
        if (!ps->occupied[probe]) return false;
        if (position_equals(ps->entries[probe], p)) return true;
    }
    return false;
}

static void position_set_add_internal(PositionSet *ps, Position p) {
    uint64_t h   = position_hash(p);
    int      idx = (int)(h % (uint64_t)ps->capacity);
    for (int i = 0; i < ps->capacity; i++) {
        int probe = (idx + i) % ps->capacity;
        if (!ps->occupied[probe]) {
            ps->entries[probe]  = p;
            ps->occupied[probe] = true;
            ps->count++;
            return;
        }
        if (position_equals(ps->entries[probe], p)) return; /* already in */
    }
}

void position_set_add(PositionSet *ps, Position p) {
    if (ps->count * 10 >= ps->capacity * 7) { /* load factor > 0.7 */
        int old_cap = ps->capacity;
        Position *old_entries  = ps->entries;
        bool     *old_occupied = ps->occupied;
        ps->capacity  = old_cap * 2;
        ps->count     = 0;
        ps->entries   = (Position *)calloc(ps->capacity, sizeof(Position));
        ps->occupied  = (bool *)calloc(ps->capacity, sizeof(bool));
        for (int i = 0; i < old_cap; i++) {
            if (old_occupied[i]) position_set_add_internal(ps, old_entries[i]);
        }
        free(old_entries);
        free(old_occupied);
    }
    position_set_add_internal(ps, p);
}

void position_set_remove(PositionSet *ps, Position p) {
    /* Tombstone deletion: mark slot as empty and re-insert cluster */
    uint64_t h   = position_hash(p);
    int      idx = (int)(h % (uint64_t)ps->capacity);
    for (int i = 0; i < ps->capacity; i++) {
        int probe = (idx + i) % ps->capacity;
        if (!ps->occupied[probe]) return;
        if (position_equals(ps->entries[probe], p)) {
            ps->occupied[probe] = false;
            ps->count--;
            /* Re-insert following elements in same cluster */
            int j = (probe + 1) % ps->capacity;
            while (ps->occupied[j]) {
                Position tmp = ps->entries[j];
                ps->occupied[j] = false;
                ps->count--;
                position_set_add(ps, tmp);
                j = (j + 1) % ps->capacity;
            }
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* StateSet                                                             */
/* ------------------------------------------------------------------ */

StateSet state_set_create(int initial_capacity) {
    StateSet ss;
    ss.capacity  = initial_capacity > 0 ? initial_capacity : 64;
    ss.count     = 0;
    ss.entries   = (State *)calloc(ss.capacity, sizeof(State));
    ss.occupied  = (bool *)calloc(ss.capacity, sizeof(bool));
    return ss;
}

void state_set_free(StateSet *ss) {
    for (int i = 0; i < ss->capacity; i++) {
        if (ss->occupied[i]) state_free(&ss->entries[i]);
    }
    free(ss->entries);
    free(ss->occupied);
    ss->entries  = NULL;
    ss->occupied = NULL;
    ss->capacity = ss->count = 0;
}

bool state_set_contains(const StateSet *ss, const State *s) {
    uint64_t h   = state_hash(s);
    int      idx = (int)(h % (uint64_t)ss->capacity);
    for (int i = 0; i < ss->capacity; i++) {
        int probe = (idx + i) % ss->capacity;
        if (!ss->occupied[probe]) return false;
        if (state_equals(&ss->entries[probe], s)) return true;
    }
    return false;
}

static void state_set_add_internal(StateSet *ss, const State *s) {
    uint64_t h   = state_hash(s);
    int      idx = (int)(h % (uint64_t)ss->capacity);
    for (int i = 0; i < ss->capacity; i++) {
        int probe = (idx + i) % ss->capacity;
        if (!ss->occupied[probe]) {
            ss->entries[probe]  = state_copy(s);
            ss->occupied[probe] = true;
            ss->count++;
            return;
        }
        if (state_equals(&ss->entries[probe], s)) return;
    }
}

void state_set_add(StateSet *ss, const State *s) {
    if (ss->count * 10 >= ss->capacity * 7) {
        int    old_cap      = ss->capacity;
        State *old_entries  = ss->entries;
        bool  *old_occupied = ss->occupied;
        ss->capacity  = old_cap * 2;
        ss->count     = 0;
        ss->entries   = (State *)calloc(ss->capacity, sizeof(State));
        ss->occupied  = (bool *)calloc(ss->capacity, sizeof(bool));
        for (int i = 0; i < old_cap; i++) {
            if (old_occupied[i]) {
                state_set_add_internal(ss, &old_entries[i]);
                state_free(&old_entries[i]);
            }
        }
        free(old_entries);
        free(old_occupied);
    }
    state_set_add_internal(ss, s);
}

/* ------------------------------------------------------------------ */
/* StateIntMap                                                          */
/* ------------------------------------------------------------------ */

StateIntMap state_int_map_create(int initial_capacity) {
    StateIntMap m;
    m.capacity  = initial_capacity > 0 ? initial_capacity : 64;
    m.count     = 0;
    m.keys      = (State *)calloc(m.capacity, sizeof(State));
    m.values    = (int *)calloc(m.capacity, sizeof(int));
    m.occupied  = (bool *)calloc(m.capacity, sizeof(bool));
    return m;
}

void state_int_map_free(StateIntMap *m) {
    for (int i = 0; i < m->capacity; i++) {
        if (m->occupied[i]) state_free(&m->keys[i]);
    }
    free(m->keys);
    free(m->values);
    free(m->occupied);
    m->keys = NULL; m->values = NULL; m->occupied = NULL;
    m->capacity = m->count = 0;
}

bool state_int_map_get(const StateIntMap *m, const State *key, int *out_value) {
    uint64_t h   = state_hash(key);
    int      idx = (int)(h % (uint64_t)m->capacity);
    for (int i = 0; i < m->capacity; i++) {
        int probe = (idx + i) % m->capacity;
        if (!m->occupied[probe]) return false;
        if (state_equals(&m->keys[probe], key)) {
            *out_value = m->values[probe];
            return true;
        }
    }
    return false;
}

int state_int_map_get_or_inf(const StateIntMap *m, const State *key) {
    int v;
    if (state_int_map_get(m, key, &v)) return v;
    return INT_MAX;
}

static void state_int_map_set_internal(StateIntMap *m, const State *key, int value) {
    uint64_t h   = state_hash(key);
    int      idx = (int)(h % (uint64_t)m->capacity);
    for (int i = 0; i < m->capacity; i++) {
        int probe = (idx + i) % m->capacity;
        if (!m->occupied[probe]) {
            m->keys[probe]     = state_copy(key);
            m->values[probe]   = value;
            m->occupied[probe] = true;
            m->count++;
            return;
        }
        if (state_equals(&m->keys[probe], key)) {
            m->values[probe] = value;
            return;
        }
    }
}

void state_int_map_set(StateIntMap *m, const State *key, int value) {
    if (m->count * 10 >= m->capacity * 7) {
        int    old_cap      = m->capacity;
        State *old_keys     = m->keys;
        int   *old_values   = m->values;
        bool  *old_occupied = m->occupied;
        m->capacity  = old_cap * 2;
        m->count     = 0;
        m->keys      = (State *)calloc(m->capacity, sizeof(State));
        m->values    = (int *)calloc(m->capacity, sizeof(int));
        m->occupied  = (bool *)calloc(m->capacity, sizeof(bool));
        for (int i = 0; i < old_cap; i++) {
            if (old_occupied[i]) {
                state_int_map_set_internal(m, &old_keys[i], old_values[i]);
                state_free(&old_keys[i]);
            }
        }
        free(old_keys); free(old_values); free(old_occupied);
    }
    state_int_map_set_internal(m, key, value);
}

/* ------------------------------------------------------------------ */
/* StateParentMap                                                       */
/* ------------------------------------------------------------------ */

StateParentMap state_parent_map_create(int initial_capacity) {
    StateParentMap m;
    m.capacity  = initial_capacity > 0 ? initial_capacity : 64;
    m.count     = 0;
    m.keys      = (State *)calloc(m.capacity, sizeof(State));
    m.values    = (ParentEntry *)calloc(m.capacity, sizeof(ParentEntry));
    m.occupied  = (bool *)calloc(m.capacity, sizeof(bool));
    return m;
}

void state_parent_map_free(StateParentMap *m) {
    for (int i = 0; i < m->capacity; i++) {
        if (m->occupied[i]) {
            state_free(&m->keys[i]);
            if (m->values[i].has_parent) state_free(&m->values[i].parent_state);
        }
    }
    free(m->keys); free(m->values); free(m->occupied);
    m->keys = NULL; m->values = NULL; m->occupied = NULL;
    m->capacity = m->count = 0;
}

bool state_parent_map_get(const StateParentMap *m, const State *key, ParentEntry *out) {
    uint64_t h   = state_hash(key);
    int      idx = (int)(h % (uint64_t)m->capacity);
    for (int i = 0; i < m->capacity; i++) {
        int probe = (idx + i) % m->capacity;
        if (!m->occupied[probe]) return false;
        if (state_equals(&m->keys[probe], key)) {
            *out = m->values[probe];
            return true;
        }
    }
    return false;
}

bool state_parent_map_contains(const StateParentMap *m, const State *key) {
    ParentEntry dummy;
    return state_parent_map_get(m, key, &dummy);
}

static void state_parent_map_set_internal(StateParentMap *m, const State *key, const ParentEntry *value) {
    uint64_t h   = state_hash(key);
    int      idx = (int)(h % (uint64_t)m->capacity);
    for (int i = 0; i < m->capacity; i++) {
        int probe = (idx + i) % m->capacity;
        if (!m->occupied[probe]) {
            m->keys[probe]     = state_copy(key);
            /* Deep copy ParentEntry */
            m->values[probe].has_parent = value->has_parent;
            m->values[probe].push       = value->push;
            if (value->has_parent)
                m->values[probe].parent_state = state_copy(&value->parent_state);
            m->occupied[probe] = true;
            m->count++;
            return;
        }
        if (state_equals(&m->keys[probe], key)) {
            /* Update: free old parent if any */
            if (m->values[probe].has_parent) state_free(&m->values[probe].parent_state);
            m->values[probe].has_parent = value->has_parent;
            m->values[probe].push       = value->push;
            if (value->has_parent)
                m->values[probe].parent_state = state_copy(&value->parent_state);
            return;
        }
    }
}

void state_parent_map_set(StateParentMap *m, const State *key, const ParentEntry *value) {
    if (m->count * 10 >= m->capacity * 7) {
        int          old_cap      = m->capacity;
        State       *old_keys     = m->keys;
        ParentEntry *old_values   = m->values;
        bool        *old_occupied = m->occupied;
        m->capacity  = old_cap * 2;
        m->count     = 0;
        m->keys      = (State *)calloc(m->capacity, sizeof(State));
        m->values    = (ParentEntry *)calloc(m->capacity, sizeof(ParentEntry));
        m->occupied  = (bool *)calloc(m->capacity, sizeof(bool));
        for (int i = 0; i < old_cap; i++) {
            if (old_occupied[i]) {
                state_parent_map_set_internal(m, &old_keys[i], &old_values[i]);
                state_free(&old_keys[i]);
                if (old_values[i].has_parent) state_free(&old_values[i].parent_state);
            }
        }
        free(old_keys); free(old_values); free(old_occupied);
    }
    state_parent_map_set_internal(m, key, value);
}