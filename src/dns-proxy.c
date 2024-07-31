#include "../include/dns-proxy.h"

static void proxy_process_request(dns_server *srv, void *cb_data,
                                  struct sockaddr *addr, uint16_t tx_id,
                                  char *dns_req, size_t dns_req_len);

static void proxy_process_response(void *data, struct sockaddr *addr,
                                   char *response, size_t response_len,
                                   uint16_t tx_id);

static void proxy_handle_timeout(EV_P_ ev_timer *w, int revents);

// outline of a timeout function (unused)
static void proxy_start(dns_proxy *prx) {
  ev_timer_init(&prx->client->timeout_observer, proxy_handle_timeout, 0,
                prx->timeout_ms / 1000.0);
  ev_timer_start(prx->loop, &prx->client->timeout_observer);
  ev_run(prx->loop, 0);
}

void proxy_init(dns_proxy *prx, dns_client *clt, dns_server *srv,
                struct ev_loop *loop) {
  prx->client = clt;
  prx->server = srv;

  prx->loop = loop;

  srv->cb = proxy_process_request;
  srv->cb_data = prx;

  clt->cb = proxy_process_response;
  clt->cb_data = prx;
}

void proxy_stop(dns_proxy *prx) { ev_break(prx->loop, EVBREAK_ALL); }

void proxy_handle_request(struct dns_proxy *prx, struct sockaddr *addr,
                          uint16_t tx_id, char *dns_req, size_t dns_req_len) {
  // Assuming the DNS header is at the start of buffer (dns_req)
  dns_header *header = (dns_header *)dns_req;

  if (ntohs(header->qd_count) == 0) {
    fprintf(stderr, "No questions in the request. id:%d\n", tx_id);
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

  if (is_blacklisted(domain)) {
#if REDIRECT == 1
    // TODO : implement redirection
#else
    header->rcode = *(uint8_t *)prx->server->cb_data;
    header->qr = 1; // This is the response
    header->rd = 0; // Not recursion desired
    server_send_response(prx->server, addr, dns_req, dns_req_len);
    free(dns_req);
    return;
#endif
  } else {
    /* @brief
     *  insert transaction_info into hashmap in order to know endpoint receiver
     */
    struct transaction_info *tx_info = malloc(
        sizeof(struct transaction_info)); /* will be freed upon deletion of
                                             entry in proxy_handle_respose */
    tx_info->client_addr = *addr;
    tx_info->original_tx_id = tx_id;
    tx_info->client_addr_len = sizeof(*addr);
    add_transaction_entry(tx_info);
    client_send_request(prx->client, dns_req, dns_req_len, tx_id);
    free(dns_req);
    return;
  }
}

void proxy_handle_response(dns_proxy *prx, struct sockaddr *addr, char *dns_res,
                           size_t response_size, uint16_t tx_id) {
  // Assuming the DNS header is at the start of buffer (dns_req)
  dns_header *header = (dns_header *)dns_res;

  if (ntohs(header->qd_count) == 0) {
    fprintf(stderr, "No questions in the request. id:%d\n", tx_id);
    free(dns_res);
    return;
  }
  transaction_info *current = find_transaction(tx_id);
  server_send_response(prx->server, (struct sockaddr *)&current->client_addr,
                       dns_res, response_size);
  delete_transaction(tx_id);
}

static void proxy_process_request(dns_server *srv, void *cb_data,
                                  struct sockaddr *addr, uint16_t tx_id,
                                  char *dns_req, size_t dns_req_len) {
  dns_proxy *prx = (dns_proxy *)cb_data;
  proxy_handle_request(prx, addr, tx_id, dns_req, dns_req_len);
}

static void proxy_process_response(void *cb_data, struct sockaddr *addr,
                                   char *response, size_t response_size,
                                   uint16_t tx_id) {
  dns_proxy *prx = (dns_proxy *)cb_data;
  proxy_handle_response(prx, addr, response, response_size, tx_id);
}

static void proxy_handle_timeout(EV_P_ ev_timer *w, int revents) {
  dns_proxy *prx = (dns_proxy *)w->data;
  // TODO Handle timeout-related tasks
  printf("Timeout occurred\n");
}
