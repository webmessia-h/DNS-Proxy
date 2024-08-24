#ifndef DNS_CLIENT
#define DNS_CLIENT

#include "config.h"
#include "hash.h"
#include "include.h"

typedef void (*res_callback)(void *data, struct sockaddr *addr, char *response,
                             size_t res_len, uint16_t tx_id);

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
  TransactionHashEntry *transactions;
  ev_io observer;
  ev_timer timeout_observer;
  int timeout_ms;
} dns_client;

void client_init(dns_client *clt, struct ev_loop *loop, res_callback cb,
                 void *data, TransactionHashEntry *transactions);

void client_send_request(dns_client *clt, const char *dns_req, size_t req_len,
                         uint16_t tx_id);

void client_cleanup(dns_client *clt);
#endif // DNS_CLIENT
