#ifndef CUSTOM_HASH_TABLE_H
#define CUSTOM_HASH_TABLE_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define TABLE_SIZE 1024

typedef struct node {
    char *key;
    bool value;
    struct node *next;
} node_t;

node_t *table[TABLE_SIZE] = {NULL};

unsigned int hash(const char *key) {
    unsigned long hash_val = 5381;
    int c;
    while ((c = *key++)) {
        hash_val = ((hash_val << 5) + hash_val) + c;
    }
    return hash_val % TABLE_SIZE;
}

void insert(const char *key, bool value) {
    unsigned int idx = hash(key);
    node_t *n = (node_t *)malloc(sizeof(node_t));
    n->key = strdup(key);
    n->value = value;
    n->next = table[idx];
    table[idx] = n;
}

bool lookup(const char *key, bool *found) {
    unsigned int idx = hash(key);
    node_t *n = table[idx];
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