/*
 * variable data structures, for better memory management (tm)
 */
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include "markov.h"

extern struct database *db_init(void) {
    struct database *db = malloc(sizeof(struct database));
    db->objs = DPA_init();
    db->sses = DPA_init();
    return db;
}
/**
 * Initialises a dynamic pointer array object.
 * Returns a pointer to said object, or NULL if there was an error.
 */
extern DPA *DPA_init(void) {
    DPA *dpa = malloc(sizeof(dpa));
    if (dpa == NULL) return NULL;
    dpa->keys = calloc(DPA_START_SIZE, sizeof(void *));
    if (dpa->keys == NULL) return NULL;
    dpa->used = 0;
    dpa->size = DPA_START_SIZE;
    return dpa;
}
/**
 * Stores obj in dpa. Returns a pointer to obj if successful, or NULL if there was an error.
 */
extern void *DPA_store(DPA *dpa, void *obj) {
    if ((dpa->size - dpa->used) <= 1) {
        // allocate more space
        void **ptr = realloc(dpa->keys, sizeof(void *) * (dpa->size + DPA_REFILL_SIZE));
        if (ptr == NULL) return NULL;
        dpa->keys = ptr;
        dpa->size += DPA_REFILL_SIZE;
    }
    (dpa->keys)[(dpa->used)++] = obj;
    return obj;
}

/**
 * Initialises a variable string object. Returns pointer on success, NULL on failure.
 */
extern struct varstr *varstr_init(void) {
    struct varstr *vs = malloc(sizeof(struct varstr));
    if (vs == NULL) return NULL;
    vs->str = calloc(VARSTR_START_SIZE, sizeof(wchar_t));
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
        wchar_t *ptr = realloc(vs->str, sizeof(wchar_t) * (vs->size + iu + VARSTR_REFILL_SIZE));
        if (ptr == NULL) return NULL;
        vs->str = ptr;
        vs->size += iu;
        vs->size += VARSTR_REFILL_SIZE;
    }
    return vs;
}
/**
 * Appends a to b using strcat(), allocating more space if needed. Returns pointer to varstr object on success, NULL on failure.
 */
extern struct varstr *varstr_cat(struct varstr *vs, wchar_t *str) {
    vs = varstr_refill_if_needed(vs, (wcslen(str) + 1));
    if (vs == NULL) return NULL;
    vs->used += (wcslen(str) + 1);
    wcscat(vs->str, str);
    return vs;
}
/**
 * Appends a to b using strncat(), allocating more space if needed. Returns pointer to varstr object on success, NULL on failure.
 */
extern struct varstr *varstr_ncat(struct varstr *vs, wchar_t *str, size_t count) {
    vs = varstr_refill_if_needed(vs, count + 1);
    if (vs == NULL) return NULL;
    vs->used += (count + 1);
    wcsncat(vs->str, str, count);
    return vs;
}
/**
 * Append a single wchar_t a to b, allocating more space if needed. Returns pointer to varstr object on success, NULL on failure.
 */
extern struct varstr *varstr_pushc(struct varstr *vs, wchar_t c) {
    vs = varstr_refill_if_needed(vs, 2);
    if (vs == NULL) return NULL;
    (vs->str)[(vs->used)++] = c;
    return vs;

}
/**
 * Free unused memory in a variable string & convert it to just a regular string.
 * Returns pointer to regular string, NULL on failure.
 */
extern wchar_t *varstr_pack(struct varstr *vs) {
    wchar_t *ptr = realloc(vs->str, sizeof(wchar_t) * (vs->used + 1));
    if (ptr == NULL) return NULL;
    //free(vs);
    return ptr;
};
