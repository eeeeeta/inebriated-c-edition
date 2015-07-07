/*
 * crappy Markov chains chatbot
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "markov.h"
#include "vbuf.h"

/**
 * Search for a key. Returns a kv_node pointer or NULL on failure.
 */
struct kv_node *search_for_key(char *key) {
    struct kv_node *curnode;
    int i = 0;
    for (curnode = markov_database->keys[i]; curnode != NULL; curnode = markov_database->keys[++i]) {
        if (strcmp(key, curnode->key) == 0) {
            return curnode;
        }
    }
    return NULL;
}

struct vn_list *get_vals(char *key) {
    struct vn_list *vnl = vnlist_init();
    if (vnl == NULL) return NULL;
    struct kv_node *kv;
    kv = search_for_key(key);
    if (kv == NULL) return NULL;
    if (kv->next == NULL) {
        struct vn_node *vn;
        vn = (struct vn_node *) malloc(sizeof(struct vn_node));
        vn->next = search_for_key(kv->val);
        vn->val = kv->val;
        vnlist_add(vnl, vn);
        return vnl;
    }
    for (;kv != NULL; kv = kv->next) {
        struct vn_node *vn;
        vn = (struct vn_node *) malloc(sizeof(*vn));
        vn->next = search_for_key(kv->val);
        vn->val = kv->val;
        vnlist_add(vnl, vn);
    }
    return vnl;
}
struct kv_node *store_kv(char *key, char *val) {
    struct kv_node *kv;
    if (search_for_key(key) == NULL) {
        kv = malloc(sizeof(struct kv_node));
        if (kv == NULL) {
            return NULL;
        }
        kv->key = key;
        kv->val = val;
        kv->next = NULL;
        db_store(kv, markov_database);
        if (kv == NULL) {
            perror("store_kv()");
        }
        return kv;
    }
    else {
        kv = malloc(sizeof(struct kv_node));
        struct kv_node *last_kv;
        last_kv = search_for_key(key);
        for (;last_kv->next != NULL; last_kv = last_kv->next) {
            if (strcmp(val, last_kv->val)) {
                return last_kv;
            }
        }
        kv->key = key;
        kv->val = val;
        last_kv->next = kv;
        return kv;
    }
}

