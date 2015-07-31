#ifndef _MARKOV
#define _MARKOV
#include "vbuf.h"
/**
 * \brief Key-value node.
 */
struct kv_node {
    struct kv_node *next; /**< next kv_node for this key */
    wchar_t *key; /**< key */
    wchar_t *val; /**< value */
    int score; /**< how many values this key has */
};

/**
 * \brief Value-next node.
 *
 * Holds the value, and a pointer to the kv_node which
 * has this value as its key.
 */
struct vn_node {
    struct kv_node *next; /**< which kv_node this val points to */
    wchar_t *val; /**< value */
    int score; /** how many values the kv_node this val points to has */
};

/** \brief Database structure */
struct database {
    DPA *objs; /**< DPA of all kv_node objects */
    DPA *sses; /**< DPA of all sentence starters */
};
extern struct database *db_init(void);

struct database *markov_database; /**< global database object */

struct kv_node *search_for_key(wchar_t *key);
struct kv_node *search_for_ss(wchar_t *key);
struct kv_node *store_kv(wchar_t *key, wchar_t *val, int ss);
extern wchar_t *generate_sentence();

extern int read_input(FILE *fp, bool is_sentence);
extern bool read_data(wchar_t *text, bool is_sentence);

extern int load(char *filename);
extern bool save(char *filename);

extern char *DB_FILENAME;
extern char *NET_PORT;

#endif
