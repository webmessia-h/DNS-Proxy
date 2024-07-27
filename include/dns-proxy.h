#ifndef PROXY_H
#define PROXY_H

#include <ares.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ev.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum {
  REQUEST_MAX = 1500 /* A default MTU */
};

/* DNS packet
 * pragma directives to ensure consequent alignment of data
 * e.g. without padding
 */
#pragma pack(push, 1)
typedef struct {
  uint16_t id;        // Identification number
  uint8_t rd : 1;     // Recursion Desired
  uint8_t tc : 1;     // Truncated Message
  uint8_t aa : 1;     // Authoritative Answer
  uint8_t opcode : 4; // Purpose of message
  uint8_t qr : 1;     // Query/Response Flag

  uint8_t rcode : 4; // Response code
  uint8_t z : 3;     // Reserved
  uint8_t ra : 1;    // Recursion Available

  uint16_t q_count;    // Number of question entries
  uint16_t ans_count;  // Number of answer entries
  uint16_t auth_count; // Number of authority entries
  uint16_t add_count;  // Number of resource entries#pragma pack(pop)
} dns_request;
#pragma pack(pop)

typedef void (*callback)(struct dns_proxy *dns, void *data,
                         struct sockaddr *addr, uint16_t tx_id, char *dns_req,
                         size_t dns_req_len);

// TODO: add wtf else is needed here
typedef struct dns_proxy {
  struct ev_loop *loop;
  void *cb_data;
  callback cb;
  int sockfd;
  socklen_t addrlen;
  ev_io watcher;
} dns_proxy;

struct dns_proxy;

/* Initialize proxy with:
 * callback function,
 * self-address, self-port,
 * callback function (just check whether the domain is in blacklist)
 * and user-defined data (response string in our case)
 */
void dns_proxy_init(dns_proxy *dns, struct ev_loop *loop, callback cb,
                    const char *listen_addr, int listen_port, void *data);

/* Checks whether domain is in the blacklist and forwards upstream or returns
 * response defined in "../config.h"
 */
void handle_dns_request(struct dns_proxy *dns, void *data,
                        struct sockaddr *addr, uint16_t tx_id, char *dns_req,
                        size_t dns_req_len);

// Sends a DNS response 'buffer' of length 'buflen' to 'raddr'.
void dns_proxy_respond(dns_proxy *dns, struct sockaddr *raddr, char *buffer,
                       size_t buflen);

void dns_proxy_stop(dns_proxy *dns);

void dns_proxy_cleanup(dns_proxy *dns);

#endif // PROXY_H
