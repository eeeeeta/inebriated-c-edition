/*
 * parser for input
 */
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>
#include "markov.h"
#include "vbuf.h"

static signed int r2w(struct varstr *into, wchar_t *from) {
    unsigned int spaces = 0, i = 0;
    wchar_t c = '\2';
    for (; c != '\0'; c = from[i++]) {
        if (iswspace(c) != 0 && ++spaces == 2) break;
        if (iswspace(c) == 0 && iswpunct(c) == 0 && iswalnum(c) == 0) continue;
        if (varstr_pushc(into, c) == NULL) {
            perror("r2w(): varstr_pushc()");
            return -1;
        }
    }
    return (c == '\0' ? (i - i*2) : i);
}
extern void read_data(wchar_t *text) {
    wchar_t *last;
    struct varstr *cur;
    signed int read_last = 0;
    for (last = NULL;;) {
        cur = varstr_init();
        if (cur == NULL) {
            perror("init varstr in read_input()");
            return;
        }
        read_last = r2w(cur, text);
        if (last != NULL) {
            wchar_t *v;
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
    for (wchar_t c = fgetwc(fp); c != EOF && c != '\n'; c = fgetwc(fp)) {
        if (varstr_pushc(buf, c) == NULL) break;
    }
    if (ferror(fp)) {
        perror("reading file in read_input()");
        return 2;
    }
    wchar_t *str;
    if ((str = varstr_pack(buf)) == NULL) {
        perror("packing varstr in read_input()");
        return 2;
    }
    read_data(str);
    return (feof(fp) ? 1 : 0);
}
