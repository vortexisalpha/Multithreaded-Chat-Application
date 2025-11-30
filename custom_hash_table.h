#ifndef CUSTOM_HASH_TABLE_H
#define CUSTOM_HASH_TABLE_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define TABLE_SIZE 100

typedef struct node {
    char *key;
    struct node *next;
} hash_node_t;

unsigned int hash(const char *key) {
    unsigned long hash_val = 5381;
    int c;
    while ((c = *key++)) {
        hash_val = ((hash_val << 5) + hash_val) + c;
    }
    return hash_val % TABLE_SIZE;
}


void insert(hash_node_t **table, const char *key) {

    unsigned int idx = hash(key);
    hash_node_t *n = table[idx];
    while (n) {
        if (strcmp(n->key, key) == 0) {
            return;
        }
        n = n->next;
    }
    
    //add new node
    hash_node_t *new_node = (hash_node_t *)malloc(sizeof(hash_node_t));
    new_node->key = strdup(key);
    new_node->next = table[idx];
    table[idx] = new_node;
}

//check if key exists in set
bool contains(hash_node_t **table, const char *key) {
    unsigned int idx = hash(key);
    hash_node_t *n = table[idx];
    while (n) {
        if (strcmp(n->key, key) == 0) {
            return true;
        }
        n = n->next;
    }
    return false;
}

//remove key from set
void remove_key(hash_node_t **table, const char *key) {
    unsigned int idx = hash(key);
    hash_node_t *n = table[idx];
    hash_node_t *prev = NULL;
    
    while (n) {
        if (strcmp(n->key, key) == 0) {
            if (prev) {
                prev->next = n->next;
            } else {
                table[idx] = n->next;
            }
            free(n->key);
            free(n);
            return;
        }
        prev = n;
        n = n->next;
    }
}

//extract username from message format "username: message"
void extract_username(const char *message, char *username, int max_len) {
    const char *colon = strchr(message, ':');
    
    if (colon != NULL) {
        int len = colon - message;
        if (len > 0 && len < max_len) {
            strncpy(username, message, len);
            username[len] = '\0';
        } else {
            username[0] = '\0';
        }
    } else {
        username[0] = '\0';
    }
}

#endif