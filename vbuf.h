#ifndef _MARKOV_VBUF
#define _MARKOV_VBUF
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
extern void varstr_obliterate(struct varstr *vs);
#endif
