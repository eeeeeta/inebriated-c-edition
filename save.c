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
    struct kv_node *curnode;
    int i = 0;
    for (curnode = markov_database->objs->keys[i]; i < markov_database->objs->used; curnode = markov_database->objs->keys[++i]) {
        fputwc(NEWKEY, fp);
        if (search_for_ss(curnode->key) == curnode) {
            fputwc(SENTENCE_STARTER, fp);
        }
        fputws(curnode->key, fp);
        fputwc(NEWVAL, fp);
        fputws(curnode->val, fp);
        if (curnode->next != NULL) {
            struct kv_node *subnode = NULL;
            subnode = curnode->next;
            for (; subnode != NULL; subnode = subnode->next) {
                fputwc(NEWVAL, fp);
                fputws(subnode->val, fp);
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
    struct varstr *key = NULL;
    wchar_t *lk = NULL;
    struct varstr *val = NULL;
    int sentence_starter = 1;
    int mode = 2; /* 0 = key, 1 = val, 2 = wtf? */
    for (wchar_t c = fgetwc(fp); c != EOF; c = fgetwc(fp)) {
        if (c == L'\0') {
            fwprintf(stderr, L"load(): Error: Your database has encoding issues. (this shouldn't happen)\n");
            return 1;
        }
        if (c == NEWKEY) {
            key = varstr_init();
            if (key == NULL) {
                perror("load(): varstr_init failed");
                return 1;
            }
            mode = 0;
            sentence_starter = 1;
        }
        else if (c == NEWVAL) {
            if (mode == 1) {
                wchar_t *k = NULL;
                if (key == NULL) k = lk;
                else k = lk = varstr_pack(key);
                wchar_t *v = varstr_pack(val);
                if (k == NULL || v == NULL) {
                    perror("load(): failed to pack varstrs");
                    return 1;
                }
                store_kv(k, v, sentence_starter);
                val = NULL;
            }
            val = varstr_init();
            if (val == NULL) {
                perror("load(): varstr_init failed");
                return 1;
            }
            mode = 1;
        }
        else if (c == NEWLINE) {
            wchar_t *k = NULL;
            if (lk == NULL) k = varstr_pack(key);
            else k = lk;
            wchar_t *v = varstr_pack(val);
            lk = NULL;
            if (k == NULL || v == NULL) {
                perror("load(): failed to pack varstrs");
                return 1;
            }
            key = NULL;
            val = NULL;
            store_kv(k, v, sentence_starter);
            mode = 2;
        }
        else if (c == SENTENCE_STARTER && mode == 0) {
            sentence_starter = 0;
        }
        else if (mode == 1 && key != NULL) {
            varstr_pushc(val, c);
        }
        else if (mode == 0 && key != NULL) {
            varstr_pushc(key, c);
        }
        else {
            errno = EINVAL;
            perror("load(): file corrupted");
            return 1;
        }
    }
    return 0;
}

