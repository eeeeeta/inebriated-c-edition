#ifndef _MARKOV_NET
#define _MARKOV_NET

#define NET_BACKLOG 3 /**< How many connections to tell listen() to keep a backlog for */
#define NET_PING_INTERVAL 45000 /**< time (in ms) between pings */
#define NET_TIMEOUT 4000 /**< time (in ms) to wait for a reply before giving up */

enum message {
    MSG_OHAI = '\x01', /**< (S <-> C) welcomes the client, or functions as a ping for the server */
    MSG_PONG, /**< (S <-> C) response to MSG_OHAI, also a null command */
    MSG_SEND_CMD, /**< (S -> C) request to send selection of command */
    MSG_GET_SENTENCE, /**< (C -> S) command to display one sentence */
    MSG_SENTENCE_GENFAILED, /**< (S -> C) tells client of failure to generate sentence (followed with MSG_SEND_CMD) */
    MSG_SENTENCE_LEN, /**< (S -> C) followed immediately by uint32_t length in Net Byte Order (big-endian), then sentence (UTF-8) */
    MSG_SENTENCE_ACK, /**< (C -> S) confirms reciept of sentence - server will follow with MSG_SEND_CMD */
    MSG_TERMINATE, /**< (C -> S) command to terminate connection */
    INT_FAIL, /**< used internally to signify a failed message reciept */
    INT_CLOSED, /**< used internally to signify a closed connection */
    INT_TIMEOUT, /**< used internally to specify a timeout event */
    INT_NULL /**< (internal) messages are initialised to this by default, for debugging */
};

extern int net_init(const char *port);
extern int net_listen(int fd);
#endif
