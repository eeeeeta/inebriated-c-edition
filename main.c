/*
 * markov chains chatbot, main file
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "markov.h"

int main() {
    printf("shitty thing v1, test edition\n");
    char tk[] = "test";
    char tv[] = "testval";
    char tv2[] = "tv2";
    char tv3[] = "tv3";
    char tv4[] = "tv4";
    struct kv_node *node;
    printf("ok here we go, time to try and store some shit\n");
    node = store_kv(&tk, &tv);
    if (node == NULL) {
        printf("well shit, we got a NULL\n");
        exit(1);
    }
    printf("ok yay\n");
    printf("ok here we go, time to try and search for that shit\n");
    struct kv_node *retnode;
    retnode = search_for_key(&tk);
    if (retnode == NULL) {
        printf("well shit, search_for_key returned NULL\n");
        exit(1);
    }
    if (retnode != node) {
        printf("well shit, addresses differ:\n");
        printf("stored: %p ", node);
        printf("ret'd: %p\n", retnode);
        exit(1);
    }
    printf("ok yay\n");
    printf("ok here we go, time to try and search for shit we didn't put in\n");
    retnode = search_for_key(&tv);
    if (retnode != NULL) {
        printf("wtf? it gave us back something!\n");
        exit(1);
    }
    printf("ok yay\n");
    printf("trying to store two vals for one key...");
    node = store_kv(&tk, &tv2);
    printf("yay!\n");
    printf("trying to get two vals back...");
    struct vn_node *vals[100];
    int vals_retd;
    vals_retd = get_vals(vals, &tk);
    printf("yay (vals ret'd: %d)\n", vals_retd);
    printf("storing shit & then walking...");
    store_kv(&tv2, &tv3);
    store_kv(&tv3, &tv4);
    store_kv(&tv, &tv4);
    printf("walking now\n");
    dbg_walk(&tk);
}
