/*
 * variable data structures, for better memory management (tm)
 */
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <stdbool.h>
#include "markov.h"
#include "vbuf.h"

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
    DPA *dpa = malloc(sizeof(DPA));
    if (dpa == NULL) return NULL;
    dpa->keys = calloc(DPA_START_SIZE, sizeof(void *));
    if (dpa->keys == NULL) return NULL;
    dpa->used = 0;
    dpa->size = DPA_START_SIZE;
    return dpa;
}
/**
 * Removes obj from dpa, by swapping the last object of dpa->keys into the slot where
 * obj was, and setting that object to NULL whilst decrementing dpa->used.
 *
 * Does NOT make any attempt to free() obj, do this yourself. Returns false if object
 * does not exist.
 */
extern bool *DPA_rem(DPA *dpa, void *obj) {
    int i = 0, j = -1;
    for (void *tbr = dpa->keys[i]; i < dpa->used; tbr = dpa->keys[++i])
        if (tbr == obj) j = i;
    if (j == -1) return false;
    dpa->keys[j] = dpa->keys[i - 1];
    dpa->keys[i - 1] = NULL;
    dpa->used--;
    return true;
}
/**
 * Stores obj in dpa. Returns a pointer to obj if successful, or NULL if there was an error.
 */
extern void *DPA_store(DPA *dpa, void *obj) {
    if ((dpa->size - dpa->used) < 2) {
        // allocate more space
        void **ptr = realloc(dpa->keys, sizeof(void *) * (dpa->size + DPA_REFILL_SIZE));
        if (ptr == NULL) return NULL;
        dpa->keys = ptr;
        dpa->size += DPA_REFILL_SIZE;
    }
    (dpa->keys)[(dpa->used++)] = obj;
    return obj;
}

extern void DPA_free(DPA *dpa) {
    free(dpa->keys);
    free(dpa);
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
        wmemset(ptr + vs->size, L'\0', iu + VARSTR_REFILL_SIZE);
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
 *
 * Remember to free() the string!
 */
extern wchar_t *varstr_pack(struct varstr *vs) {
    wchar_t *ptr = malloc(sizeof(wchar_t) * (vs->used + 1));
    if (ptr == NULL) return NULL;
    wcscpy(ptr, (const wchar_t *) vs->str);
    free(vs->str);
    free(vs);
    return ptr;
};

extern void varstr_free(struct varstr *vs) {
    free(vs->str);
    free(vs);
};
