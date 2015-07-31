/*
 * markov chains chatbot, main file
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <stdbool.h>
#include <unistd.h>
#include "markov.h"
#include "net.h"

char *DB_FILENAME = "markov_keys.mkdb";
char *NET_PORT = "7070";
static char *infile = "infile.txt";
static int load_with_output(void) {
    wprintf(L"loading database...\n");
    int retval = load(DB_FILENAME);
    if (retval == 2) {
        wprintf(L"No database found.\n");
        return retval;
    }
    if (retval != 0) {
        wprintf(L"Failed to load database!\n");
        exit(EXIT_FAILURE);
    }
    wprintf(L"database ready\n");
    return 0;
}
static void gen_with_output(void) {
    wchar_t c = L'y';
    while (c != EOF) {
        wchar_t *sent = generate_sentence();
        if (sent == NULL) {
            perror("generate_sentence()");
            wprintf(L"Sentence generation failed!\n");
            exit(EXIT_FAILURE);
        }
        wprintf(L"Sentence: \"%ls\"\n", sent);
        wprintf(L"Another? (ENTER for yes, Ctrl-D to stop) ");
        c = fgetc(stdin);
    }
    exit(EXIT_SUCCESS);
}
static void save_with_output(void) {
    wprintf(L"saving database...\n");
    bool retval = save(DB_FILENAME);
    if (retval != true) {
        wprintf(L"Database saving failed!\n");
        exit(EXIT_FAILURE);
    }
    wprintf(L"database saved\n");
    exit(EXIT_SUCCESS);
}
static void input_new_data(void) {
    wprintf(L"inputting new data, Control-D to stop\n");
    while (read_input(stdin, true) == 0)
        ;
    save_with_output();
}
static void infile_with_output(void) {
    wprintf(L"inputting new data from file '%s'\n", infile);
    FILE *fp;
    fp = fopen(infile, "r");
    if (fp == NULL) {
        perror("infile_with_output()");
        exit(EXIT_FAILURE);
    }
    wprintf(L"reading from file (can take some time)...\n");
    while (read_input(fp, false) == 0)
        ;
    wprintf(L"read done!\n");
    save_with_output();
}
static void floodgates(void) {
    wprintf(L"opening the floodgates! paste as much data as you wish below (Ctrl-D to stop)\n");
    while (read_input(stdin, false) == 0)
        ;
    wprintf(L"floodgates closed\n");
    save_with_output();
}
static void syntax_lecture(char *name) {
    fwprintf(stderr, L"Syntax: %s [-d dbfile] [-p port] [-f infile] action\n", name);
    fwprintf(stderr, L"action: one of the following:\n");
    fwprintf(stderr, L"\tinput: input new sentences from terminal\n");
    fwprintf(stderr, L"\tgen: generate a sentence\n");
    fwprintf(stderr, L"\tinfile: load sentences from infile.txt or file specified by -f\n");
    fwprintf(stderr, L"\tnet: test networking on port 7070 or port specified by -p\n");
    fwprintf(stderr, L"\tflood: data dump a bunch of incoherent sentences into the database\n");
    fwprintf(stderr, L"-d DBFILE: adjust database file\n");
    exit(EXIT_FAILURE);
}
int main(int argc, char *argv[]) {
    int opt;
    wprintf(L"inebriated, C version, by eeeeeta\n");
    while ((opt = getopt(argc, argv, "d:f:p:")) != -1) {
        switch (opt) {
            case 'd':
                DB_FILENAME = optarg;
                break;
            case 'p':
                NET_PORT = optarg;
                break;
            case 'f':
                infile = optarg;
                break;
            default:
                syntax_lecture(argv[0]);
                break;
        }
    }
    markov_database = db_init();
    srand(time(0));
    if (optind >= argc) {
        syntax_lecture(argv[0]);
    }
    char *action = argv[optind];
    int ret = load_with_output();
    if (strcmp("input", action) == 0) {
        input_new_data();
    }
    else if (strcmp("gen", action) == 0 && ret != 2) {
        gen_with_output();
    }
    else if (strcmp("infile", action) == 0) {
        infile_with_output();
    }
    else if (strcmp("flood", action) == 0) {
        floodgates();
    }
    else if (strcmp("net", action) == 0 && ret != 2) {
        wprintf(L"initialising networking test...");
        int fd;
        fd = net_init();
        if (fd == -1) {
            wprintf(L"failed\n");
            exit(EXIT_FAILURE);
        }
        wprintf(L"done!\n", fd);
        wprintf(L"Listening for connections (port %s) - database: %s\n", NET_PORT, DB_FILENAME);
        net_listen(fd);
    }
    else {
        fwprintf(stderr, L"[!] Either you didn't specify a valid action, or you tried to use the database without having one.\n");
        syntax_lecture(argv[0]);
    }
}
