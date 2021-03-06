/*
 * networking parts!
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
#include <poll.h>
#include <stdlib.h>
#include <wchar.h>
#include <unistd.h>
#include <stdbool.h>
#include "vbuf.h"
#include "markov.h"
#include "net.h"
#include "proto.h"

/*
 * Terminates a connection due to failure.
 */
extern void *net_fail(int fd) {
    close(fd);
    wprintf(L"net_fail(): closing fd %d due to errors\n", fd);
    return NULL;
}
/**
 * Internal function to get an inet address from a struct sockaddr.
 */
extern void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

extern void *net_handler_tfunc(void *fda) {
    int *fd = (int *) fda;
    char msg;
    if (!net_send_msg(*fd, MSG_OHAI)) return net_fail(*fd);
    if (net_recv_msg(*fd) != MSG_PONG) return net_fail(*fd);
    wprintf(L"net_handler_tfunc(): handshake completed (fd %d)\n", *fd);
    while (1) {
        /* main thread loop */
        if (!net_send_msg(*fd, MSG_SEND_CMD)) break;
        if ((int) (msg = net_recv_msg_or_ping(*fd)) >= (int) INT_FAIL) {
            fwprintf(stderr, L"net_handler_tfunc(): error receiving next command (fd %d)\n", *fd);
            break;
        }
        if (msg == MSG_GET_SENTENCE) {
            const wchar_t *sentence = generate_sentence();
            if (sentence == NULL) {
                perror("net_handler_tfunc(): generate_sentence failed");
                net_send_msg(*fd, MSG_SENTENCE_GENFAILED);
                continue;
            }
            if (!net_send_sentence(*fd, sentence)) {
                msg = INT_FAIL;
                break;
            }
        }
        else if (msg == MSG_SENTENCE_LEN) {
            wchar_t *s = net_recv_sentence(*fd);
            if (s == NULL) {
                msg = INT_FAIL;
                break;
            }
            if (!read_data(s, true)) {
                fwprintf(stderr, L"net_handler_tfunc(): read_data failed");
                continue;
            }
        }
        else if (msg == MSG_TERMINATE) break;
        else if (msg == MSG_OHAI) {
            if (!net_send_msg(*fd, MSG_PONG)) {
                fwprintf(stderr, L"net_handler_tfunc(): we failed to return ping (fd %d)\n", *fd);
                msg = INT_FAIL;
                break;
            }
        }
        else if (msg == MSG_DB_SAVE) {
            if (!save(DB_FILENAME)) net_send_msg(*fd, MSG_SAVE_ERR);
            else net_send_msg(*fd, MSG_SAVED);
        }
        else if (msg == MSG_PONG) continue;
        else {
            fwprintf(stderr, L"net_handler_tfunc(): got unknown message type (%d) from fd %d\n", msg, *fd);
        }
    }
    if (msg == INT_FAIL) return net_fail(*fd);
    else if (msg == INT_TIMEOUT) return net_fail(*fd);
    else if (msg == INT_CLOSED) return NULL;
    else {
        close(*fd);
        wprintf(L"net_handler_tfunc(): fd %d terminated\n", *fd);
        return NULL;
    }
}
extern int net_init() {
    struct addrinfo hints;
    struct addrinfo *info;
    int status, socket_fd;
    int yes = 1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((status = getaddrinfo(NULL, NET_PORT, &hints, &info)) != 0) {
        fwprintf(stderr, L"net_init(): getaddrinfo() failed: %s\n", gai_strerror(status));
        if (status == EAI_SYSTEM) {
            perror("net_init(): getaddrinfo() failed due to system error");
        }
        return -1;
    }
    /* try each result in the linked list */
    struct addrinfo *addr = NULL;
    for (addr = info; addr != NULL; addr = addr->ai_next) {
        if ((socket_fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1) {
            perror("net_init(): socket() failed");
            continue;
        }
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("net_init(): setsockopt() failed");
            exit(EXIT_FAILURE);
        }
        if (bind(socket_fd, addr->ai_addr, addr->ai_addrlen) == -1) {
            perror("net_init(): bind() failed");
            continue;
        }
        break;
    }
    if (addr == NULL) {
        fwprintf(stderr, L"net_init(): binding failed!\n");
        return -1;
    }
    freeaddrinfo(info);
    return socket_fd;
}
extern int net_listen(int fd) {
    if (listen(fd, NET_BACKLOG) == -1) {
        perror("net_listen(): listen() failed");
        return 1;
    }
    char otheraddr_readable[INET6_ADDRSTRLEN];
    pthread_t thread;
    while (1) {
        int *sockfd = malloc(sizeof(int));
        struct sockaddr_storage *addr = malloc(sizeof(struct sockaddr_storage));
        if (sockfd == NULL || addr == NULL) {
            perror("net_listen(): malloc() failed");
            continue;
        }
        socklen_t addr_size = sizeof(struct sockaddr_storage);
        *sockfd = accept(fd, (struct sockaddr *) addr, &addr_size);
        if (*sockfd == -1) {
            perror("net_listen(): accept() failed");
            continue;
        }
        fcntl(*sockfd, F_SETFL, O_NONBLOCK);
        inet_ntop(addr->ss_family, get_in_addr((struct sockaddr *) addr), otheraddr_readable, sizeof(otheraddr_readable));
        wprintf(L"net_listen(): got connection from %s (fd %d)\n", otheraddr_readable, *sockfd);
        wprintf(L"net_listen(): starting thread to handle fd %d\n", *sockfd);
        pthread_create(&thread, NULL, net_handler_tfunc, sockfd);
    }
}


