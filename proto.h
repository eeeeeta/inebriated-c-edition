#ifndef _MARKOV_NET_PROTO
#define _MARKOV_NET_PROTO

enum message {
    MSG_OHAI = '\x01', /**< (S <-> C) welcomes the client, or functions as a ping for the server */
    MSG_PONG, /**< (S <-> C) response to MSG_OHAI, also a null command */
    MSG_SEND_CMD, /**< (S -> C) request to send selection of command */
    MSG_GET_SENTENCE, /**< (C -> S) command to display one sentence */
    MSG_SENTENCE_GENFAILED, /**< (S -> C) tells other party of failure to generate sentence/encoding issues (followed with MSG_SEND_CMD) */
    MSG_SENTENCE_LEN, /**< (S <-> C) followed immediately by uint32_t length in Net Byte Order (big-endian), then sentence (UTF-8) */
    MSG_SENTENCE_ACK, /**< (C <-> S) confirms receipt of sentence - server will follow with MSG_SEND_CMD */
    MSG_TERMINATE, /**< (C -> S) command to terminate connection */
    MSG_REQ_SEND_SENTENCE, /**< (C -> S) command to input a new sentence */
    MSG_SENDNOW, /**< (S -> C) request for the client to begin sentence transmission */
    MSG_DB_SAVE,
    MSG_SAVED,
    MSG_DB_ERR,
    MSG_SHUTDOWN, /**< (S -> C) signifies a shutdown of the server */
    INT_FAIL, /**< used internally to signify a failed message reciept */
    INT_CLOSED, /**< used internally to signify a closed connection */
    INT_TIMEOUT, /**< used internally to specify a timeout event */
    INT_NULL /**< (internal) messages are initialised to this by default, for debugging */
};

extern void *net_handler_tfunc(void *fda);

#endif

