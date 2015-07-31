/*
 * crappy Markov chains chatbot
 */
#include <stdio.h>
#include <wchar.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "markov.h"

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

DPA *get_vals(struct kv_node *kv) {
    DPA *vnl = DPA_init();
    if (vnl == NULL) return NULL;
    if (kv->next == NULL) {
        struct vn_node *vn;
        vn = malloc(sizeof(struct vn_node));
        vn->next = search_for_key(kv->val);
        vn->val = kv->val;
        if (vn->next == NULL) vn->score = 0;
        else vn->score = vn->next->score;
        DPA_store(vnl, vn);
        return vnl;
    }
    for (;kv != NULL; kv = kv->next) {
        struct vn_node *vn;
        vn = malloc(sizeof(struct vn_node));
        vn->next = search_for_key(kv->val);
        vn->val = kv->val;
        if (vn->next == NULL) vn->score = 0;
        else vn->score = vn->next->score;
        DPA_store(vnl, vn);
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
static float determine_dscore(struct kv_node *kv) {
    float dscore = wcslen(kv->key);
    for (DPA *vnl = get_vals(kv); vnl != NULL; vnl = get_vals(kv)) {
        struct vn_node *vn = vnl->keys[rand_lim(vnl->used - 1)];
        dscore += wcslen(vn->val) - 1.2;
        dscore += kv->score - 2;
        if (vn->next == NULL) break;
        kv = vn->next;
    }
    return dscore / 10;
}
/**
 * Poorly implemented random start object picker.
 *
 * Chooses from a DPA with objects of type kv_node.
 * Is weighted toward objects with a higher score value, as those produce
 * the best results.
 */
static void *get_rand_start_kv(DPA *dpa, int mode) {
    if (mode != 0 && mode != 1) {
        errno = EINVAL;
        return NULL;
    }
    if (dpa->used < 1) {
        errno = ENXIO;
        return NULL;
    }
    DPA *desirable = DPA_init();
    if (desirable == NULL) return NULL;
    for (int mscore = (3 + rand_lim(5)); mscore > -1; mscore--) {
        int i = 0;
            for (struct kv_node *kv = dpa->keys[i++]; i < dpa->used; kv = dpa->keys[i++]) {
                float dscore = determine_dscore(kv);
                if (dscore >= mscore) {
                    DPA_store(desirable, kv);
                }
            }
        if (desirable->used > 0 && rand_lim(2) == 0) break; /* 1/3 chance of not including the crappier choices at all */
        if (mscore == 1 && desirable->used > 0) break; /* don't go to mscore = 0 (enders) unless we /have/ to */
    }
    if (desirable->used < 1) return NULL;
    void *chosen = desirable->keys[rand_lim(desirable->used - 1)];
    if (chosen == NULL) return NULL;
    free(desirable);
    return chosen;
}

extern wchar_t *generate_sentence() {
    struct varstr *sentence = varstr_init();
    if (sentence == NULL) return NULL;
    if (markov_database->sses->used < 1) {
        errno = ENXIO;
        return NULL;
    }
    struct kv_node *kv = get_rand_start_kv(markov_database->sses, 0);
    if (kv == NULL) {
        errno = EFAULT;
        return NULL;
    }
    varstr_cat(sentence, kv->key);
    DPA *vnl = get_vals(kv);
    for (struct vn_node *vn; vnl != NULL; vnl = get_vals(vn->next)) {
        vn = vnl->keys[rand_lim(vnl->used - 1)];
        if (vn == NULL) break;
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
        kv->score = 1;
        kv = DPA_store(markov_database->objs, kv);
        if (ss == 0) kv = DPA_store(markov_database->sses, kv);
        if (kv == NULL) {
            perror("store_kv()");
        }
        return kv;
    }
    else {
        kv = malloc(sizeof(struct kv_node));
        struct kv_node *origin = search_for_key(key);
        struct kv_node *last_kv = origin;
        int score = 2;
        for (struct kv_node *curnode = origin->next; curnode != NULL; curnode = curnode->next) {
            score++;
            last_kv = curnode;
        }
        kv->key = key;
        kv->val = val;
        kv->next = NULL;
        kv->score = 0;
        origin->score = score;
        last_kv->next = kv;
        return kv;
    }
}

