#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "data_types.h"

/* ===== PROBLEM-SPECIFIC CONSTANTS ====================================== */
#define HASH_TABLE_BASED_SIZE    1009   /* prime starting size */
#define HASH_TABLE_INCREASING_RATE 70  /* resize when load exceeds 70% */
/* Key format: "pr,pc:br0,bc0;br1,bc1;"  -- 64 bytes is generous for 2 boxes */
#define MAX_KEY_SIZE  64

/* ===== FRAMEWORK TYPES (do not change) ================================= */
typedef struct {
    unsigned int  size;
    unsigned int  count;
    unsigned char **State_Key;
} Hash_Table;

/* ===== PROBLEM-SPECIFIC DECLARATION ==================================== */
void Generate_HashTable_Key(const State *const state, unsigned char *key);

/* ===== FRAMEWORK DECLARATIONS (do not change) ========================== */
Hash_Table* New_Hash_Table(const int size);
void        Resize_Hash_Table(Hash_Table *ht, const int size);
void        Delete_Hash_Table(Hash_Table *ht);
void        ht_insert(Hash_Table *ht, const State *const state);
void        ht_insert_key(Hash_Table *ht, const char *key);
int         ht_search(Hash_Table *ht, const State *const state);
void        Show_Hash_Table(Hash_Table *ht);

#endif
