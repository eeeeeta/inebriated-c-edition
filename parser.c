/*
 * parser for input
 */
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>
#include <stdbool.h>
#include "markov.h"

/**
 * Reads two words into a varstr from a UCS-2 string.
 * Returns (negative if end of string) the amount of characters
 * to advance the buffer by.
 */
static signed int r2w(struct varstr *into, wchar_t *from) {
    register int spaces = 0, i = 0;
    wchar_t c = L'\2';
    for (; c != L'\0'; c = from[i++]) {
        if (iswspace(c) != 0 && ++spaces == 2) break;
        if (c == L'\n') break;
        if (iswspace(c) == 0 && iswpunct(c) == 0 && iswalnum(c) == 0) continue;
        if (varstr_pushc(into, c) == NULL) {
            perror("r2w(): varstr_pushc()");
            return -1;
        }
    }
    return (c == L'\0' ? (i - i*2) : i);
}
/**
 * Loops through text and, calling r2w() on it, breaks it up into (and stores)
 * kv pairs.
 *
 * Set is_sentence according to whether the text should be interpreted as a sentence or not.
 */
extern bool read_data(wchar_t *text, bool is_sentence) {
    wchar_t *last;
    struct varstr *cur;
    signed int read_last = 0;
    int is_ss = (is_sentence ? 0 : 1);
    for (last = NULL;;) {
        cur = varstr_init();
        if (cur == NULL) {
            perror("init varstr in read_data()");
            return false;
        }
        read_last = r2w(cur, text);
        if (last != NULL) {
            wchar_t *v;
            if ((v = varstr_pack(cur)) == NULL) {
                perror("packing varstr in read_data()");
                return false;
            }
            store_kv(last, v, is_ss);
            is_ss = 1;
        }
        if ((last = varstr_pack(cur)) == NULL) {
            perror("packing varstr in read_data()");
            return false;
        }
        if (read_last < 0) {
            break;
        }
        text += read_last;
    }
    return true;
}
/**
 * Read a line of input from file pointer fp (pre-opened).
 * Returns 1 on EOF, 2 on error, and 0 otherwise.
 *
 * is_sentence == true if the line is a complete sentence.
 */
extern int read_input(FILE *fp, bool is_sentence) {
    struct varstr *buf = varstr_init();
    if (buf == NULL) {
        perror("init varstr in read_input()");
        return 2;
    }
    for (wchar_t c = fgetwc(fp); c != WEOF && c != L'\n'; c = fgetwc(fp)) {
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
    if (!read_data(str, is_sentence)) return 2;
    return (feof(fp) ? 1 : 0);
}
