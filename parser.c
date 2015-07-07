/*
 * parser for input
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "markov.h"
#include "vbuf.h"

static signed int r2w(struct varstr *into, char *from, int mlen) {
    unsigned int spaces = 0, i = 0;
    for (; from[i] != '\0'; i++) {
        if (i >= mlen) break;
        if (from[i] == ' ' && ++spaces == 2) break;
    }
    varstr_ncat(into, from, i++);
    return (from[i] == '\0' ? (i - i*2) : i);
}
extern void read_data(char *text) {
    char *last;
    struct varstr *cur;
    signed int read_last = 0;
    for (last = NULL;;) {
        cur = varstr_init();
        if (cur == NULL) {
            perror("init varstr in read_input()");
            return;
        }
        read_last = r2w(cur, text, MAXWORDS);
        if (last != NULL) {
            char *v;
            if ((v = varstr_pack(cur)) == NULL) {
                perror("packing varstr in read_input()");
                return;
            }
            store_kv(last, v);
        }
        if ((last = varstr_pack(cur)) == NULL) {
            perror("packing varstr in read_input()");
            return;
        }
        if (read_last < 0) {
            break;
        }
        text += read_last;
    }
    return;
}
/**
 * Read a line of input from file pointer fp (pre-opened).
 * Returns 1 on EOF, 2 on error, and 0 otherwise.
 */
extern int read_input(FILE *fp) {
    struct varstr *buf = varstr_init();
    if (buf == NULL) {
        perror("init varstr in read_input()");
        return 2;
    }
    for (char c = fgetc(fp); c != EOF && c != '\n'; c = fgetc(fp)) {
        if (varstr_pushc(buf, c) == NULL) break;
    }
    if (ferror(fp)) {
        perror("reading file in read_input()");
        return 2;
    }
    char *str;
    if ((str = varstr_pack(buf)) == NULL) {
        perror("packing varstr in read_input()");
        return 2;
    }
    read_data(str);
    return (feof(fp) ? 1 : 0);
}
