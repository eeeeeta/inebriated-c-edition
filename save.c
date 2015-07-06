/*
 * save and load
 */
#include <stdio.h>
#include <stdlib.h>
#include "markov.h"

static const unsigned int NEWKEY = '\x11';
static const unsigned int NEWVAL = '\x12';
static const unsigned int NEWLINE = '\n';
int save(void) {
    FILE *fp;
    const char *filename = "markov_keys.mkdb";
    fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("save()");
        return 1;
    }
    struct kv_node *curnode;
    int i = 0;
    for (curnode = keys[i]; curnode != NULL; curnode = keys[++i]) {
        fwrite(&NEWKEY, sizeof(unsigned int), (size_t) 1, fp);
        fwrite(curnode->key, sizeof(char), sizeof(curnode->key), fp);
        printf("Saving: \"%s\" = ", curnode->key);
        fwrite(&NEWVAL, sizeof(unsigned int), (size_t) 1, fp);
        fwrite(curnode->val, sizeof(char), sizeof(curnode->val), fp);
        printf("\"%s\"", curnode->val);
        if (curnode->next != NULL) {
            struct kv_node *subnode;
            subnode = curnode->next;
            for (; subnode != NULL; subnode = subnode->next) {
                fwrite(&NEWVAL, sizeof(unsigned int), (size_t) 1, fp);
                fwrite(subnode->val, sizeof(char), sizeof(subnode->val), fp);
                printf(", \"%s\"", subnode->val);
            }
        }
        fwrite(&NEWLINE, sizeof(unsigned int), (size_t) 1, fp);
        printf("\n");
    }
    if (ferror(fp)) {
        perror("save()");
        return 1;
    }
    return 0;
}
