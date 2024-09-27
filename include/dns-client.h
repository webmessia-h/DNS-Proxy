#ifndef DNS_CLIENT
#define DNS_CLIENT

#include "hash.h"

struct dns_client;

/**
 * @brief Callback function type for DNS responses
 *
 * @param clt Pointer to the dns_client structure
 * @param data User-defined callback data
 * @param addr Address of the responding DNS server
 * @param response Buffer containing the DNS response
 * @param res_len Length of the response buffer
 * @param tx_id Transaction ID of the DNS query
 */
typedef void (*res_callback)(void *clt, void *data, const struct sockaddr *addr,
                             const uint16_t tx_id,
                             const char *restrict response,
                             const size_t res_len);

/**
 * @brief Structure representing a DNS resolver
 */
typedef struct {
  struct sockaddr_storage addr; /**< Address of the resolver */
  socklen_t addrlen;            /**< Length of the resolver's address */
  int socket;                   /**< Socket file descriptor */
  ev_io observer;               /**< Event loop I/O watcher */
} resolver;

/**
 * @brief Structure representing a DNS client
 */
typedef struct dns_client {
  struct ev_loop *loop;          /**< Event loop */
  void *cb_data;                 /**< User-defined callback data */
  res_callback callback;         /**< Response callback function */
  int sockfd;                    /**< Socket file descriptor */
  resolver resolvers[RESOLVERS]; /**< Array of DNS resolvers */
  transaction_hash_entry
      *transactions;         /**< Hash table of ongoing transactions */
  ev_io observer;            /**< Event loop I/O watcher */
  ev_timer timeout_observer; /**< Event loop timer for timeouts */
  double timeout_s;          /**< Timeout in seconds */
} dns_client;

/**
 * @brief Initialize a DNS client
 *
 * @param clt Pointer to the dns_client structure to initialize
 * @param loop Event loop
 * @param cb Callback function for DNS responses
 * @param data User-defined callback data
 * @param transactions Pointer to the transactions hash table
 */
void client_init(dns_client *clt, struct ev_loop *loop, res_callback callback,
                 void *data, transaction_hash_entry *restrict transactions);

/**
 * @brief Sends a DNS request to an upstream resolver.
 *
 * This function sends a DNS request to one of the upstream resolvers using a
 * simple round-robin selection method. It also records the send time for the
 * transaction if the transaction info is found.
 *
 * @param client Pointer to the dns_client structure.
 * @param dns_req Pointer to the DNS request buffer.
 * @param req_len Length of the DNS request.
 * @param tx_id Transaction ID for the DNS request.
 *
 * @note This function uses the restrict keyword for pointers to indicate they
 * don't alias.
 *
 * @warning This function assumes that the RESOLVERS macro and upstream_resolver
 * variable are defined.
 *
 * @see dns_client
 * @see resolver
 * @see transaction_info
 * @see find_transaction
 */
void client_send_request(dns_client *clt, const char *dns_req,
                         const size_t req_len, const uint16_t tx_id);

/**
 * @brief Clean up resources used by a DNS client
 *
 * @param clt Pointer to the dns_client structure to clean up
 */
void client_cleanup(dns_client *restrict clt);

#endif // DNS_CLIENT
