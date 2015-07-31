/*
 * network protocol functions
 */
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
            perror("send_all(): send failed");
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
 * Receives size amount of data from fd into buf, with custom timeout (in ms).
 * Set timeout to -1 to wait forever.
 */
static size_t net_recv_timed(int fd, void *buf, size_t size, int timeout) {
    size_t brcv = 0;
    int pret = 0;
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    pret = poll(fds, 1, timeout);
    if (pret > 0) {
        brcv = recv(fd, buf, size, MSG_WAITALL);
        if (brcv == 0) {
            fwprintf(stderr, L"net_recv_timed(): connection closed unexpectedly\n");
            net_fail(fd);
            return -1;
        }
        else if (brcv == -1) {
            perror("net_recv_timed(): recv failed");
            return -2;
        }
        return brcv;
    }
    else if (pret == 0) return -1;
    else {
        perror("net_recv_timed(): poll failed");
        return -2;
    }
}
/**
 * Receives one message from fd (with timeout tmout, -1 to disable).
 */
static char net_recv_msg_timed(int fd, int tmout) {
    char *msg = malloc(sizeof(char));
    if (msg == NULL) {
        perror("net_recv_msg_timed(): malloc failed");
        return INT_FAIL;
    }
    int ret = net_recv_timed(fd, msg, sizeof(char), tmout);
    if (ret == -2) return INT_TIMEOUT;
    if (ret == -1) return INT_FAIL;
    if (ret != sizeof(char)) {
        fwprintf(stderr, L"net_recv_msg_timed(): did not receive all bytes\n");
        return INT_FAIL;
    }
    return *msg;
}
/**
 * Calls net_recv_msg_timed with the default timeout (NET_TIMEOUT)
 * (because I'm too lazy to rewrite my code).
 */
static char net_recv_msg(int fd) {
    return net_recv_msg_timed(fd, NET_TIMEOUT);
}
/**
 * Calls net_recv_timed with the default timeout.
 */
static size_t net_recv(int fd, void *buf, size_t size) {
    return net_recv_timed(fd, buf, size, NET_TIMEOUT);
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
static bool net_recv_sentence(int fd) {
    uint32_t *nbo_len = malloc(sizeof(uint32_t));
    mbstate_t *ps = malloc(sizeof(mbstate_t));
    if (ps == NULL || nbo_len == NULL) {
        perror("net_recv_sentence(): malloc failed");
        net_send_msg(fd, MSG_SENTENCE_GENFAILED);
        return false;
    }
    memset(ps, 0, sizeof(mbstate_t));
    if (!net_send_msg(fd, MSG_SENDNOW)) return false;
    size_t brcv = net_recv(fd, nbo_len, sizeof(uint32_t));
    if (brcv != sizeof(uint32_t)) {
        if (brcv > 0) fwprintf(stderr, L"net_recv_sentence(): not enough bytes received for len (xptd %d got %d)\n", sizeof(uint32_t), brcv);
        return false;
    }
    uint32_t len = ntohl(*nbo_len);
    char *utf8_str = malloc(len);
    wchar_t *wsent = malloc(len);
    if (utf8_str == NULL || wsent == NULL) {
        perror("net_recv_sentence(): malloc failed");
        net_send_msg(fd, MSG_SENTENCE_GENFAILED);
        return false;
    }
    brcv = net_recv(fd, utf8_str, len);
    if (brcv != sizeof(uint32_t)) {
        if (brcv > 0) fwprintf(stderr, L"net_recv_sentence(): not enough bytes received for sent. (xptd %d got %d)\n", len, brcv);
        return false;
    }
    /*
     * =!= RADIOACTIVE AREA =!=
     * UTF-8 CONVERSION IN PROGRESS
     * DO NOT ENTER
     */
    if (mbsrtowcs(wsent, (const char **) &utf8_str, len, ps) == -1) {
        perror("net_recv_sentence(): utf8 conversion failed");
        net_send_msg(fd, MSG_SENTENCE_GENFAILED);
        return false;
    }
    if (utf8_str != NULL) fwprintf(stderr, L"net_recv_sentence(): WARNING: likely failure of wcsrtombs()!\n");
    if (!read_data(wsent, true)) {
        fwprintf(stderr, L"net_recv_sentence(): read_data() failed\n");
        net_send_msg(fd, MSG_SENTENCE_GENFAILED);
        return false;
    }
    if (!net_send_msg(fd, MSG_SENTENCE_ACK)) return false;
    return true;
}
/**
 * Sends a generated sentence, generated through generate_sentence(), to file descriptor fd.
 * (also, waits for the client's ack)
 */
static bool net_send_sentence(int fd) {
    const wchar_t *sentence = generate_sentence();
    if (sentence == NULL) {
        perror("net_send_sentence(): generate_sentence()");
        net_send_msg(fd, MSG_SENTENCE_GENFAILED);
        return false;
    }
    /* entering voodoo magic UTF-8 conversion zone
     * abandon hope, ye all who enter here */
    size_t ssize = wcslen(sentence) * sizeof(wchar_t);
    mbstate_t *ps = malloc(sizeof(mbstate_t));
    char *utf8_str = malloc(ssize);
    if (ps == NULL || utf8_str == NULL) {
        perror("net_send_sentence(): malloc failed");
        net_send_msg(fd, MSG_SENTENCE_GENFAILED);
        return false;
    }
    memset(ps, 0, sizeof(mbstate_t));
    uint32_t len = wcsrtombs(utf8_str, &sentence, ssize, ps);
    if (sentence != NULL) fwprintf(stderr, L"net_send_sentence(): WARNING: likely failure of wcsrtombs()!\n");
    uint32_t nbo_len = htonl(len);
    if (len == -1) {
        perror("net_send_sentence(): unicode conversion failed");
        net_send_msg(fd, MSG_SENTENCE_GENFAILED);
        return false;
    }
    if (sentence != NULL) {
        fwprintf(stderr, L"net_send_sentence(): unicode conversion failed: not all data converted (?!)\n");
        return false;
    }
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
    send_all(fd, utf8_str, len);
    if (net_recv_msg(fd) != MSG_SENTENCE_ACK) {
        fwprintf(stderr, L"net_send_sentence(): did not recieve ack\n");
        return false;
    }
    return true;
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
        if (msg == MSG_REQ_SEND_SENTENCE) {
            if (!net_recv_sentence(*fd)) {
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
        else if (msg == MSG_DB_SAVE) {
            if (!save(DB_FILENAME)) net_send_msg(*fd, MSG_DB_ERR);
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
