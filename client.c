/*
 * example client
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <poll.h>
#include <stdlib.h>
#include <wchar.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include "proto.h"
#include "vbuf.h"

extern void *net_fail(int fd) {
    fwprintf(stderr, L"errors occurred, quitting\n");
    exit(EXIT_FAILURE);
}
static void *recv_tfunc(void *fdp) {
    int socket_fd = *((int *) fdp);
    while (1) {
        enum message msg = net_recv_msg(socket_fd);
        if ((int) msg >= (int) INT_FAIL && msg != INT_TIMEOUT) {
            fwprintf(stderr, L"error receiving next message (%d)\n", (int) msg);
            net_fail(socket_fd);
        }
        else if (msg == MSG_OHAI) {
            if (!net_send_msg(socket_fd, MSG_PONG)) {
                fwprintf(stderr, L"failed to return ping\n");
                net_fail(socket_fd);
            }
            wprintf(L"PONG! replied to server's ping\n");
        }
        else if (msg == MSG_PONG) {
            wprintf(L"server is still there! :D\n");
        }
        else if (msg == MSG_SENTENCE_LEN) {
            wchar_t *sent = net_recv_sentence(socket_fd);
            if (sent == NULL) {
                fwprintf(stderr, L"error receiving sentence\n");
                continue;
            }
            wprintf(L"sentence: %ls\n", sent);
        }
        else if (msg == MSG_SENTENCE_GENFAILED) {
            wprintf(L"sentence transmission error\n");
        }
        else if (msg == MSG_SAVE_ERR) {
            wprintf(L"db save error\n");
        }
        else if (msg == MSG_SAVED) {
            wprintf(L"database saved\n");
        }
    }
    net_fail(socket_fd);
    return NULL;
}
static void syntax_lecture(char *name) {
    fwprintf(stderr, L"Syntax: %s [-h host] [-p port] [-l locale]\n", name);
    fwprintf(stderr, L"-h HOST: (default localhost) adjust host\n");
    fwprintf(stderr, L"-p PORT: (default 7070) adjust port\n");
    fwprintf(stderr, L"-l LOCALE: adjust locale used for character encoding\n");
    exit(EXIT_FAILURE);
}
int main(int argc, char *argv[]) {
    int opt;
    char *port = "7070";
    char *host = "localhost";
    wprintf(L"inebriated, C version, by eeeeeta - reference client\n");
    char *locale_ctype = "";
    while ((opt = getopt(argc, argv, "h:p:l:")) != -1) {
        switch (opt) {
            case 'l':
                locale_ctype = optarg;
                break;
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            default:
                syntax_lecture(argv[0]);
                break;
        }
    }
    if (setlocale(LC_CTYPE, locale_ctype) == NULL) {
        perror("error setting locale");
        exit(EXIT_FAILURE);
    }
    wprintf(L"connecting to %s port %s...\n", host, port);
    struct addrinfo hints;
    struct addrinfo *info;
    int status, socket_fd;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    if ((status = getaddrinfo(host, port, &hints, &info)) != 0) {
        fwprintf(stderr, L"getaddrinfo() failed: %s\n", gai_strerror(status));
        if (status == EAI_SYSTEM) {
            perror("getaddrinfo() failed due to system error");
        }
        return -1;
    }
    /* try each result in the linked list */
    struct addrinfo *addr = NULL;
    for (addr = info; addr != NULL; addr = addr->ai_next) {
        if ((socket_fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1) {
            perror("socket() failed");
            continue;
        }
        if (connect(socket_fd, addr->ai_addr, addr->ai_addrlen) == -1) {
            perror("connect() failed");
            continue;
        }
        break;
    }
    if (addr == NULL) {
        fwprintf(stderr, L"connecting failed!\n");
        return -1;
    }
    freeaddrinfo(info);
    wprintf(L"connected on fd %d\n", socket_fd);
    if (net_recv_msg(socket_fd) != MSG_OHAI) {
        fwprintf(stderr, L"failed to receive OHAI\n");
        net_fail(socket_fd);
    }
    if (!net_send_msg(socket_fd, MSG_PONG)) net_fail(socket_fd);
    wprintf(L"handshake complete\n");
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, &recv_tfunc, &socket_fd);
    wchar_t act;
    wprintf(L"commands: (g) get sentence (t) terminate (e) send sentence (s) queue db save (p) ping\n");
    wprintf(L"type [command]<Enter> to execute\n");
    while (1) {
        act = fgetwc(stdin);
        if (fgetwc(stdin) != L'\n') continue;
        switch (act) {
            case L'g':
                if (!net_send_msg(socket_fd, MSG_GET_SENTENCE)) {
                    wprintf(L"failed to send command\n");
                    net_fail(socket_fd);
                }
                wprintf(L"command sent\n");
                break;
            case L'p':
                if (!net_send_msg(socket_fd, MSG_OHAI)) {
                    wprintf(L"failed to send command\n");
                    net_fail(socket_fd);
                }
                wprintf(L"command sent\n");
                break;
            case L't':
                if (!net_send_msg(socket_fd, MSG_TERMINATE)) net_fail(socket_fd);
                wprintf(L"command sent, shutting down client\n");
                exit(EXIT_SUCCESS);
                break;
            case L's':
                if (!net_send_msg(socket_fd, MSG_DB_SAVE)) {
                    wprintf(L"failed to send command\n");
                    net_fail(socket_fd);
                }
                wprintf(L"command sent\n");
                break;
            case L'e':
                wprintf(L"enter sentence now, followed by <Enter>\n");
                struct varstr *buf = varstr_init();
                for (wchar_t c = fgetwc(stdin); c != WEOF && c != L'\n'; c = fgetwc(stdin)) {
                    if (varstr_pushc(buf, c) == NULL) break;
                }
                wchar_t *s = varstr_pack(buf);
                wprintf(L"transmitting sentence: '%ls'\n", s);
                if (!net_send_sentence(socket_fd, s)) {
                    wprintf(L"failed");
                    net_fail(socket_fd);
                }
                else {
                    wprintf(L"completed!\n");
                }
            default:
                wprintf(L"invalid command\n");
                break;
        }
    }
}
