#ifndef DNS_PROXY
#define DNS_PROXY

#include "dns-client.h"
#include "dns-server.h"
#include "include.h"

typedef struct dns_proxy {
  struct ev_loop *loop;
  dns_client *client;
  dns_server *server;
  int timeout_ms;
} dns_proxy;

/* Initialize self with:
 * dns_server struct pointer
 * dns_client strcut pointer
 * structs must be created and initialized with dns_***_init() fn call
 */
void proxy_init(dns_proxy *prx, dns_client *clt, dns_server *srv,
                struct ev_loop *loop);
/* Checks whether domain is in the blacklist and forwards upstream or returns
 * response defined in "../config.h"
 */
void proxy_handle_request(struct dns_proxy *prx, struct sockaddr *addr,
                          uint16_t tx_id, char *dns_req, size_t dns_req_len);

void proxy_stop(dns_proxy *proxy);
void dns_proxy_handle_request(dns_proxy *proxy, const char *request,
                              char *response, int response_size,
                              uint16_t tx_id);

#endif // DNS_PROXY
