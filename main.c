/*
 * markov chains chatbot, main file
 */
#include <stdio.h>
#include <stdlib.h>
#include "markov.h"

int main() {
    printf("markov chatbot (shit edition!) version 0.01-alpha1\n");
    markov_database = db_init();
    /*printf("enter test strings:\n");
    for (int i = 0; i < 5; i++) {
        printf("> ");
        read_input(stdin);
    }
    printf("saving...");
    int retval = save();
    if (retval != 0) {
        printf("error :(\n)");
        exit(EXIT_FAILURE);
    }
    else {
        printf("done :D\n");
    }*/
    
    printf("loading in database...\n");
    int retval = load();
    printf("returned %d\n", retval);
    printf("saving database...\n");
    retval = save();
    printf("returned %d\n", retval);
    exit(EXIT_SUCCESS);
}
