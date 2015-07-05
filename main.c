/*
 * markov chains chatbot, main file
 */
#include <stdio.h>
#include "markov.h"

int main() {
    printf("markov chatbot (shit edition!) version 0.01-alpha1\n");
    printf("insert a test string! > ");
    read_input(stdin);
    void dbg_walk(char *);
    dbg_walk("a b");
}
