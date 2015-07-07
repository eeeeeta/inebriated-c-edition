/*
 * variable data structures, for better memory management (tm)
 */
#include <stdlib.h>
#include <string.h>
#include "markov.h"
#include "vbuf.h"
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

/**
 * Initialises a variable string object. Returns pointer on success, NULL on failure.
 */
extern struct varstr *varstr_init(void) {
    struct varstr *vs = malloc(sizeof(struct varstr));
    if (vs == NULL) return NULL;
    vs->str = calloc(VARSTR_START_SIZE, sizeof(char));
    if (vs->str == NULL) return NULL;
    vs->used = 0;
    vs->size = VARSTR_START_SIZE;
    return vs;
}
/**
 * (internal function) Refill a varstr if space left is less than/equal to iu.
 */
static struct varstr *varstr_refill_if_needed(struct varstr *vs, int iu) {
    if ((vs->size - vs->used) <= iu) {
        char *ptr = realloc(vs->str, (vs->size + VARSTR_REFILL_SIZE));
        if (ptr == NULL) return NULL;
        vs->str = ptr;
        vs->size += VARSTR_REFILL_SIZE;
    }
    return vs;
}
/**
 * Appends a to b using strcat(), allocating more space if needed. Returns pointer to varstr object on success, NULL on failure.
 */
extern struct varstr *varstr_cat(struct varstr *vs, char *str) {
    vs = varstr_refill_if_needed(vs, (strlen(str) + 1));
    if (vs == NULL) return NULL;
    vs->used += (strlen(str) + 1);
    strcat(vs->str, str);
    return vs;
}
/**
 * Appends a to b using strncat(), allocating more space if needed. Returns pointer to varstr object on success, NULL on failure.
 */
extern struct varstr *varstr_ncat(struct varstr *vs, char *str, size_t count) {
    vs = varstr_refill_if_needed(vs, count + 1);
    if (vs == NULL) return NULL;
    vs->used += (count + 1);
    strncat(vs->str, str, count);
    return vs;
}
/**
 * Append a single char a to b, allocating more space if needed. Returns pointer to varstr object on success, NULL on failure.
 */
extern struct varstr *varstr_pushc(struct varstr *vs, char c) {
    vs = varstr_refill_if_needed(vs, 2);
    if (vs == NULL) return NULL;
    (vs->str)[(vs->used)++] = c;
    return vs;

}
/**
 * Free unused memory in a variable string & convert it to just a regular string.
 * Returns pointer to regular string, NULL on failure.
 */
extern char *varstr_pack(struct varstr *vs) {
    char *ptr = realloc(vs->str, (vs->used + 1));
    if (ptr == NULL) return NULL;
    //free(vs);
    return ptr;
};
/**
 * Obliterate a varstr and its contents off the face of the earth! >:)
 * (free memory used by a varstr)
 */
extern void varstr_obliterate(struct varstr *vs) {
    free(vs->str);
    free(vs);
};

