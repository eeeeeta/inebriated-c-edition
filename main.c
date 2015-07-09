/*
 * markov chains chatbot, main file
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include "markov.h"

static int load_with_output(void) {
    wprintf(L"loading database...");
    int retval = load();
    if (retval == 2) {
        wprintf(L"you don't have one!\n");
        return retval;
    }
    if (retval != 0) {
        wprintf(L"error!\n");
        exit(EXIT_FAILURE);
    }
    wprintf(L"done\n");
    return 0;
}
static void gen_with_output(void) {
    wchar_t c = L'y';
    while (c != EOF) {
    wchar_t *sent = generate_sentence();
    if (sent == NULL) {
        wprintf(L"error\n");
        perror("generate_sentence()");
        exit(EXIT_FAILURE);
    }
    wprintf(L"Sentence: \"%ls\"\n", sent);
    wprintf(L"Another? (ENTER for yes, Ctrl-D to stop) ");
    c = fgetc(stdin);
    }
    exit(EXIT_SUCCESS);
}
static void save_with_output(void) {
    wprintf(L"saving database...");
    int retval = save();
    if (retval != 0) {
        wprintf(L"error!\n");
        exit(EXIT_FAILURE);
    }
    wprintf(L"done\n");
    exit(EXIT_SUCCESS);
}
static void input_new_data(void) {
    wprintf(L"inputting new data, Control-D to stop\n");
    while (read_input(stdin) == 0)
        ;
    save_with_output();
}
static void infile_with_output(void) {
    wprintf(L"inputting new data from file './infile.txt'\n");
    FILE *fp;
    fp = fopen("./infile.txt", "r");
    if (fp == NULL) {
        perror("infile_with_output()");
        exit(EXIT_FAILURE);
    }
    wprintf(L"reading from file (can take some time)...");
    while (read_input(fp) == 0)
        ;
    wprintf(L"done!\n");
    save_with_output();
}
int main(int argc, char *argv[]) {
    wprintf(L"inebriated, C version, by eeeeeta\n");
    markov_database = db_init();
    srand(time(0));
    if (argc < 2) {
        fwprintf(stderr, L"Syntax: %s [action]\n", argv[0]);
        fwprintf(stderr, L"action: one of [input, gen, infile]\n");
        exit(EXIT_FAILURE);
    }
    int ret = load_with_output();
    if (strcmp("input", argv[1]) == 0) {
        input_new_data();
    }
    else if (strcmp("gen", argv[1]) == 0 && ret != 2) {
        gen_with_output();
    }
    else if (strcmp("infile", argv[1]) == 0) {
        infile_with_output();
    }
    else {
        fwprintf(stderr, L"[!] Either you didn't specify a valid action, or you tried to use the database without having one.\n");
    }
}
