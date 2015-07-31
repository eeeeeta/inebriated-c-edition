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


/**
 * Internal function to get an inet address from a struct sockaddr.
 */
extern void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
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


