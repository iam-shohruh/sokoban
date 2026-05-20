#include "HashTable.h"
#include <stdlib.h>
#include <string.h>

unsigned long Generate_HashTable_Key(const char *s) {
    unsigned long h = 5381;
    while (*s) {
        h = ((h << 5) + h) + (unsigned char)(*s++);
    }
    return h;
}

void hash_table_init(HashTable *table, int size) {
    table->size = size;
    table->buckets = calloc((size_t)table->size, sizeof(HashEntry *));
}

bool hash_table_contains(HashTable *table, const char *key) {
    unsigned long h = Generate_HashTable_Key(key) % (unsigned long)table->size;
    for (HashEntry *entry = table->buckets[h]; entry; entry = entry->next) {
        if (strcmp(entry->key, key) == 0) {
            return true;
        }
    }
    return false;
}

void hash_table_insert(HashTable *table, char *key) {
    unsigned long h = Generate_HashTable_Key(key) % (unsigned long)table->size;
    HashEntry *entry = malloc(sizeof(HashEntry));
    entry->key = key;
    entry->next = table->buckets[h];
    table->buckets[h] = entry;
}

void hash_table_free(HashTable *table) {
    if (!table || !table->buckets) {
        return;
    }

    for (int i = 0; i < table->size; i++) {
        HashEntry *entry = table->buckets[i];
        while (entry) {
            HashEntry *next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }

    free(table->buckets);
    table->buckets = NULL;
    table->size = 0;
}
