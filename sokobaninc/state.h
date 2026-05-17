#ifndef STATE_H
#define STATE_H


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


typedef struct {
    int row;
    int col;
} Position;

typedef struct {
    Position position;
    char     direction[8]; 
} Push;

typedef struct {
    Position  player;
    Position *boxes;    
    int       num_boxes;
} State;

typedef struct {
    int       game_id;
    int       width;
    int       height;
    Position *walls;
    int       num_walls;
    Position *goals;
    int       num_goals;
    State     init_state;
} Level;


bool position_equals(Position a, Position b);
int  position_compare(const void *a, const void *b); /* for qsort */


State    state_copy(const State *s);
void     state_free(State *s);
bool     state_equals(const State *a, const State *b);
uint64_t state_hash(const State *s);
void     state_sort_boxes(State *s);


void level_free(Level *l);


typedef struct {
    Position *entries;
    bool     *occupied;
    int       capacity;
    int       count;
} PositionSet;

PositionSet position_set_create(int initial_capacity);
PositionSet position_set_from_array(const Position *arr, int count);
void        position_set_free(PositionSet *ps);
bool        position_set_contains(const PositionSet *ps, Position p);
void        position_set_add(PositionSet *ps, Position p);
void        position_set_remove(PositionSet *ps, Position p);


typedef struct {
    State *entries;
    bool  *occupied;
    int    capacity;
    int    count;
} StateSet;

StateSet state_set_create(int initial_capacity);
void     state_set_free(StateSet *ss);
bool     state_set_contains(const StateSet *ss, const State *s);
void     state_set_add(StateSet *ss, const State *s);

typedef struct {
    State *keys;
    int   *values;
    bool  *occupied;
    int    capacity;
    int    count;
} StateIntMap;

StateIntMap state_int_map_create(int initial_capacity);
void        state_int_map_free(StateIntMap *m);
bool        state_int_map_get(const StateIntMap *m, const State *key, int *out_value);
void        state_int_map_set(StateIntMap *m, const State *key, int value);
int         state_int_map_get_or_inf(const StateIntMap *m, const State *key);

typedef struct {
    bool  has_parent;
    State parent_state;
    Push  push;
} ParentEntry;

typedef struct {
    State       *keys;
    ParentEntry *values;
    bool        *occupied;
    int          capacity;
    int          count;
} StateParentMap;

StateParentMap state_parent_map_create(int initial_capacity);
void           state_parent_map_free(StateParentMap *m);
bool           state_parent_map_get(const StateParentMap *m, const State *key, ParentEntry *out);
void           state_parent_map_set(StateParentMap *m, const State *key, const ParentEntry *value);
bool           state_parent_map_contains(const StateParentMap *m, const State *key);

#endif