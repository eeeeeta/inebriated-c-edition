/*
 * parser for input
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "markov.h"


static signed int r2w(char *into, char *from, int mlen) {
    unsigned int spaces = 0, i = 0;
    for (; from[i] != '\0'; i++) {
        if (i >= mlen) break;
        if (from[i] == ' ' && ++spaces == 2) break;
    }
    strncpy(into, from, i++);
    return (from[i] == '\0' ? (i - i*2) : i);
}
extern void read_data(char *text) {
    char **words = calloc(MAXWORDS, sizeof(char));
    signed int read_last = 0;
    for (unsigned int pno = 0;; pno++) {
        char *buf = calloc(MAXWORDS, sizeof(char));
        read_last = r2w(buf, text, MAXWORDS);
        words[pno] = buf;
        if (read_last < 0) {
            break;
        }
        text += read_last;
    }
    unsigned int word = 0;
    for (; words[word] != NULL; word += 1) {
        if (word == 0) continue;
        store_kv(words[word-1], words[word]);
    }
    words = realloc(words, sizeof(char) * word);
    return;
}
extern void read_input(FILE *fp) {
    char *buf = calloc(MAXWORDS, sizeof(char));
    register unsigned int i = 0;
    for (char c = fgetc(fp); c != EOF && c != '\n'; c = fgetc(fp)) {
        if (i >= MAXWORDS) break;
        buf[i++] = c;
    }
    if (ferror(fp)) {
        perror("reading file in read_input()");
    }
    read_data(buf);
    return;
}
