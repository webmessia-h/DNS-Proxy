#include "dns-proxy.h"
#include "config.h" /* Main configuration file */
#include "dns-server.h"
#include "hash.h"

void proxy_init(dns_proxy *prx, dns_client *clt, dns_server *srv,
                struct ev_loop *loop);

static void proxy_start(dns_proxy *prx);

void proxy_stop(dns_proxy *prx);

void proxy_handle_request(struct dns_proxy *prx, struct sockaddr *addr,
                          uint16_t tx_id, char *dns_req, size_t dns_req_len);

void proxy_handle_response(dns_proxy *prx, struct sockaddr *addr, char *dns_res,
                           size_t response_size, uint16_t tx_id);

static void proxy_handle_timeout(EV_P_ ev_timer *w, int revents);

static bool validate_request(dns_header *header, uint16_t tx_id, char *dns_req,
                             size_t dns_req_len, char *domain);

static transaction_info *create_transaction_info(struct sockaddr *clt,
                                                 uint16_t tx_id);

static char *create_blacklisted_response(char *dns_req, size_t dns_req_len);

static void handle_blacklisted(struct dns_proxy *prx, struct sockaddr *addr,
                               uint16_t tx_id, char *dns_req,
                               size_t dns_req_len, const char *domain);

static void send_blacklisted_response(struct dns_proxy *prx,
                                      struct sockaddr *addr, uint16_t tx_id,
                                      char *dns_req, size_t dns_req_len);

static void forward_request(struct dns_proxy *prx, struct sockaddr *addr,
                            uint16_t tx_id, char *dns_req, size_t dns_req_len);
/*---*/

void proxy_init(dns_proxy *prx, dns_client *clt, dns_server *srv,
                struct ev_loop *loop) {
  prx->client = clt;
  prx->server = srv;

  prx->loop = loop;

  srv->cb = proxy_handle_request;
  srv->cb_data = prx;

  clt->cb = proxy_handle_response;
  clt->cb_data = prx;
}

/* outline of a timeout function (unused)
 * TODO consider using it
 */
static void proxy_start(dns_proxy *prx) {
  ev_timer_init(&prx->client->timeout_observer, proxy_handle_timeout, 0,
                prx->timeout_ms / 1000.0);
  ev_timer_start(prx->loop, &prx->client->timeout_observer);
  ev_run(prx->loop, 0);
}

void proxy_stop(dns_proxy *prx) { ev_break(prx->loop, EVBREAK_ALL); }

/**
 * @brief Handle a DNS request in the proxy
 *
 * @param prx Pointer to the dns_proxy structure
 * @param addr Address of the client
 * @param tx_id Transaction ID of the request
 * @param dns_req Buffer containing the DNS request
 * @param dns_req_len Length of the DNS request buffer
 */
void proxy_handle_request(struct dns_proxy *prx, struct sockaddr *addr,
                          uint16_t tx_id, char *dns_req, size_t dns_req_len) {
  dns_header *header = (dns_header *)dns_req;
  char domain[MAX_DOMAIN_LENGTH];

  if (!validate_request(header, tx_id, dns_req, dns_req_len, domain)) {
    free(dns_req);
    return;
  }

  if (is_blacklisted(domain)) {
    handle_blacklisted(prx, addr, tx_id, dns_req, dns_req_len, domain);
  } else {
    forward_request(prx, addr, tx_id, dns_req, dns_req_len);
  }
}

void proxy_handle_response(dns_proxy *prx, struct sockaddr *addr, char *dns_res,
                           size_t response_size, uint16_t tx_id) {
  dns_header *header = (dns_header *)dns_res;
  if (header != NULL)
    if (ntohs(header->qd_count) == 0) {
      fprintf(stderr, "No questions in the request. id:%d\n", tx_id);
      free(dns_res);
      return;
    }

  transaction_info *current = find_transaction(tx_id);
  if (current != NULL) {
    server_send_response(prx->server, (struct sockaddr *)&current->client_addr,
                         dns_res, response_size);
    delete_transaction(tx_id);
  } else if (log) {
    fprintf(stderr, "No transaction_info for tx_id #%d\n", tx_id);
  }
  free(dns_res);
}

// unused TODO: consider using
static void proxy_handle_timeout(EV_P_ ev_timer *w, int revents) {
  dns_proxy *prx = (dns_proxy *)w->data;
  printf("Timeout occurred\n");
}

static bool validate_request(dns_header *header, uint16_t tx_id, char *dns_req,
                             size_t dns_req_len, char *domain) {
  if (header == NULL || ntohs(header->qd_count) == 0) {
    fprintf(stderr, "No questions in the request. id:%d\n", tx_id);
    return false;
  }

  size_t query_offset = sizeof(*header);
  if (!parse_domain_name(dns_req, dns_req_len, query_offset, domain,
                         MAX_DOMAIN_LENGTH)) {
    fprintf(stderr, "Failed to parse domain name. id:%d\n", tx_id);
    return false;
  }

  return true;
}

static transaction_info *create_transaction_info(struct sockaddr *clt,
                                                 uint16_t tx_id) {
  struct transaction_info *tx_info =
      malloc(sizeof(struct transaction_info)); /* will be free'd upon deletion
                                                  of transaction_entry in
                                                  proxy_handle_respose */
  if (tx_info == NULL) {
    fprintf(stderr, "Failed transaction_info allocation for id:%d", tx_id);
    return NULL;
  }
  tx_info->client_addr = *clt;
  tx_info->original_tx_id = tx_id;
  tx_info->client_addr_len = sizeof(*clt);
  return tx_info;
}

static char *create_blacklisted_response(char *dns_req, size_t dns_req_len) {
  dns_header *header = (dns_header *)dns_req;
  size_t response_len = dns_req_len;
  size_t query_offset = sizeof(*header);
  char *resp = (char *)malloc(response_len);
  if (resp == NULL) {
    fprintf(stderr, "Memory allocation failed.");
    free(dns_req);
    return NULL;
  }

  // Copy the DNS header from the request to the response
  memcpy(resp, dns_req, sizeof(*header));
  dns_header *resp_header = (dns_header *)resp;

  // Copy the question section from the request to the response
  memcpy(resp + query_offset, dns_req + query_offset,
         dns_req_len - query_offset);

  // Modify the DNS header to indicate this is a response and set the RCODE
  resp_header->rcode = BLACKLISTED_RESPONSE;
  resp_header->qr = (uint8_t)1; // This is the response
  resp_header->rd = (uint8_t)0; // Not recursion desired

  return resp;
}

static void handle_blacklisted(struct dns_proxy *prx, struct sockaddr *addr,
                               uint16_t tx_id, char *dns_req,
                               size_t dns_req_len, const char *domain) {
#if REDIRECT == 1
  handle_redirect(prx, addr, tx_id, dns_req, dns_req_len, domain);
#else
  send_blacklisted_response(prx, addr, tx_id, dns_req, dns_req_len);
#endif
  free(dns_req);
}

#if REDIRECT == 1
static void handle_redirect(struct dns_proxy *prx, struct sockaddr *addr,
                            uint16_t tx_id, char *dns_req, size_t dns_req_len,
                            const char *domain) {
  size_t redir_len = dns_req_len - (strlen(domain) + 2) + strlen(redirect_to);
  char *redir = (char *)malloc(redir_len);
  if (dns_redir == NULL) {
    fprintf(stderr, "Memory allocation failed. id:%d\n", tx_id);
    return;
  }

  create_redirect_packet(redir, dns_req, dns_req_len, domain);

  struct transaction_info *tx_info = create_transaction_info(addr, tx_id);
  add_transaction_entry(tx_info);

  client_send_request(prx->client, redir, dns_req_len, tx_id);
  free(redir);
}
#else
static void send_blacklisted_response(struct dns_proxy *prx,
                                      struct sockaddr *addr, uint16_t tx_id,
                                      char *dns_req, size_t dns_req_len) {
  char *resp = create_blacklisted_response(dns_req, dns_req_len);
  if (resp == NULL) {
    fprintf(stderr, "Failed to create blacklisted response. id:%d\n", tx_id);
    return;
  }

  server_send_response(prx->server, addr, resp, dns_req_len);
  free(resp);
}
#endif

static void forward_request(struct dns_proxy *prx, struct sockaddr *addr,
                            uint16_t tx_id, char *dns_req, size_t dns_req_len) {
  struct transaction_info *tx_info = create_transaction_info(addr, tx_id);
  add_transaction_entry(tx_info);
  client_send_request(prx->client, dns_req, dns_req_len, tx_id);
}
