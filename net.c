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

pthread_mutex_t sentence_lock; /**< mutex for writing to the markov db (currently unused) */
/**
 * Internal function to get an inet address from a struct sockaddr.
 */
static void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}
/*
 * Terminates a connection due to failure.
 */
static void *net_fail(int fd) {
    close(fd);
    wprintf(L"net_fail(): closing fd %d due to errors\n", fd);
    return NULL;
}
/**
 * Wrapper for send() that ensures all the data is sent.
 */
static int send_all(int fd, const void *buf, size_t length) {
    size_t total = length;
    int ret = 0;
    size_t sent = 0;
    for (; sent < length;) {
        ret = send(fd, buf, total, MSG_NOSIGNAL);
        if (ret == -1) {
            perror("net_send_sentence(): send failed");
            return false;
        }
        sent += ret;
        if (sent == length) break;
        buf += sent;
        total -= sent;
    }
    return sent;
}
/**
 * Sends a message (see enum message type) to a file descriptor (fd).
 */
static bool net_send_msg(int fd, enum message msg) {
    char byte = msg;
    int sent = send_all(fd, &byte, 1);
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
/**
 * Receives one message (enum message type) from fd, with custom timeout (in ms).
 * Set timeout to -1 to wait forever.
 */
static char net_recv_msg_timed(int fd, int timeout) {
    char msg = INT_NULL;
    int brcv = 0, pret = 0;
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    pret = poll(fds, 1, timeout);
    if (pret > 0) {
        brcv = recv(fd, &msg, 1, 0);
        if (brcv == 0) {
            wprintf(L"net_recv_msg_timed(): connection closed unexpectedly\n");
            net_fail(fd);
            return INT_CLOSED;
        }
        else if (brcv == -1) {
            perror("net_recv_msg_timed(): recv failed");
            return INT_FAIL;
        }
        return msg;
    }
    else if (pret == 0) return INT_TIMEOUT;
    else {
        perror("net_recv_msg_timed(): poll failed");
        return INT_FAIL;
    }
}
/**
 * Calls net_recv_msg_timed with the default timeout (NET_TIMEOUT)
 * (because I'm too lazy to rewrite my code).
 */
static char net_recv_msg(int fd) {
    return net_recv_msg_timed(fd, NET_TIMEOUT);
}
/**
 * Calls net_recv_msg_timed in a loop to check for new messages. If the connection
 * times out, sends a MSG_OHAI to ensure it's still alive and loops again.
 */
static char net_recv_msg_or_ping(int fd) {
    char msg = INT_NULL;
    while (1) {
        msg = net_recv_msg_timed(fd, NET_PING_INTERVAL);
        if (msg != INT_TIMEOUT) return msg;
        else if (!net_send_msg(fd, MSG_OHAI)) {
            net_fail(fd);
            return INT_CLOSED;
        }
        msg = net_recv_msg(fd);
        if (msg != MSG_PONG) {
            fwprintf(stderr, L"net_recv_msg_or_ping(): timeout (failed to respond to ping), closing connection\n");
            net_fail(fd);
            return INT_CLOSED;
        }
        msg = INT_NULL;
    }
}
/**
 * Sends a generated sentence, generated through generate_sentence(), to file descriptor fd.
 * (also, waits for the client's ack)
 */
static bool net_send_sentence(int fd) {
    wchar_t *sentence;
    sentence = generate_sentence();
    if (sentence == NULL) {
        perror("net_send_sentence(): generate_sentence()");
        return net_send_msg(fd, MSG_SENTENCE_GENFAILED);
    }
    uint32_t len = wcslen(sentence) * sizeof(wchar_t);
    uint32_t nbo_len = htonl(len);
    if (!net_send_msg(fd, MSG_SENTENCE_LEN)) return false;
    int sent = 0;
    sent = send_all(fd, &nbo_len, sizeof(uint32_t));
    if (sent == -1) {
        perror("net_send_sentence(): send failed");
        return false;
    }
    else if (sent != sizeof(uint32_t)) {
        fwprintf(stderr, L"net_send_sentence(): sent %d bytes, expected %d\n", sent, sizeof(uint32_t));
        return false;
    }
    send_all(fd, sentence, len);
    if (net_recv_msg(fd) != MSG_SENTENCE_ACK) {
        fwprintf(stderr, L"net_send_sentence(): did not recieve ack\n");
        return false;
    }
    return true;
}
static void *net_handler_tfunc(void *fda) {
    int *fd = (int *) fda;
    char msg;
    if (!net_send_msg(*fd, MSG_OHAI)) return net_fail(*fd);
    if (net_recv_msg(*fd) != MSG_PONG) return net_fail(*fd);
    wprintf(L"net_handler_tfunc(): handshake completed (fd %d)\n", *fd);
    while (1) {
        /* main thread loop */
        if (!net_send_msg(*fd, MSG_SEND_CMD)) break;
        if ((int) (msg = net_recv_msg_or_ping(*fd)) > (int) INT_FAIL) {
            fwprintf(stderr, L"net_handler_tfunc(): error receiving next command (fd %d)\n", *fd);
            break;
        }
        if (msg == MSG_GET_SENTENCE) {
            if (!net_send_sentence(*fd)) {
                msg = INT_FAIL;
                break;
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
    if (listen(fd, NET_BACKLOG) == -1) {
        perror("net_listen(): listen() failed");
        return 1;
    }
    /* multithreading */
    if (pthread_mutex_init(&sentence_lock, NULL) != 0) {
        perror("net_listen(): multithreading: pthread_mutex_init() failed");
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


