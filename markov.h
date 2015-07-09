#ifndef _MARKOV
#define _MARKOV
#include <stdio.h>
#define DB_START_SIZE 100 /**< Amount of keys the database should start with **/
#define DB_REFILL_SIZE 10 /**< Amount of keys the database should top itself up with **/

/**
 * Key-value node.
 */
struct kv_node {
    struct kv_node *next; /**< next kv_node for this key */
    wchar_t *key; /**< key */
    wchar_t *val; /**< value */
};

/**
 * Value-next node. Holds the value, and a pointer to the kv_node which
 * has this value as its key.
 */
struct vn_node {
    struct kv_node *next; /**< which kv_node this val points to */
    wchar_t *val; /**< value */
};

/** Database structure */
struct database {
    struct kv_list *objs; /**< kv_list of all kv_node objects */
    struct kv_list *sses; /**< kv_list of all sentence starters */
};
extern struct database *db_init(void);
/**
 * Used to store a set of kv_node objects dynamically.
 * (used to be a database :()
 */
struct kv_list {
    struct kv_node **keys; /**< Array of kv_node objects */
    int used; /**< Objects stored */
    int size; /**< Array size (in kv_node objects) */
};

extern struct kv_list *kvl_init(void);
extern struct kv_node *kvl_store(struct kv_node *obj, struct kv_list *kvl);

struct database *markov_database; /**< global database object */

struct kv_node *search_for_key(wchar_t *key);
struct kv_node *search_for_ss(wchar_t *key);
struct kv_node *store_kv(wchar_t *key, wchar_t *val, int ss);
extern wchar_t *generate_sentence();

extern int read_input(FILE *fp);

int load(void);
int save(void);

#endif
