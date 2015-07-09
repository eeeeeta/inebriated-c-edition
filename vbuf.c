/*
 * variable data structures, for better memory management (tm)
 */
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include "markov.h"
#include "vbuf.h"

extern struct database *db_init(void) {
    struct database *db = malloc(sizeof(struct database));
    db->objs = kvl_init();
    db->sses = kvl_init();
    return db;
}
/**
 * Initialises a kv_list object.
 * Returns a pointer to said object, or NULL if there was an error.
 */
extern struct kv_list *kvl_init(void) {
    struct kv_list *db = malloc(sizeof(struct kv_list));
    if (db == NULL) return NULL;
    db->keys = calloc(DB_START_SIZE, sizeof(struct kv_node *));
    if (db->keys == NULL) return NULL;
    db->used = 0;
    db->size = DB_START_SIZE;
    return db;
}
/**
 * Stores obj in kvl. Returns a pointer to obj if successful, or NULL if there was an error.
 */
extern struct kv_node *kvl_store(struct kv_node *obj, struct kv_list *db) {
    if ((db->size - db->used) <= 1) {
        // allocate more space
        struct kv_node **ptr = realloc(db->keys, sizeof(struct kv_node *) * (db->size + DB_REFILL_SIZE));
        if (ptr == NULL) return NULL;
        memset((ptr + db->size), 0, DB_REFILL_SIZE);
        db->keys = ptr;
        db->size += DB_REFILL_SIZE;
    }
    (db->keys)[(db->used)++] = obj;
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
static struct varstr *varstr_refill_if_needed(struct varstr *vs, size_t iu) {
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

/**
 * Initialises a variable list of vn_node objects.
 * Returns NULL on failure.
 */
extern struct vn_list *vnlist_init(void) {
    struct vn_list *vnl = malloc(sizeof(struct vn_list));
    if (vnl == NULL) return NULL;
    vnl->list = calloc(VNLIST_START_SIZE, sizeof(struct vn_node));
    if (vnl->list == NULL) return NULL;
    vnl->used = 0;
    vnl->size = VNLIST_START_SIZE;
    return vnl;
}
/**
 * Add a vn_node to a vn_list, allocating more space if needed.
 * Returns NULL on failure.
 */
extern struct vn_list *vnlist_add(struct vn_list *vnl, struct vn_node *vn) {
    if ((vnl->size - vnl->used) <= 1) {
        struct vn_node **ptr = realloc(vnl->list, sizeof(struct vn_node *) * (vnl->size + VNLIST_REFILL_SIZE));
        if (ptr == NULL) return NULL;
        vnl->size += VNLIST_REFILL_SIZE;
        vnl->list = ptr;
    }
    (vnl->list)[(vnl->used)++] = vn;
    return vnl;
}
