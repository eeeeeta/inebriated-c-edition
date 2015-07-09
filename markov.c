/*
 * crappy Markov chains chatbot
 */
#include <stdio.h>
#include <wchar.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include "markov.h"
#include "vbuf.h"

/**
 * Search for a key. Returns a kv_node pointer or NULL on failure.
 */
struct kv_node *search_for_key(wchar_t *key) {
    struct kv_node *curnode;
    int i = 0;
    for (curnode = markov_database->objs->keys[i]; i < markov_database->objs->used; curnode = markov_database->objs->keys[++i]) {
        if (wcscmp(key, curnode->key) == 0) {
            return curnode;
        }
    }
    return NULL;
}

/**
 * Search for a sentence starter. Returns a kv_node pointer or NULL on failure.
 */
struct kv_node *search_for_ss(wchar_t *key) {
    struct kv_node *curnode;
    int i = 0;
    for (curnode = markov_database->sses->keys[i]; i < markov_database->sses->used; curnode = markov_database->sses->keys[++i]) {
        if (wcscmp(key, curnode->key) == 0) {
            return curnode;
        }
    }
    return NULL;
}

struct vn_list *get_vals(struct kv_node *kv) {
    struct vn_list *vnl = vnlist_init();
    if (vnl == NULL) return NULL;
    if (kv->next == NULL) {
        struct vn_node *vn;
        vn = malloc(sizeof(struct vn_node));
        vn->next = search_for_key(kv->val);
        vn->val = kv->val;
        vnlist_add(vnl, vn);
        return vnl;
    }
    for (;kv != NULL; kv = kv->next) {
        struct vn_node *vn;
        vn = malloc(sizeof(struct vn_node));
        vn->next = search_for_key(kv->val);
        vn->val = kv->val;
        vnlist_add(vnl, vn);
    }
    return vnl;
}
/**
 * Generate a random number between 0 and limit inclusive.
 */
static int rand_lim(int limit) {
    int divisor = RAND_MAX / (limit+1);
    int retval;
    do {
        retval = rand() / divisor;
    } while (retval > limit);
    return retval;
}

extern wchar_t *generate_sentence() {
    struct varstr *sentence = varstr_init();
    if (sentence == NULL) return NULL;
    if (markov_database->sses->used < 2) {
        errno = ENXIO;
        return NULL;
    }
    struct kv_node *kv = markov_database->sses->keys[rand_lim(markov_database->sses->used - 1)];
    if (kv == NULL) {
        errno = EFAULT;
        return NULL;
    }
    varstr_cat(sentence, kv->key);
    struct vn_list *vnl = get_vals(kv);
    for (struct vn_node *vn; vnl != NULL; vnl = get_vals(vn->next)) {
        vn = vnl->list[rand_lim(vnl->used - 1)];
        varstr_cat(sentence, L" ");
        varstr_cat(sentence, vn->val);
        if (vn->next == NULL) break;
    }
    wchar_t *str;
    if ((str = varstr_pack(sentence)) == NULL) return NULL;
    return str;
}
/**
 * Store a key-value pair, key = val. (ss = 0 if sentence-starter, 1 if not)
 * Returns a pointer to the kv_node in the database or NULL on failure.
 */
extern struct kv_node *store_kv(wchar_t *key, wchar_t *val, int ss) {
    struct kv_node *kv = NULL;
    if (search_for_key(key) == NULL) {
        kv = malloc(sizeof(struct kv_node));
        if (kv == NULL) return NULL;
        kv->key = key;
        kv->val = val;
        kv->next = NULL;
        kv = kvl_store(kv, markov_database->objs);
        if (ss == 0) kv = kvl_store(kv, markov_database->sses);
        if (kv == NULL) {
            perror("store_kv()");
        }
        return kv;
    }
    else {
        kv = malloc(sizeof(struct kv_node));
        struct kv_node *last_kv = NULL;
        last_kv = search_for_key(key);
        for (;last_kv->next != NULL; last_kv = last_kv->next) {
            if (wcscmp(val, last_kv->val)) {
                return last_kv;
            }
        }
        kv->key = key;
        kv->val = val;
        kv->next = NULL;
        last_kv->next = kv;
        return kv;
    }
}

