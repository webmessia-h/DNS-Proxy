#ifndef DNS_CLIENT
#define DNS_CLIENT

#include "../include/hash.h"
#include "config.h"
#include "include.h"

typedef void (*res_callback)(void *data, char *response, size_t res_len,
                             uint16_t tx_id);

typedef struct {
  struct sockaddr_storage addr;
  socklen_t addrlen;
  int socket;
  ev_io observer;
} resolver;

typedef struct dns_client {
  struct ev_loop *loop;
  void *cb_data;
  res_callback cb;
  int sockfd;
  resolver resolvers[RESOLVERS];
  ev_io observer;
  ev_timer timeout_observer;
  int timeout_ms;
  char query[REQUEST_MAX];
  size_t query_len;
} dns_client;

void dns_client_init(dns_client *clt, struct ev_loop *loop, res_callback cb,
                     void *data, HashMap *map);

void dns_client_send_request(dns_client *clt, const char *dns_req,
                             size_t req_len, uint16_t tx_id);

void dns_client_cleanup(dns_client *clt);
#endif // DNS_CLIENT
