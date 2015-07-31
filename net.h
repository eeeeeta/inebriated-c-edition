#ifndef _MARKOV_NET
#define _MARKOV_NET

#define NET_BACKLOG 3 /**< How many connections to tell listen() to keep a backlog for */
#define NET_PING_INTERVAL 45000 /**< time (in ms) between pings */
#define NET_TIMEOUT 4000 /**< time (in ms) to wait for a reply before giving up */

extern int net_init();
extern int net_listen(int fd);
#endif
