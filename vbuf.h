#ifndef _MARKOV_VBUF
#define _MARKOV_VBUF
#define VARSTR_START_SIZE 15 /**< Default variable string start length */
#define VARSTR_REFILL_SIZE 5 /**< Default variable string refill length */
#define VNLIST_START_SIZE 3 /**< Default variable vn_node list start length */
#define VNLIST_REFILL_SIZE 2 /**< Default variable vn_node list refill length */
/**
 * Expandable variable-length string.
 */
struct varstr {
    char *str; /**< string */
    int used; /**< chars used (not including null) */
    int size; /**< size of string (including null) */
};

extern struct varstr *varstr_init(void);
extern struct varstr *varstr_cat(struct varstr *vs, char *str);
extern struct varstr *varstr_ncat(struct varstr *vs, char *str, size_t count);
extern struct varstr *varstr_pushc(struct varstr *vs, char c);
extern char *varstr_pack(struct varstr *vs);

/**
 * Expandable variable-length list of value-next nodes (vn_node)
 */
struct vn_list {
    struct vn_node **list; /**< list of vn_node nodes */
    int used; /**< nodes stored */
    int size; /**< size of array */
};

extern struct vn_list *vnlist_init(void);
extern struct vn_list *vnlist_add(struct vn_list *vnl, struct vn_node *vn);
#endif
