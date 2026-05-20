#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>

#define HASH_TABLE_BASED_SIZE 262144
#define HASH_TABLE_INCREASING_RATE 2
#define MAX_KEY_SIZE 4096

typedef struct HashEntry {
    char *key;
    struct HashEntry *next;
} HashEntry;

typedef struct HashTable {
    HashEntry **buckets;
    int size;
} HashTable;

unsigned long Generate_HashTable_Key(const char *s);
void hash_table_init(HashTable *table, int size);
bool hash_table_contains(HashTable *table, const char *key);
void hash_table_insert(HashTable *table, char *key);
void hash_table_free(HashTable *table);

#endif
