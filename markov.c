/*
 * crappy Markov chains chatbot
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "markov.h"

#define MAXLEN 130 /* maximum length of two word pairs */
#define DBSIZE 100
#define MAXVALS 100

static struct kv_node *keys[DBSIZE];

struct kv_node *search_for_key(char *key) {
    static struct kv_node *curnode;
    int i = 0;
    for (curnode = keys[i]; curnode != NULL; curnode = keys[++i]) {
        if (strcmp(key, curnode->key) == 0) {
            return curnode;
        }
    }
    return NULL;
}
int get_vals(struct vn_node *into[], char *key) {
    static struct kv_node *kv;
    kv = search_for_key(key);
    int valc = 0;
    if (kv == NULL) {
        return 0;
    }
    if (kv->next == NULL) {
        static struct vn_node *vn;
        vn = (struct vn_node *) malloc(sizeof(*vn));
        vn->next = search_for_key(kv->val);
        vn->val = kv->val;
        into[0] = vn;
        return 1;
    }
    int i = 0;
    for (;kv != NULL; kv = kv->next) {
        static struct vn_node *vn;
        vn = (struct vn_node *) malloc(sizeof(*vn));
        vn->next = search_for_key(kv->val);
        vn->val = kv->val;
        into[i++] = vn;
    }
    return i;
}
void dbg_walk(char *key) {
    static struct kv_node *kv;
    printf("dbg_walk: walking %s\n", key);
    kv = search_for_key(key);
    if (kv == NULL) {
        printf("dbg_walk: no results for '%s'\n", key);
        return;
    }
    static struct vn_node *vals[MAXVALS];
    static int results_retd = 0;
    results_retd = get_vals(vals, key);
    printf("dbg_walk: %d results for '%s'\n", results_retd, key);
    for (int i = 1; i <= results_retd; i++) {
        printf("dbg_walk: walking val %d of %s\n", (i-1), key);
        static struct vn_node *val;
        val = vals[(i-1)];
        if (val == NULL) {
            printf("dbg_walk: EVERYTHING IS BROKEN - if you are a user, godspeed\n");
            exit(101);
        }
        printf("dbg_walk: val %d: %s\n", (i-1), val->val);
        if (val->next == NULL) {
            printf("dbg_walk: branch %d of %s ends here\n", (i-1), key);
        }
        else {
            printf("dbg_walk: continuing branch %d of %s ('%s')\n", (i-1), key, val->val);
            dbg_walk(val->val);
        }
    }
}
int get_ins_pos() {
    struct kv_node *curnode;
    int i = 0;
    for (curnode = keys[i]; curnode != NULL; curnode = keys[++i])
        ;
    return i;
}
struct kv_node *store_kv(char *key, char *val) {
    static struct kv_node *kv;
    if (search_for_key(key) == NULL) {
        kv = (struct kv_node *) malloc(sizeof(*kv));
        if (kv == NULL) {
            return NULL;
        }
        kv->key = key;
        kv->val = val;
        keys[get_ins_pos()] = kv;
        return kv;
    }
    else {
        kv = (struct kv_node *) malloc(sizeof(*kv));
        static struct kv_node *last_kv;
        last_kv = search_for_key(key);
        for (;last_kv->next != NULL; last_kv = last_kv->next)
            ;
        kv->key = key;
        kv->val = val;
        last_kv->next = kv;
        return kv;
    }
}

