/*
 * save and load
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <wchar.h>
#include "markov.h"
#include "vbuf.h"

static const unsigned int NEWKEY = '\x11';
static const unsigned int NEWVAL = '\x12';
static const unsigned int NEWLINE = '\n';
static const char *filename = "markov_keys.mkdb";

int save(void) {
    FILE *fp;
    fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("save(): opening file");
        return 1;
    }
    struct kv_node *curnode;
    int i = 0;
    for (curnode = markov_database->objs->keys[i]; i < markov_database->objs->used; curnode = markov_database->objs->keys[++i]) {
        fwrite(&NEWKEY, sizeof(unsigned int), (size_t) 1, fp);
        fwrite(curnode->key, sizeof(wchar_t), (size_t) wcslen(curnode->key), fp);
        fwrite(&NEWVAL, sizeof(unsigned int), (size_t) 1, fp);
        fwrite(curnode->val, sizeof(wchar_t), (size_t) wcslen(curnode->val), fp);
        if (curnode->next != NULL) {
            struct kv_node *subnode = NULL;
            subnode = curnode->next;
            for (; subnode != NULL; subnode = subnode->next) {
                fwrite(&NEWVAL, sizeof(unsigned int), (size_t) 1, fp);
                fwrite(subnode->val, sizeof(wchar_t), (size_t) wcslen(subnode->val), fp);
            }
        }
        fwrite(&NEWLINE, sizeof(unsigned int), (size_t) 1, fp);
    }
    if (ferror(fp)) {
        perror("save(): writing data");
        return 1;
    }
    return 0;
}
/**
 * Load the database. Returns 0 on success, 1 on error, and 2 if you don't have one.
 */
int load(void) {
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return 2;
        }
        perror("load(): opening file");
        return 1;
    }
    struct varstr *key = varstr_init();
    struct varstr *val = varstr_init();
    int mode = 2; /* 0 = key, 1 = val, 2 = wtf? */
    if (key == NULL || val == NULL) {
        perror("load(): failed to initialise varstrs");
        return 1;
    }
    for (wchar_t c = fgetwc(fp); c != EOF; c = fgetwc(fp)) {
        if (c == '\0') continue;
        if (c == NEWKEY) {
            key = varstr_init();
            mode = 0;
        }
        else if (c == NEWVAL) {
            if (mode == 1) {
                wchar_t *k = varstr_pack(key);
                wchar_t *v = varstr_pack(val);
                if (k == NULL || v == NULL) {
                    perror("load(): failed to pack varstrs");
                    return 1;
                }
                //printf("loaded: %s = %s\n", k, v);
                store_kv(k, v);
            }
            val = varstr_init();
            mode = 1;
        }
        else if (c == NEWLINE) {
            wchar_t *k = varstr_pack(key);
            wchar_t *v = varstr_pack(val);
            if (k == NULL || v == NULL) {
                perror("load(): failed to pack varstrs");
                return 1;
            }
            //printf("loaded: %s = %s\n", k, v);
            store_kv(k, v);
            mode = 2;
        }
        else if (mode == 2) {
            errno = EINVAL;
            perror("load(): file corrupted");
        }
        else if (mode == 1) {
            varstr_pushc(val, c);
        }
        else if (mode == 0) {
            varstr_pushc(key, c);
        }
        else {
            fprintf(stderr, "EVERYTHING IS BROKEN - this should NEVER happen");
            return 999;
        }
    }
    return 0;
}

