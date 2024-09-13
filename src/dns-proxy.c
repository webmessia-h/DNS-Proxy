#include "dns-proxy.h"
#include "config.h" /* Main configuration file */
#include "hash.h"
#include "log.h"

inline void proxy_init(dns_proxy *restrict prx, dns_client *restrict clt,
                       dns_server *restrict srv, struct ev_loop *loop);

static void proxy_start(dns_proxy *restrict prx);

void proxy_stop(const dns_proxy *restrict prx);

void proxy_handle_request(dns_proxy *restrict prx, void *restrict data,
                          const struct sockaddr *addr, const uint16_t tx_id,
                          char *restrict dns_req, const size_t dns_req_len);

void proxy_handle_response(dns_proxy *restrict prx, void *restrict data,
                           const struct sockaddr *addr, const uint16_t tx_id,
                           const char *restrict dns_res,
                           const size_t dns_res_len);

static void proxy_handle_timeout(EV_P_ ev_timer *w, int revents);

static inline bool validate_request(const dns_header *restrict header,
                                    const uint16_t tx_id,
                                    const char *restrict dns_req,
                                    const size_t dns_req_len,
                                    char *restrict domain);

static inline transaction_info *
create_transaction_info(const struct sockaddr *clt, const uint16_t tx_id);

static inline char *create_blacklisted_response(const char *dns_req,
                                                const size_t dns_req_len);

static inline void handle_blacklisted(const dns_proxy *prx,
                                      const struct sockaddr *addr,
                                      const uint16_t tx_id, char *dns_req,
                                      const size_t dns_req_len,
                                      const char *domain);

static inline void send_blacklisted_response(const dns_proxy *prx,
                                             const struct sockaddr *addr,
                                             const uint16_t tx_id,
                                             const char *dns_req,
                                             const size_t dns_req_len);

static inline void forward_request(const dns_proxy *prx,
                                   const struct sockaddr *addr,
                                   const uint16_t tx_id, const char *dns_req,
                                   const size_t dns_req_len);
/*---*/
// IMPLEMENTATION

void proxy_init(dns_proxy *prx, dns_client *clt, dns_server *srv,
                struct ev_loop *loop) {
  LOG_DEBUG("proxy_init(prx ptr: %p, clt ptr: %p, srv ptr: %p, loop ptr: %p)\n",
            prx, clt, srv, loop);
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
static void proxy_start(dns_proxy *restrict prx) {
  LOG_DEBUG("proxy_start(prx ptr: %p)\n", prx);
  ev_timer_init(&prx->client->timeout_observer, proxy_handle_timeout, 0,
                prx->timeout_ms / 1000.0);
  ev_timer_start(prx->loop, &prx->client->timeout_observer);
  ev_run(prx->loop, 0);
}

void proxy_stop(const dns_proxy *restrict prx) {
  LOG_DEBUG("proxy_stop(prx ptr: %p)\n", prx);
  ev_break(prx->loop, EVBREAK_ALL);
}

/**
 * @brief Handle a DNS request in the proxy
 *
 * @param prx Pointer to the dns_proxy structure
 * @param addr Address of the client
 * @param tx_id Transaction ID of the request
 * @param dns_req Buffer containing the DNS request
 * @param dns_req_len Length of the DNS request buffer
 */
void proxy_handle_request(dns_proxy *restrict prx, void *restrict data,
                          const struct sockaddr *addr, const uint16_t tx_id,
                          char *restrict dns_req, const size_t dns_req_len) {
  LOG_DEBUG("proxy_handle_request(prx ptr: %p, data ptr: %p, addr ptr: %p, "
            "tx_id: %u, "
            "dns_req ptr: %p, dns_req_len: %zu)\n",
            prx, data, addr, tx_id, dns_req, dns_req_len);
  // WARN:
  prx = (dns_proxy *)data;

  dns_header *header = (dns_header *)dns_req;
  char domain[MAX_DOMAIN_LENGTH];

  if (!validate_request(header, tx_id, dns_req, dns_req_len, domain)) {
    LOG_ERROR("Failed to validate request, tx_id: #%du\n", tx_id);
    free(dns_req);
    return;
  }

  if (is_blacklisted(domain)) {
    handle_blacklisted(prx, addr, tx_id, dns_req, dns_req_len, domain);
  } else {
    forward_request(prx, addr, tx_id, dns_req, dns_req_len);
  }
  free(dns_req);
}

void proxy_handle_response(dns_proxy *prx, void *data,
                           const struct sockaddr *addr, const uint16_t tx_id,
                           const char *dns_res, const size_t dns_res_len) {
  LOG_DEBUG("proxy_handle_response(prx ptr: %p, data ptr: %p, addr ptr: %p, "
            "tx_id: %u, "
            "dns_res ptr: %p, dns_res_len: %zu)\n",
            prx, data, addr, tx_id, dns_res, dns_res_len);
  // WARN:
  prx = (dns_proxy *)data;

  transaction_info *current = find_transaction(tx_id);
  if (current != NULL) {
    server_send_response(prx->server, (struct sockaddr *)&current->client_addr,
                         dns_res, dns_res_len);
    delete_transaction(tx_id);
  } else {
    LOG_ERROR("No transaction_info for tx_id #%du\n", tx_id);
  }
  free((void *)dns_res);
}

// unused TODO: consider using
static void proxy_handle_timeout(EV_P_ ev_timer *w, int revents) {
  LOG_DEBUG("proxy_handle_timeout(loop ptr: %p, w ptr: %p, revents: %d)\n",
            loop, w, revents);
  dns_proxy *prx = (dns_proxy *)w->data;
  LOG_ERROR("Timeout occurred\n");
}

static inline bool validate_request(const dns_header *header,
                                    const uint16_t tx_id, const char *dns_req,
                                    const size_t dns_req_len, char *domain) {
  LOG_DEBUG("validate_request(header ptr: %p, tx_id: %u, dns_req ptr: %p, "
            "dns_req_len: %zu, domain ptr: %p)\n",
            header, tx_id, dns_req, dns_req_len, domain);
  if (ntohs(header->qd_count) == 0) {
    LOG_ERROR("No questions in the request for tx_id #%du\n", tx_id);
    return false;
  }

  size_t query_offset = sizeof(*header);
  if (!parse_domain_name(dns_req, dns_req_len, query_offset, domain,
                         MAX_DOMAIN_LENGTH)) {
    LOG_ERROR("Failed to parse domain name for tx_id #%du\n", tx_id);
    return false;
  }

  return true;
}

static inline transaction_info *
create_transaction_info(const struct sockaddr *clt, const uint16_t tx_id) {
  LOG_DEBUG("create_transaction_info(clt ptr: %p, tx_id: %u)\n", clt, tx_id);
  struct transaction_info *tx_info =
      malloc(sizeof(struct transaction_info)); /* will be free'd upon deletion
                                                  of transaction_entry in
                                                  proxy_handle_respose */
  if (tx_info == NULL) {
    LOG_ERROR("Failed transaction_info allocation for tx_id #%du\n", tx_id);
    return NULL;
  }
  tx_info->client_addr = *clt;
  tx_info->original_tx_id = tx_id;
  tx_info->client_addr_len = sizeof(*clt);
  return tx_info;
}

static inline char *create_blacklisted_response(const char *dns_req,
                                                const size_t dns_req_len) {
  LOG_DEBUG("create_blacklisted_response(dns_req ptr: %p, dns_req_len: %zu)\n",
            dns_req, dns_req_len);
  dns_header *header = (dns_header *)dns_req;
  size_t query_offset = sizeof(*header);
  char *resp = (char *)malloc(dns_req_len);
  if (resp == NULL) {
    LOG_ERROR("Memory allocation failed.\n");
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

static inline void handle_blacklisted(const dns_proxy *prx,
                                      const struct sockaddr *addr,
                                      const uint16_t tx_id, char *dns_req,
                                      const size_t dns_req_len,
                                      const char *domain) {
  LOG_DEBUG("handle_blacklisted(prx ptr: %p, addr ptr: %p, tx_id: %u, "
            "dns_req ptr: %p, dns_req_len: %zu, domain: %s)\n",
            prx, addr, tx_id, dns_req, dns_req_len, domain);
#if REDIRECT == 1
  handle_redirect(prx, addr, tx_id, dns_req, dns_req_len, domain);
#else
  send_blacklisted_response(prx, addr, tx_id, dns_req, dns_req_len);
#endif
}

#if REDIRECT == 1
static inline void handle_redirect(dns_proxy *prx, struct sockaddr *addr,
                                   uint16_t tx_id, char *dns_req,
                                   size_t dns_req_len, const char *domain) {
  LOG_DEBUG("send_blacklisted_response(prx ptr: %p, addr ptr: %p, tx_id: %u, "
            "dns_req ptr: %p, dns_req_len: %zu, domain: %p)\n",
            prx, addr, tx_id, dns_req, dns_req_len, domain);
  size_t redir_len = dns_req_len - (strlen(domain) + 2) + strlen(redirect_to);
  char *redir = (char *)malloc(redir_len);
  if (redir == NULL) {
    LOG_ERROR("Memory allocation failed for tx_id #%du\n", tx_id);
    return;
  }
  // TODO: implement
  create_redirect_packet(redir, dns_req, dns_req_len, domain);

  struct transaction_info *tx_info = create_transaction_info(addr, tx_id);
  add_transaction_entry(tx_info);

  client_send_request(prx->client, redir, dns_req_len, tx_id);
  free(redir);
}
#else
static inline void send_blacklisted_response(const dns_proxy *prx,
                                             const struct sockaddr *addr,
                                             const uint16_t tx_id,
                                             const char *dns_req,
                                             const size_t dns_req_len) {
  LOG_DEBUG("send_blacklisted_response(prx ptr: %p, addr ptr: %p, tx_id: %u, "
            "dns_req ptr: %p, dns_req_len: %zu)\n",
            prx, addr, tx_id, dns_req, dns_req_len);
  char *resp = create_blacklisted_response(dns_req, dns_req_len);
  if (resp == NULL) {
    LOG_ERROR("Failed to create blacklisted response for tx_id #%du\n", tx_id);
    return;
  }

  server_send_response(prx->server, addr, resp, dns_req_len);
  free(resp);
}
#endif

static inline void forward_request(const dns_proxy *restrict prx,
                                   const struct sockaddr *addr,
                                   const uint16_t tx_id,
                                   const char *restrict dns_req,
                                   const size_t dns_req_len) {
  LOG_DEBUG("forward_request(prx ptr: %p, addr ptr: %p, tx_id: %u, "
            "dns_req ptr: %p, dns_req_len: %zu)\n",
            prx, addr, tx_id, dns_req, dns_req_len);
  struct transaction_info *tx_info = create_transaction_info(addr, tx_id);
  add_transaction_entry(tx_info);
  client_send_request(prx->client, dns_req, dns_req_len, tx_id);
}
