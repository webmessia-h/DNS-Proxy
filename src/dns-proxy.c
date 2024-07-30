#include "../include/dns-proxy.h"

static void handle_dns_request_cb(dns_server *srv, void *cb_data, HashMap *map,
                                  struct sockaddr *addr, uint16_t tx_id,
                                  char *dns_req, size_t dns_req_len);

static void handle_dns_response_cb(void *data, char *response,
                                   size_t response_len, uint16_t tx_id);

static void timeout_cb(EV_P_ ev_timer *w, int revents);

void dns_proxy_init(dns_proxy *prx, dns_client *clt, dns_server *srv,
                    struct ev_loop *loop) {
  prx->client = clt;
  prx->server = srv;

  prx->loop = loop;

  srv->cb = handle_dns_request_cb;
  srv->cb_data = prx;

  clt->cb = handle_dns_response_cb;
  clt->cb_data = prx;
}

void dns_proxy_start(dns_proxy *prx) {
  ev_timer_init(&prx->client->timeout_observer, timeout_cb, 0,
                prx->timeout_ms / 1000.0);
  ev_timer_start(prx->loop, &prx->client->timeout_observer);
  ev_run(prx->loop, 0);
}

void dns_proxy_stop(dns_proxy *prx) { ev_break(prx->loop, EVBREAK_ALL); }

void handle_dns_request(struct dns_proxy *prx, struct sockaddr *addr,
                        uint16_t tx_id, char *dns_req, size_t dns_req_len) {
  // Assuming the DNS header is at the start of buffer (dns_req)
  dns_header *header = (dns_header *)dns_req;

  if (ntohs(header->qd_count) == 0) {
    printf("No questions in the request. id:%d\n", tx_id);
    free(dns_req);
    return;
  }

  // Offset to the query section (DNS header is 12 bytes)
  size_t query_offset = sizeof(*header);
  char domain[MAX_DOMAIN_LENGTH];

  if (!parse_domain_name(dns_req, dns_req_len, query_offset, domain,
                         sizeof(domain))) {
    fprintf(stderr, "Failed to parse domain name. id:%d\n", tx_id);
    free(dns_req);
    return;
  }

  if (is_blacklisted(domain, prx->server->blacklist)) {
    header->rcode = *(uint8_t *)prx->server->cb_data;
    header->qr = 1; // This is the response
    header->ans_count = 0;
    dns_server_respond(prx->server, addr, dns_req, dns_req_len);
    free(dns_req);
    return;
  } else {
    dns_client_send_request(prx->client, dns_req, dns_req_len, tx_id);
    free(dns_req);
    return;
  }
}

void dns_proxy_handle_response(dns_proxy *prx, char *response,
                               size_t response_size, uint16_t tx_id) {
  // Process the response if needed before sending it back to the client
  dns_server_respond(prx->server, NULL, response, response_size);
}

static void handle_dns_request_cb(dns_server *srv, void *cb_data, HashMap *map,
                                  struct sockaddr *addr, uint16_t tx_id,
                                  char *dns_req, size_t dns_req_len) {
  dns_proxy *prx = (dns_proxy *)cb_data;
  handle_dns_request(prx, addr, tx_id, dns_req, dns_req_len);
}

static void handle_dns_response_cb(void *cb_data, char *response,
                                   size_t response_size, uint16_t tx_id) {
  dns_proxy *prx = (dns_proxy *)cb_data;
  dns_proxy_handle_response(prx, response, response_size, tx_id);
}

static void timeout_cb(EV_P_ ev_timer *w, int revents) {
  dns_proxy *prx = (dns_proxy *)w->data;
  // Handle timeout-related tasks
  printf("Timeout occurred\n");
}
