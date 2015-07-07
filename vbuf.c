/*
 * variable data structures, for better memory management (tm)
 */
#include <stdlib.h>
#include "markov.h"

/**
 * Initialises a database object.
 * Returns a pointer to said object, or NULL if there was an error.
 */
extern Database *db_init(void) {
    Database *db = malloc(sizeof(Database));
    if (db == NULL) return NULL;
    db->keys = calloc(DB_START_SIZE, sizeof(struct kv_node));
    if (db->keys == NULL) return NULL;
    db->used = 0;
    db->size = DB_START_SIZE;
    return db;
}
/**
 * Stores obj in db. Returns a pointer to obj if successful, or NULL if there was an error.
 */
extern struct kv_node *db_store(struct kv_node *obj, Database *db) {
    if ((db->size - db->used) <= 1) {
        // allocate more space
        struct kv_node **ptr = realloc(db->keys, (db->size + DB_REFILL_SIZE));
        if (ptr == NULL) return NULL;
        db->keys = ptr;
        db->size += DB_REFILL_SIZE;
        (db->keys)[(db->used)++] = obj;
        return obj;
    }
    else {
        (db->keys)[(db->used)++] = obj;
        return obj;
    }
}

