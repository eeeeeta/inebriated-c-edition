/*
 * markov chains chatbot, main file
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "markov.h"
#define MAXWORDS 100

signed int r2w(char *into, char *from, int mlen) {
    unsigned int spaces = 0, i = 0;
    for (; from[i] != '\0'; i++) {
        if (i >= mlen) break;
        if (from[i] == ' ' && ++spaces == 2) break;
    }
    strncpy(into, from, i++);
    return (from[i] == '\0' ? (i - i*2) : i);
}
void read_data(char *text) {
    char **words = (char **) calloc(MAXWORDS, sizeof(char));
    signed int read_last = 0;
    for (unsigned int pno = 0;; pno++) {
        unsigned char *buf = (char *) calloc(MAXWORDS, sizeof(char));
        read_last = r2w(buf, text, MAXWORDS);
        words[pno] = buf;
        if (read_last < 0) {
            break;
        }
        text += read_last;
    }
    unsigned int mode = 0;
    unsigned int last = 0;
    for (unsigned int word = 0; words[word] != NULL; word += 1) {
        if (word == 0) continue;
        store_kv(words[word-1], words[word]);
    }
    free(words);
    return;
}
char *read_input(FILE *fp) {
    char *buf = (char *) calloc(MAXWORDS, sizeof(char));
    register unsigned int i = 0;
    for (char c; c != EOF && c != '\n'; c = fgetc(fp)) {
        if (i >= MAXWORDS) break;
        /* FIXME: why does fgetc(stdin) sometimes return a null? */
        if (c == '\0') c = fgetc(fp);
        buf[i++] = c;
    }
    if (ferror(fp)) {
        perror("reading file in read_input()");
    }
    read_data(buf);
}

int main() {
    printf("markov chatbot (shit edition!) version 0.01-alpha1\n");
    printf("insert a test string! > ");
    read_input(stdin);
    dbg_walk("a b");
}
