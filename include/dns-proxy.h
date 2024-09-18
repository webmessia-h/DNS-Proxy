#ifndef DNS_PROXY
#define DNS_PROXY

#include "dns-client.h"
#include "dns-server.h"
#include "include.h"

/**
 * @brief Structure representing a DNS proxy.
 *
 * Contains the event loop, client, server, and timeout configuration.
 */
typedef struct dns_proxy {
  struct ev_loop *loop; /**< Event loop used by the proxy. */
  dns_client *client;   /**< Pointer to the DNS client. */
  dns_server *server;   /**< Pointer to the DNS server. */
} dns_proxy;

/**
 * @brief Initializes a DNS proxy.
 *
 * Initializes the `dns_proxy` structure with the provided DNS client, server,
 * and event loop. The `dns_client` and `dns_server` structures must already be
 * initialized using their respective init functions.
 *
 * @param prx Pointer to the dns_proxy structure to initialize.
 * @param clt Pointer to the initialized dns_client structure.
 * @param srv Pointer to the initialized dns_server structure.
 * @param loop Pointer to the libev event loop (ev_loop) structure.
 */
void proxy_init(dns_proxy *restrict prx, dns_client *restrict clt,
                dns_server *restrict srv, struct ev_loop *loop);

/**
 * @brief Handles a DNS request.
 *
 * Checks if the domain is in the blacklist. If not blacklisted, the request is
 * forwarded upstream. If the domain is blacklisted, a pre-defined response from
 * the configuration is returned or IF the redirection flag is set changes
 * the query to a pre-defined domain name
 *
 * @param prx Pointer to the dns_proxy structure.
 * @param addr Pointer to the sockaddr structure containing the client's
 * address.
 * @param tx_id The transaction ID of the DNS request.
 * @param dns_req Pointer to the DNS request packet.
 * @param dns_req_len Length of the DNS request packet.
 */
void proxy_handle_request(void *restrict prx, void *restrict data,
                          const struct sockaddr *addr, const uint16_t tx_id,
                          char *restrict dns_req, const size_t dns_req_len);

/**
 * @brief Handles a DNS response.
 *
 * Checks if the transaction id is present in the
 * hash table, and if so sends the response back to client
 * else logs the error message to stderr
 *
 * @param prx Pointer to the dns_proxy structure.
 * @param addr Pointer to the sockaddr structure containing the client's
 * address.
 * @param tx_id The transaction ID of the DNS request.
 * @param dns_res Pointer to the DNS response packet.
 * @param dns_res_len Length of the DNS response packet.
 */
void proxy_handle_response(void *restrict prx, void *restrict data,
                           const struct sockaddr *addr, const uint16_t tx_id,
                           const char *restrict dns_res,
                           const size_t dns_res_len);

/**
 * @brief Stops the DNS proxy.
 *
 * Terminates the DNS proxy's operation and stops handling new requests.
 *
 * @param proxy Pointer to the dns_proxy structure to stop.
 */
void proxy_stop(const dns_proxy *restrict proxy);

#endif // DNS_PROXY
