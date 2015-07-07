/*
 * save and load
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "markov.h"

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
    for (curnode = keys[i]; curnode != NULL; curnode = keys[++i]) {
        fwrite(&NEWKEY, sizeof(unsigned int), (size_t) 1, fp);
        fwrite(curnode->key, sizeof(char), (size_t) strlen(curnode->key), fp);
        fwrite(&NEWVAL, sizeof(unsigned int), (size_t) 1, fp);
        fwrite(curnode->val, sizeof(char), (size_t) strlen(curnode->val), fp);
        if (curnode->next != NULL) {
            struct kv_node *subnode;
            subnode = curnode->next;
            for (; subnode != NULL; subnode = subnode->next) {
                fwrite(&NEWVAL, sizeof(unsigned int), (size_t) 1, fp);
                fwrite(subnode->val, sizeof(char), (size_t) strlen(subnode->val), fp);
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

int load(void) {
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("load(): opening file");
        return 1;
    }
    char *key = calloc(MAXWORDS, sizeof(char));
    char *val = calloc(MAXWORDS, sizeof(char));
    int mode = 2; /* 0 = key, 1 = val, 2 = wtf? */
    int i = 0, j = 0;
    char **words = (char **) calloc(MAXWORDS * MAXWORDS, sizeof(char));
    if (key == NULL || val == NULL || words == NULL) {
        perror("load(): calloc failed");
        return 1;
    }
    for (char c = fgetc(fp); c != EOF; c = fgetc(fp)) {
        if (c == '\0') continue;
        if (c == NEWKEY) {
            key = calloc(MAXWORDS, sizeof(char));
            mode = 0;
            i = 0;
        }
        else if (c == NEWVAL) {
            if (mode == 1) {
                words[j++] = key;
                words[j++] = val;
            }
            val = calloc(MAXWORDS, sizeof(char));
            mode = 1;
            i = 0;
        }
        else if (c == NEWLINE) {
            words[j++] = key;
            words[j++] = val;
            i = 0;
            mode = 2;
        }
        else if (mode == 2) {
            errno = EINVAL;
            perror("load(): file corrupted");
        }
        else if (i >= MAXWORDS) {
            errno = E2BIG;
            perror("load(): error processing file");
        }
        else if (mode == 1) {
            val[i++] = c;
        }
        else if (mode == 0) {
            key[i++] = c;
        }
        else {
            printf("EVERYTHING IS BROKEN - this should NEVER happen");
        }
    }
    unsigned int word = 0;
    for (; words[word] != NULL; word += 1) {
        if (word == 0) continue;
        store_kv(words[word-1], words[word]);
        printf("loaded: %s = %s", words[word-1], words[word]);
    }
    words = realloc(words, sizeof(char) * word);
    return 0;
}

