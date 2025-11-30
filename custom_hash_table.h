#ifndef CUSTOM_HASH_TABLE_H
#define CUSTOM_HASH_TABLE_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define TABLE_SIZE 100

typedef struct node {
    char *key;
    bool value;
    struct node *next;
} hash_node_t;

hash_node_t *table[TABLE_SIZE] = {NULL};

unsigned int hash(const char *key) {
    unsigned long hash_val = 5381;
    int c;
    while ((c = *key++)) {
        hash_val = ((hash_val << 5) + hash_val) + c;
    }
    return hash_val % TABLE_SIZE;
}

void insert(hash_node_t * table[TABLE_SIZE], const char *key, bool value) {
    unsigned int idx = hash(key);
    hash_node_t *n = (hash_node_t *)malloc(sizeof(hash_node_t));
    n->key = strdup(key);
    n->value = value;
    n->next = table[idx];
    table[idx] = n;
}

bool lookup(hash_node_t * table[TABLE_SIZE], const char *key, bool *found) {
    unsigned int idx = hash(key);
    hash_node_t *n = table[idx];
    while (n) {
        if (strcmp(n->key, key) == 0) {
            if (found) *found = true;
            return n->value;
        }
        n = n->next;
    }
    if (found) *found = false;
    return false; 
}

#endif