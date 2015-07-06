#ifndef MARKOV
#define MARKOV


#define MAXLEN 130 /* maximum length of two word pairs */
#define DBSIZE 100
#define MAXVALS 100

struct kv_node *keys[DBSIZE];
struct kv_node {
    struct kv_node *next; /* next kv_node for this key */
    char *key;
    char *val;
};

struct vn_node {
    struct kv_node *next; /* which kv_node this val points to */
    char *val;
};

struct kv_node *search_for_key(char *key);
int get_vals(struct vn_node *into[], char *key);
int get_ins_pos();
struct kv_node **get_db_ptr(void);
struct kv_node *store_kv(char *key, char *val);

extern void read_input(FILE *fp);
#endif
