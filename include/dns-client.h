#ifndef DNS_CLIENT
#define DNS_CLIENT

#include "../include/hash.h"
#include "config.h"
#include "dns-server.h"
#include <ev.h>
#include <netinet/in.h>
#include <pwd.h>
#include <stdint.h>

typedef void (*response_cb)(void *user_data, const char *response,
                            size_t response_len);

typedef struct {
  struct sockaddr_storage addr;
  socklen_t addrlen;
  int socket;
  ev_io read_observer;
} resolver;

typedef struct dns_client {
  struct ev_loop *loop;
  void *cb_data;
  response_cb cb;
  int sockfd;
  resolver resolvers[RESOLVERS];
  ev_io read_observer;
  ev_timer timeout_observer;
  char query[REQUEST_MAX];
  size_t query_len;
} dns_client;

void dns_client_init(dns_client *client, struct ev_loop *loop, response_cb cb,
                     void *data, HashMap *map);

void dns_client_send_request(dns_client *client, const char *dns_req,
                             size_t req_len);

void dns_client_cleanup(dns_client *client);
#endif // DNS_CLIENT
