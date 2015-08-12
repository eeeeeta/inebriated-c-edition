/*
 * save and load
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdbool.h>
#include <pthread.h>
#include "markov.h"

static const wchar_t NEWKEY = L'\x11';
static const int yes = 1;
static const wchar_t NEWVAL = L'\x12';
static const wchar_t SENTENCE_STARTER = L'\x13';
static const wchar_t NEWLINE = L'\n';
static pthread_mutex_t db_lock;
static bool pthreads_ready = false;

static void *save_tfunc(void *filename) {
    wprintf(L"save_tfunc(): database save queued, waiting for lock\n");
    pthread_mutex_lock(&db_lock);
    wprintf(L"save_tfunc(): saving database to '%s'\n", filename);
    FILE *fp;
    fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("save_tfunc(): opening file");
        pthread_mutex_unlock(&db_lock);
        return NULL;
    }
    DBN *curnode;
    int i = 0;
    for (curnode = markov_database->objs->keys[i]; i < markov_database->objs->used; curnode = markov_database->objs->keys[++i]) {
        if (curnode->vptr == NULL) continue;
        fputwc(NEWKEY, fp);
        if (DBN_getss(curnode->key) == curnode) {
            fputwc(SENTENCE_STARTER, fp);
        }
        fputws(curnode->key, fp);
        fputwc(NEWVAL, fp);
        fputws(curnode->vptr->key, fp);
        if (curnode->next != NULL) {
            DBN *subnode = NULL;
            subnode = curnode->next;
            for (; subnode != NULL; subnode = subnode->next) {
                if (subnode->vptr == NULL) continue;
                fputwc(NEWVAL, fp);
                fputws(subnode->vptr->key, fp);
            }
        }
        fputwc(NEWLINE, fp);
    }
    fflush(fp);
    if (ferror(fp)) {
        perror("save_tfunc(): writing data");
        pthread_mutex_unlock(&db_lock);
        return NULL;
    }
    wprintf(L"save_tfunc(): database save successful\n");
    pthread_mutex_unlock(&db_lock);
    return (int *) &yes;
}
extern bool save(char *filename) {
    if (!pthreads_ready) {
        if (pthread_mutex_init(&db_lock, NULL) != 0) {
            perror("save(): pthread_mutex_init failed");
            return false;
        }
        pthreads_ready = true;
    }
    pthread_t thread;
    void *retval;
    if ((errno = pthread_create(&thread, NULL, &save_tfunc, filename)) != 0) {
        perror("save(): pthread_create failed");
        return false;
    }
    if ((errno = pthread_join(thread, &retval)) != 0) {
        perror("save(): pthread_join failed");
        return false;
    }
    if (retval != &yes) {
        fwprintf(stderr, L"save(): thread did not return success\n");
        return false;
    }
    return true;
}
/**
 * Read one key.
 * Returns 0 if not starter, 1 if starter, -1 if internal error, -2 if corrupt, -3 on EOF
 */
static signed int rkey(FILE *fp, struct varstr *into) {
    int starter = 0;
    bool got_start_indic = false;
    wchar_t c = '\0';
    while ((c = fgetwc(fp)) != WEOF) {
        if (!got_start_indic && c == NEWKEY) {
            got_start_indic = true;
            continue;
        }
        else if (!got_start_indic) return -2; /* one of the most inelegant solutions ever! */
        if (c == SENTENCE_STARTER) {
            starter = 1;
            continue;
        }
        if (c == NEWKEY) return -2;
        if (c == NEWVAL) {
            ungetwc(NEWVAL, fp);
            return starter;
        }
        if (c == '\0') return -2;
        if (c == NEWLINE) return -2;
        if (varstr_pushc(into, c) == NULL) return -1;
    };
    return -3;
}
/**
 * Read one value.
 * Returns 0 on success, 1 if there is another value, -1 if internal error, -2 if corrupt, -3 on EOF
 */
static signed int rval(FILE *fp, struct varstr *into) {
    wchar_t c = '\0';
    bool got_start_indic = false;
    while ((c = fgetwc(fp)) != WEOF) {
        if (!got_start_indic && c == NEWVAL) {
            got_start_indic = true;
            continue;
        }
        else if (!got_start_indic) return -2;
        if (c == SENTENCE_STARTER) return -2;
        if (c == NEWKEY) return -2;
        if (c == NEWVAL) {
            ungetwc(NEWVAL, fp);
            return 1;
        }
        if (c == '\0') return -2;
        if (c == NEWLINE) return 0;
        if (varstr_pushc(into, c) == NULL) return -1;
    };
    return -3;
}/**
  * Load the database. Returns 0 on success, 1 on error, and 2 if you don't have one.
  */
extern int load(char *filename) {
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return 2;
        }
        perror("load(): opening file");
        return 1;
    }
    struct varstr *key = NULL, *val = NULL;
    wchar_t *k = NULL, *v = NULL;
    signed int retval = -1;
    bool valonly = false;
    signed int is_ss = -1;
    do {
        key = varstr_init();
        val = varstr_init();
        if (key == NULL || val == NULL) {
            perror("load(): varstr_init failed");
            return 1;
        }
        valonly = false;
        is_ss = rkey(fp, key);
newval: if (is_ss >= 0) retval = rval(fp, val);
        if (is_ss < 0 || retval < 0) {
            if (is_ss < 0) retval = is_ss;
            if (retval == -2) {
                fwprintf(stderr, L"load(): database corrupted\n");
                return 1;
            }
            if (retval == -3) {
                if (ferror(fp)) {
                    perror("load(): error reading file");
                    return 1;
                }
                if (is_ss < 0) break;
                if ((k = varstr_pack(key)) == NULL || (v = varstr_pack(val)) == NULL) {
                    perror("load(): varstr_pack failed");
                    return 1;
                }
                key = val = NULL;
                store_kv(k, v, is_ss);
                break;
            }
            perror("load(): internal error");
            return 1;
        }
        if ((!valonly && (k = varstr_pack(key)) == NULL) || (v = varstr_pack(val)) == NULL) {
            perror("load(): varstr_pack failed");
            return 1;
        }
        key = val = NULL;
        store_kv(k, v, (is_ss == 1 ? true : false));
        if (retval == 1) {
            if ((val = varstr_init()) == NULL) {
                perror("load(): varstr_init failed");
                return 1;
            }
            valonly = true;
            goto newval; /* http://xkcd.com/292/ */
        }
    } while (true);
    if (key != NULL) varstr_free(key);
    if (val != NULL) varstr_free(val);
    return 0;
}

