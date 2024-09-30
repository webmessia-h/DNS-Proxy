#ifndef SERVER_H
#define SERVER_H

#include "hash.h"
#include "include.h"

#pragma pack(push, 1)
/**
 * @brief DNS header structure
 */
struct dns_header {
  uint16_t id;         /**< Identification number */
  uint8_t rd : 1;      /**< Recursion Desired */
  uint8_t tc : 1;      /**< Truncated Message */
  uint8_t aa : 1;      /**< Authoritative Answer */
  uint8_t opcode : 4;  /**< Purpose of message */
  uint8_t qr : 1;      /**< Query/Response Flag */
  uint8_t rcode : 4;   /**< Response code */
  uint8_t z : 3;       /**< Reserved */
  uint8_t ra : 1;      /**< Recursion Available */
  uint16_t qd_count;   /**< Number of question entries */
  uint16_t ans_count;  /**< Number of answer entries */
  uint16_t auth_count; /**< Number of authority entries */
  uint16_t add_count;  /**< Number of resource entries */
};
/**
 * @brief DNS question structure
 */
struct dns_question {
  char *qname;     /**< Domain name */
  uint16_t qtype;  /**< Query type */
  uint16_t qclass; /**< Query class */
};
#pragma pack(pop)

struct dns_server;

/**
 * @brief Callback function type for DNS requests
 */
typedef void (*req_callback)(void *restrict srv, void *restrict data,
                             const struct sockaddr *addr, const uint16_t tx_id,
                             char *restrict dns_req, const size_t dns_req_len);

/**
 * @brief DNS server structure
 */
struct dns_server {
  struct ev_loop *loop;        /**< Event loop */
  void *cb_data;               /**< Additional data for callback */
  req_callback cb;             /**< Callback function */
  int sockfd;                  /**< Socket file descriptor */
  socklen_t addrlen;           /**< Address length */
  ev_io observer;              /**< Event loop observer */
  const hash_entry *blacklist; /**< Blacklist */
};

/**
 * @brief Initialize the DNS server
 * @param srv Pointer to dns_server struct
 * @param loop Event loop
 * @param callback Callback function for requests
 * @param listen_addr Listen address
 * @param listen_port Listen port
 * @param fallback_port Fallback port
 * @param data User-defined data
 * @param blacklist Blacklist hash map
 */
void server_init(struct dns_server *restrict srv, struct ev_loop *loop,
                 req_callback callback, const char *restrict listen_addr,
                 const uint16_t listen_port, void *restrict data,
                 const hash_entry *restrict blacklist);

/**
 * @brief Check if a domain is blacklisted
 * @param domain Domain name to check
 * @return true if blacklisted, false otherwise
 */
bool is_blacklisted(const char *restrict domain);

/**
 * @brief Parse a domain name from a DNS request
 * @param dns_req DNS request buffer
 * @param dns_req_len Length of DNS request
 * @param offset Offset in dns_req where domain name starts
 * @param domain Buffer to store parsed domain name
 * @param domain_max_len Maximum length of domain buffer
 * @return true if parsing successful, false otherwise
 */
bool parse_domain_name(const char *restrict dns_req, const size_t dns_req_len,
                       const size_t offset, char *restrict domain,
                       const size_t domain_max_len);

/**
 * @brief Send a DNS response
 * @param srv Pointer to dns_server struct
 * @param raddr Recipient address
 * @param buffer Response buffer
 * @param buflen Length of response buffer
 */
void server_send_response(const struct dns_server *restrict srv,
                          const struct sockaddr *raddr,
                          const char *restrict buffer, const size_t buflen);

/**
 * @brief Stop the event loop
 * @param srv Pointer to dns_server struct
 */
void server_stop(struct dns_server *restrict srv);

/**
 * @brief Clean up server resources
 * @param srv Pointer to dns_server struct
 */
void server_cleanup(const struct dns_server *restrict srv);

#endif // SERVER_H
