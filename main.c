/*
 * markov chains chatbot, main file
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "markov.h"

static void load_with_output(void) {
    printf("loading database...");
    int retval = load();
    if (retval != 0) {
        printf("error!\n");
        exit(EXIT_FAILURE);
    }
    printf("done\n");
}
static void gen_with_output(void) {
    printf("running generate_sentence()...");
    char *sent = generate_sentence();
    if (sent == NULL) {
        printf("error\n");
        perror("generate_sentence()");
        exit(EXIT_FAILURE);
    }
    printf("done\n");
    printf("Sentence: \"%s\"\n", sent);
}
static void save_with_output(void) {
    printf("saving database...");
    int retval = save();
    if (retval != 0) {
        printf("error!\n");
        exit(EXIT_FAILURE);
    }
    printf("done\n");
    exit(EXIT_SUCCESS);
}
static void input_new_data(void) {
    printf("inputting new data, Control-D to stop\n");
    while (read_input(stdin) == 0)
        ;
    save_with_output();
}
static void infile_with_output(void) {
    printf("inputting new data from file './infile.txt'\n");
    FILE *fp;
    fp = fopen("./infile.txt", "r");
    if (fp == NULL) {
        perror("infile_with_output()");
        exit(EXIT_FAILURE);
    }
    while (read_input(fp) == 0)
        ;
    save_with_output();
}
int main(int argc, char *argv[]) {
    printf("inebriated, C version, by eeeeeta\n");
    markov_database = db_init();
    srand(time(0));
    if (argc < 2) {
        fprintf(stderr, "Syntax: %s [action]\n", argv[0]);
        fprintf(stderr, "action: one of [input, gen, infile]\n");
        exit(EXIT_FAILURE);
    }
    load_with_output();
    if (strcmp("input", argv[1]) == 0) {
        input_new_data();
    }
    else if (strcmp("gen", argv[1]) == 0) {
        gen_with_output();
    }
    else if (strcmp("infile", argv[1]) == 0) {
        infile_with_output();
    }
    else {
        fprintf(stderr, "unrecognized action\n");
    }
}
