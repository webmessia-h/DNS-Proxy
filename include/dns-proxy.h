#ifndef PROXY_H
#define PROXY_H

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum {
  REQUEST_MAX = 1500 /* A default MTU */
};

// TODO: add wtf else is needed here
typedef struct dns_server_s {
  struct ev_loop *loop;
  void *cb_data;
  int sock;
  socklen_t addrlen;
} dns_server_t;

#endif
