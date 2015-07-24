/*
 * networking parts!
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <unistd.h>
#include <stdbool.h>
#include "vbuf.h"
#include "markov.h"

#define BACKLOG 10

enum message {
    MSG_OHAI = '\x01', /*<< (S -> C) welcomes the client */
    MSG_PONG, /*<< (C -> S) response to MSG_OHAI, also a null command */
    MSG_SEND_CMD, /**< (S -> C) request to send selection of command */
    MSG_GET_SENTENCE, /*<< (C -> S) command to display one sentence */
    MSG_SENTENCE_GENFAILED, /**< (S -> C) tells client of failure to generate sentence (followed with MSG_SEND_CMD) */
    MSG_SENTENCE_LEN, /*<< (S -> C) followed immediately by uint32_t length in Net Byte Order (big-endian), then sentence (using wchar_t) */
    MSG_SENTENCE_ACK, /*<< (C -> S) confirms reciept of sentence - server will follow with MSG_SEND_CMD */
    MSG_TERMINATE, /*<< (C -> S) command to terminate connection */
    INT_FAIL, /**< used internally to signify a failed message reciept */
    INT_CLOSED /*<< used internally to signify a closed connection */
};

pthread_mutex_t sentence_lock;
static void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}
static bool net_send_msg(int fd, enum message msg) {
    char byte = msg;
    int sent = send(fd, &byte, 1, 0);
    if (sent == -1) {
        perror("net_send_msg(): send failed");
        return false;
    }
    else if (sent != 1) {
        fwprintf(stderr, L"net_send_msg(): sent %d bytes, expected 1\n", sent);
        return false;
    }
    else {
        return true;
    }
}
static char net_recv_msg(int fd) {
    char msg = '?';
    int brcv = 0;
    brcv = recv(fd, &msg, 1, 0);
    if (brcv == 0) {
        wprintf(L"net_recv_msg(): connection closed unexpectedly\n");
        return INT_CLOSED;
    }
    else if (brcv == -1) {
        perror("net_recv_msg(): recv failed");
        return INT_FAIL;
    }
    return msg;
}
static bool net_send_sentence(int fd) {
    pthread_mutex_lock(&sentence_lock);
    wchar_t *sentence;
    sentence = generate_sentence();
    pthread_mutex_unlock(&sentence_lock);
    if (sentence == NULL) {
        perror("net_send_sentence(): generate_sentence()");
        return net_send_msg(fd, MSG_SENTENCE_GENFAILED);
    }
    uint32_t len = wcslen(sentence) * sizeof(wchar_t);
    uint32_t nbo_len = htonl(len);
    if (!net_send_msg(fd, MSG_SENTENCE_LEN)) return false;
    int sent = 0;
    sent = send(fd, &nbo_len, sizeof(uint32_t), 0);
    if (sent == -1) {
        perror("net_send_sentence(): send failed");
        return false;
    }
    else if (sent != sizeof(uint32_t)) {
        fwprintf(stderr, L"net_send_sentence(): sent %d bytes, expected %d\n", sent, sizeof(uint32_t));
        return false;
    }
    for (sent = 0; sent < len;) {
        sent = send(fd, sentence, len, 0);
        if (sent == -1) {
            perror("net_send_sentence(): send failed");
            return false;
        }
        if (sent == len) break;
        sentence += sent;
        len -= sent;
    }
    if (net_recv_msg(fd) != MSG_SENTENCE_ACK) {
        fwprintf(stderr, L"net_send_sentence(): did not recieve ack\n");
        return false;
    }
    return true;
}
static void *net_fail(int fd) {
    close(fd);
    wprintf(L"net_fail(): closing fd %d due to errors\n", fd);
    return NULL;
}
static void *net_handler_tfunc(void *fda) {
    int *fd = (int *) fda;
    char msg;
    wprintf(L"dbg: sending fd %d MSG_OHAI\n", *fd);
    if (!net_send_msg(*fd, MSG_OHAI)) return net_fail(*fd);
    wprintf(L"dbg: waiting for PONG\n");
    if (net_recv_msg(*fd) != MSG_PONG) return net_fail(*fd);
    while (1) {
        /* main thread program loop */
        if (!net_send_msg(*fd, MSG_SEND_CMD)) break;
        if ((msg = net_recv_msg(*fd)) == INT_FAIL) {
            fwprintf(stderr, L"net_handler_tfunc(): error recieving next command (timeout?)\n");
            break;
        }
        if (msg == MSG_GET_SENTENCE) {
            if (!net_send_sentence(*fd)) {
                msg = INT_FAIL;
                break;
            }
        }
        else if (msg == MSG_TERMINATE) break;
        else if (msg == MSG_PONG) continue;
        else {
            fwprintf(stderr, L"net_handler_tfunc(): got unknown message type (%d) from client\n", msg);
        }
    }
    if (msg == INT_FAIL) return net_fail(*fd);
    else {
        close(*fd);
        wprintf(L"net_handler_tfunc(): fd %d terminated\n", *fd);
        return NULL;
    }
}
extern int net_init(const char *port) {
    struct addrinfo hints;
    struct addrinfo *info;
    int status, socket_fd;
    int yes = 1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((status = getaddrinfo(NULL, port, &hints, &info)) != 0) {
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
    if (listen(fd, BACKLOG) == -1) {
        perror("net_listen(): listen() failed");
        return 1;
    }
    /* multithreading */
    if (pthread_mutex_init(&sentence_lock, NULL) != 0) {
        perror("net_listen(): multithreading: pthread_mutex_init() failed");
        return 1;
    }
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
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
        setsockopt(*sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
        inet_ntop(addr->ss_family, get_in_addr((struct sockaddr *) addr), otheraddr_readable, sizeof(otheraddr_readable));
        wprintf(L"net_listen(): got connection from %s (fd %d)\n", otheraddr_readable, *sockfd);
        wprintf(L"dbg: starting thread (welp) to handle fd %d\n", *sockfd);
        pthread_create(&thread, NULL, net_handler_tfunc, sockfd);
    }
}


