#include "data_types.h"
#include "HashTable.h"
#include "GRAPH_SEARCH.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ===== PROBLEM-SPECIFIC ================================================ */

/* Encode the state as "player_row,player_col:box0_row,box0_col;box1_row,box1_col;"
   Boxes must be sorted (canonical order) before this is called so that
   two states with the same box configuration produce the same key. */
void Generate_HashTable_Key(const State *const state, unsigned char *key)
{
    int i, used;
    used = sprintf((char *)key, "%d,%d:", state->player_row, state->player_col);
    for (i = 0; i < state->num_boxes; i++)
        used += sprintf((char *)key + used, "%d,%d;", state->box_rows[i], state->box_cols[i]);

    if (used >= MAX_KEY_SIZE) {
        printf("ERROR: MAX_KEY_SIZE exceeded in Generate_HashTable_Key.\n");
        exit(-1);
    }
}

/* ===== FRAMEWORK (unchanged from professor's template) ================= */

static int is_prime(const unsigned int x)
{
    unsigned int i;
    if (x < 2) return FALSE;
    for (i = 2; i <= x / 2; i++)
        if (x % i == 0) return FALSE;
    return TRUE;
}

static unsigned int next_prime(unsigned int x)
{
    while (is_prime(x) == FALSE)
        x++;
    return x;
}

static unsigned int hash_func(const char *key, const int size)
{
    unsigned int hash = 0, i;
    const int a           = 151;
    const int length_key  = (int)strlen(key);
    for (i = 0; i < (unsigned)length_key; i++) {
        hash += (unsigned int)pow(a, length_key - (int)(i + 1)) * (unsigned char)key[i];
        hash  = hash % (unsigned)size;
    }
    return hash;
}

Hash_Table *New_Hash_Table(const int size)
{
    Hash_Table *ht = (Hash_Table *)malloc(sizeof(Hash_Table));
    if (ht == NULL) Warning_Memory_Allocation();
    ht->size  = next_prime((unsigned)size);
    ht->count = 0;
    ht->State_Key = (unsigned char **)calloc(ht->size, sizeof(unsigned char *));
    if (ht->State_Key == NULL) Warning_Memory_Allocation();
    return ht;
}

void ht_insert(Hash_Table *ht, const State *const state)
{
    char key[MAX_KEY_SIZE];
    Generate_HashTable_Key(state, (unsigned char *)key);
    ht_insert_key(ht, key);
}

void ht_insert_key(Hash_Table *ht, const char *key)
{
    unsigned int index;
    unsigned int load    = ht->count * 100 / ht->size;
    unsigned int new_size;

    if (load > HASH_TABLE_INCREASING_RATE) {
        new_size = ht->size * 2;
        Resize_Hash_Table(ht, (int)new_size);
    }
    if (ht->size == ht->count) {
        printf("ERROR: Hash table is full.\n");
        exit(-1);
    }

    index = hash_func(key, (int)ht->size);
    while (ht->State_Key[index] != NULL) {
        index = (index == ht->size - 1) ? 0 : index + 1;
    }

    ht->State_Key[index] = (unsigned char *)malloc(MAX_KEY_SIZE * sizeof(unsigned char));
    if (ht->State_Key[index] == NULL) Warning_Memory_Allocation();
    strcpy((char *)ht->State_Key[index], key);
    ht->count++;
}

int ht_search(Hash_Table *ht, const State *const state)
{
    char key[MAX_KEY_SIZE];
    unsigned int first_index, index;

    Generate_HashTable_Key(state, (unsigned char *)key);
    index       = hash_func(key, (int)ht->size);
    first_index = index;

    while (ht->State_Key[index] != NULL) {
        if (strcmp((char *)ht->State_Key[index], key) == 0)
            return TRUE;
        index = (index == ht->size - 1) ? 0 : index + 1;
        if (index == first_index) return FALSE;
    }
    return FALSE;
}

void Resize_Hash_Table(Hash_Table *ht, const int size)
{
    int i;
    unsigned int  temp_size, temp_count;
    unsigned char **temp_key;
    Hash_Table *new_ht = New_Hash_Table(size);

    for (i = 0; i < (int)ht->size; i++)
        if (ht->State_Key[i] != NULL)
            ht_insert_key(new_ht, (char *)ht->State_Key[i]);

    temp_size    = ht->size;      ht->size     = new_ht->size;    new_ht->size  = temp_size;
    temp_count   = ht->count;     ht->count    = new_ht->count;   new_ht->count = temp_count;
    temp_key     = ht->State_Key; ht->State_Key = new_ht->State_Key; new_ht->State_Key = temp_key;

    Delete_Hash_Table(new_ht);
}

void Delete_Hash_Table(Hash_Table *ht)
{
    unsigned int i;
    for (i = 0; i < ht->size; i++)
        if (ht->State_Key[i] != NULL)
            free(ht->State_Key[i]);
    free(ht->State_Key);
    free(ht);
}

void Show_Hash_Table(Hash_Table *ht)
{
    unsigned int i;
    printf("\nHASH TABLE (size=%u, count=%u):\n", ht->size, ht->count);
    for (i = 0; i < ht->size; i++)
        if (ht->State_Key[i] != NULL)
            printf("  [%u] %s\n", i, ht->State_Key[i]);
}
