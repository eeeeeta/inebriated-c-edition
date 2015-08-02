#ifndef _MARKOV_NET_PROTO
#define _MARKOV_NET_PROTO

enum message {
    MSG_OHAI = '\x01', /**< (S <-> C) welcomes the client, or functions as a ping for the server */
    MSG_PONG, /**< (S <-> C) response to MSG_OHAI, also a null command */
    MSG_SEND_CMD, /**< (S -> C) request to send selection of command */
    MSG_GET_SENTENCE, /**< (C -> S) command to display one sentence */
    MSG_SENTENCE_GENFAILED, /**< (S -> C) tells other party of failure to parse or generate sentence/encoding issues */
    MSG_SENTENCE_LEN, /**< (S <-> C) sending sentence: followed immediately by uint32_t length in Net Byte Order (big-endian), then sentence (UTF-8) */
    MSG_SENTENCE_ACK, /**< (C <-> S) confirms receipt of sentence - server will follow with MSG_SEND_CMD */
    MSG_TERMINATE, /**< (C -> S) command to terminate connection */
    MSG_DB_SAVE,
    MSG_SAVE_ERR,
    MSG_SAVED,
    MSG_SHUTDOWN, /**< (S -> C) signifies a shutdown of the server */
    INT_FAIL, /**< used internally to signify a failed message reciept */
    INT_CLOSED, /**< used internally to signify a closed connection */
    INT_TIMEOUT, /**< used internally to specify a timeout event */
    INT_NULL /**< (internal) messages are initialised to this by default, for debugging */
};

extern void *net_handler_tfunc(void *fda);
extern void *net_fail(int fd);
extern int send_all(int fd, const void *buf, size_t length);
extern bool net_send_msg(int fd, enum message msg);
extern size_t net_recv_timed(int fd, void *buf, size_t size, int timeout);
extern char net_recv_msg_timed(int fd, int tmout);
extern char net_recv_msg(int fd);
extern size_t net_recv(int fd, void *buf, size_t size);
extern char net_recv_msg_or_ping(int fd);
extern wchar_t *net_recv_sentence(int fd);
extern bool net_send_sentence(int fd, const wchar_t *sentence);
#endif

