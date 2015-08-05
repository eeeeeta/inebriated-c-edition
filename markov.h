#ifndef _MARKOV
#define _MARKOV
#include "vbuf.h"
/**
 * \brief Database node
 */
struct kv_node {
    struct kv_node *next; /**< linked list: next kv_node for this key */
    wchar_t *key; /**< key */
    unsigned long *hash; /**< pointer to key hash */
    struct kv_node *vptr; /**< pointer to value */
#ifdef I_HAVE_A_VERY_BIG_NODE
    int16_t score; /**< (if origin node) amount of nodes in linked list, (if not) -1 */
#else
    int8_t score;
#endif
};
typedef struct kv_node DBN;

/** \brief Database structure */
struct database {
    DPA *objs; /**< DPA of all kv_node objects */
    DPA *sses; /**< DPA of all sentence starters */
};
extern struct database *db_init(void);

struct database *markov_database; /**< global database object */

struct kv_node *search_for_key(wchar_t *key);
struct kv_node *search_for_ss(wchar_t *key);
extern DBN *store_kv(wchar_t *k, wchar_t *v, bool ss);
extern wchar_t *generate_sentence();

extern int read_input(FILE *fp, bool is_sentence);
extern bool read_data(wchar_t *text, bool is_sentence);

extern int load(char *filename);
extern bool save(char *filename);

extern char *DB_FILENAME;
extern char *NET_PORT;

extern void kv_free(struct kv_node *kv);
extern void kv_free_DPA(DPA *dpa);
extern void vn_free_DPA(DPA *dpa);
#endif
