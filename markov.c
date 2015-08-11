/*
 * crappy Markov chains chatbot
 */
#include <stdio.h>
#include <wchar.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include "markov.h"

/**
 * hash algo for strings
 */
static unsigned long djb_hash(void *key, int len) {
    unsigned char *p = key;
    unsigned long h = 0;
    int i;
    for (i = 0; i < len; i++) {
        h = 33 * h ^ p[i];
    }
    return h;
}
static unsigned long hash(wchar_t *key) {
    return djb_hash(key, wcslen(key) * sizeof(wchar_t));
}
/**
 * Search for key in dpa. Returns a DBN pointer or NULL on failure.
 */
extern DBN *DBN_search(wchar_t *key, DPA *dpa) {
    DBN *curnode = NULL;
    int i = 0;
    unsigned long hsh = hash(key);
    for (curnode = dpa->keys[i]; i < dpa->used; curnode = dpa->keys[++i]) {
        if (hsh == *(curnode->hash)) {
            return curnode;
        }
    }
    return NULL;
}

extern DBN *DBN_getk(wchar_t *key) {
    return DBN_search(key, markov_database->objs);
}
extern DBN *DBN_getss(wchar_t *key) {
    return DBN_search(key, markov_database->sses);
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
/**
 * Get item n in linked list with origin node dbn.
 */
static DBN *lln(int n, DBN *dbn) {
    register int c = 0;
    while (c++ != n) {
        assert(dbn->next != NULL);
        dbn = dbn->next;
    }
    return dbn;
};
static float determine_dscore(DBN *kv) {
    float dscore = 0;
    for (kv = lln(rand_lim(kv->score), kv); kv != NULL; kv = lln(rand_lim(kv->score), kv)) {
        dscore += wcslen(kv->key) - 1.2;
        if ((kv = kv->vptr) == NULL) break;
    }
    return dscore / 10;
}
/**
 * Poorly implemented random start object picker.
 *
 * Chooses from a DPA with objects of type DBN.
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
        for (DBN *kv = dpa->keys[i]; i < dpa->used; kv = dpa->keys[i++]) {
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
    DPA_free(desirable);
    return chosen;
}

extern wchar_t *generate_sentence() {
    struct varstr *sentence = varstr_init();
    if (sentence == NULL) return NULL;
    if (markov_database->sses->used < 1) {
        errno = ENXIO;
        return NULL;
    }
    DBN *kv = get_rand_start_kv(markov_database->sses, 0);
    if (kv == NULL) {
        errno = EFAULT;
        return NULL;
    }
    bool first = true;
    for (kv = lln(rand_lim(kv->score), kv); kv != NULL; kv = lln(rand_lim(kv->score), kv)) {
        if (!first) varstr_cat(sentence, L" ");
        else first = false;
        varstr_cat(sentence, kv->key);
        if ((kv = kv->vptr) == NULL) break;
    }
    wchar_t *str;
    if ((str = varstr_pack(sentence)) == NULL) return NULL;
    return str;
}
/**
 * Store a database node (key -> vptr). ss = true if sentence starter.
 *
 * Returns pointer to origin DBN (in database) on success, NULL on failure.
 */
extern DBN *DBN_store(wchar_t *key, DBN *vptr, bool ss) {
    DBN *kv = NULL;
    unsigned long *hptr = NULL;
    if ((hptr = malloc(sizeof(unsigned long))) == NULL) return NULL;
    if (vptr != NULL) assert(vptr->score != -1); /* vptr should ALWAYS point to an origin node */
    if (DBN_getk(key) == NULL) {
        /* key does not exist */
        kv = malloc(sizeof(DBN));
        if (kv == NULL) return NULL;
        kv->key = key;
        kv->vptr = vptr;
        kv->next = NULL;
        *hptr = hash(key);
        kv->hash = hptr;
        kv = DPA_store(markov_database->objs, kv);
        kv->score = 0;
        if (ss) kv = DPA_store(markov_database->sses, kv);
        if (kv == NULL) {
            perror("DBN_store()");
        }
        return kv;
    }
    else {
#ifdef I_HAVE_A_VERY_BIG_NODE
        int16_t score = 0;
#else
        int8_t score = 0;
#endif
        kv = malloc(sizeof(DBN));
        DBN *orig = DBN_getk(key);
        DBN *lkv = orig;
        for (DBN *curnode = lkv->next; curnode != NULL; curnode = curnode->next) {
            if (curnode->vptr == vptr) return curnode;
            lkv = curnode;
#ifndef I_HAVE_A_VERY_BIG_NODE
            /* overflow detection */
            if (score == 127) {
                fwprintf(stderr, L"You have a very big node in your database. Recompile with -DI_HAVE_A_VERY_BIG_NODE.\n");
                assert(score < 127);
            }
#endif
            score++;
        }
        kv->key = orig->key;
        kv->vptr = vptr;
        kv->next = NULL;
        lkv->next = kv;
        kv->hash = orig->hash;
        orig->score = score;
        kv->score = -1;
        if (ss) orig = DPA_store(markov_database->sses, orig);
        return orig;
    }
}
/**
 * Store one key-value pair in the database.
 *
 * Returns NULL on failure, or pointer to DBN of key on success.
 */
extern DBN *store_kv(wchar_t *k, wchar_t *v, bool ss) {
    DBN *kdbn = NULL;
    DBN *vdbn = NULL;
    if ((vdbn = DBN_getk(v)) == NULL) vdbn = DBN_store(v, NULL, false); /* store the value as a DBN */
    if (vdbn == NULL) return NULL;
    if ((kdbn = DBN_getk(k)) != NULL && kdbn->vptr == NULL) {
        if (vdbn == kdbn) {
            /* yay, recursion is fun
             * do nothing for now, because cba */
            return kdbn;
        }
        kdbn->vptr = vdbn;
        if (ss && DBN_getss(k) == NULL) DPA_store(markov_database->sses, kdbn);
        return kdbn;
    }
    return DBN_store(k, vdbn, ss);
}
/**
 * Free one DB node.
 *
 * WARNING: Make sure to update the linked list that this node is part of!
 */
extern void DBN_free(DBN *dbn) {
    if (dbn->score != -1) { /* is origin */
        free(dbn->key);
        free(dbn->hash);
    }
    free(dbn);
}
/**
 * Free an entire linked list of DB nodes, starting at origin (dbn).
 */
extern void DBN_free_list(DBN *dbn) {
    if (dbn->next != NULL) DBN_free_list(dbn->next);
    DBN_free(dbn);
}
