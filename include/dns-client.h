#ifndef DNS_CLIENT
#define DNS_CLIENT

#include "hash.h"
#include "include.h"

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
typedef void (*res_callback)(struct dns_client *clt, void *data,
                             struct sockaddr *addr, char *response,
                             size_t res_len, uint16_t tx_id);

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
  struct ev_loop *loop;               /**< Event loop */
  void *cb_data;                      /**< User-defined callback data */
  res_callback cb;                    /**< Response callback function */
  int sockfd;                         /**< Socket file descriptor */
  resolver resolvers[RESOLVERS];      /**< Array of DNS resolvers */
  TransactionHashEntry *transactions; /**< Hash table of ongoing transactions */
  ev_io observer;                     /**< Event loop I/O watcher */
  ev_timer timeout_observer;          /**< Event loop timer for timeouts */
  int timeout_ms;                     /**< Timeout in milliseconds */
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
void client_init(dns_client *clt, struct ev_loop *loop, res_callback cb,
                 void *data, TransactionHashEntry *transactions);

/**
 * @brief Send a DNS request
 *
 * @param clt Pointer to the dns_client structure
 * @param dns_req Buffer containing the DNS request
 * @param req_len Length of the request buffer
 * @param tx_id Transaction ID for the request
 */
void client_send_request(dns_client *clt, const char *dns_req, size_t req_len,
                         uint16_t tx_id);

/**
 * @brief Clean up resources used by a DNS client
 *
 * @param clt Pointer to the dns_client structure to clean up
 */
void client_cleanup(dns_client *clt);

#endif // DNS_CLIENT
