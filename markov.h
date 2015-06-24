#ifndef MARKOV
#define MARKOV

extern struct kv_node {
    struct kv_node *next; /* next kv_node for this key */
    char *key;
    char *val;
};

extern struct vn_node {
    struct kv_node *next; /* which kv_node this val points to */
    char *val;
};

struct kv_node *search_for_key(char *key);
int get_vals(struct vn_node *into[], char *key);
int get_ins_pos();
struct kv_node *store_kv(char *key, char *val);

#endif
