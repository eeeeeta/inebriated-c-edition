#ifndef _MARKOV
#define _MARKOV
#include <stdio.h>
#define MAXLEN 130 /* maximum length of two word pairs */
#define DBSIZE 100
#define MAXVALS 100
#define MAXWORDS 100
#define DB_START_SIZE 100 /**< Amount of keys the database should start with **/
#define DB_REFILL_SIZE 10 /**< Amount of keys the database should top itself up with **/


/**
 * Key-value node.
 */
struct kv_node {
    struct kv_node *next; /**< next kv_node for this key */
    char *key; /**< key */
    char *val; /**< value */
    unsigned int starter; /**< is this key a sentence starter? 0 if true */
};

/**
 * Value-next node. Holds the value, and a pointer to the kv_node which
 * has this value as its key.
 */
struct vn_node {
    struct kv_node *next; /**< which kv_node this val points to */
    char *val; /**< value */
};

/**
 * Database object. Used to store a set of kv_node objects dynamically.
 */
typedef struct {
    struct kv_node **keys; /**< Array of kv_node objects */
    int used; /**< Objects stored */
    int size; /**< Array size (in kv_node objects) */
} Database;

extern Database *db_init(void);
extern struct kv_node *db_store(struct kv_node *obj, Database *db);

Database *markov_database; /**< global Database object */

struct kv_node *search_for_key(char *key);
struct kv_node *store_kv(char *key, char *val);
extern char *generate_sentence();

extern void read_input(FILE *fp);

int load(void);
int save(void);

#endif
