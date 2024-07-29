#ifndef SERVER_H
#define SERVER_H

// #include <ares.h> // FIXME FORBIDDEN BY TASK
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

// DNS header flags
#define DNS_FLAG_QR 0x8000
#define DNS_FLAG_OPCODE 0x7800
#define DNS_FLAG_AA 0x0400
#define DNS_FLAG_TC 0x0200
#define DNS_FLAG_RD 0x0100
#define DNS_FLAG_RA 0x0080
#define DNS_FLAG_Z 0x0070
#define DNS_FLAG_RCODE 0x000F

// DNS opcodes
#define DNS_OPCODE_QUERY 0
#define DNS_OPCODE_IQUERY 1
#define DNS_OPCODE_STATUS 2

// DNS response codes
#define DNS_RCODE_NOERROR 0
#define DNS_RCODE_FORMERR 1
#define DNS_RCODE_SERVFAIL 2
#define DNS_RCODE_NXDOMAIN 3
#define DNS_RCODE_NOTIMP 4
#define DNS_RCODE_REFUSED 5

// DNS types
#define DNS_TYPE_A 1
#define DNS_TYPE_NS 2
#define DNS_TYPE_CNAME 5
#define DNS_TYPE_SOA 6
#define DNS_TYPE_PTR 12
#define DNS_TYPE_MX 15
#define DNS_TYPE_TXT 16
#define DNS_TYPE_AAAA 28

// DNS classes
#define DNS_CLASS_IN 1

// DNS sizes
#define MAX_DOMAIN_LENGTH 253
#define DNS_HEADER_SIZE 12

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

  uint16_t qd_count;   // Number of question entries
  uint16_t ans_count;  // Number of answer entries
  uint16_t auth_count; // Number of authority entries
  uint16_t add_count;  // Number of resource entries#pragma pack(pop)
} dns_header;
typedef struct {
  char *qname;     // Domain name
  uint16_t qtype;  // Query type
  uint16_t qclass; // Query class
} dns_question;
#pragma pack(pop)

struct dns_server;

typedef void (*callback)(struct dns_server *dns, void *data,
                         struct sockaddr *addr, uint16_t tx_id, char *dns_req,
                         size_t dns_req_len);

// TODO: add wtf else is needed here
typedef struct dns_server {
  struct ev_loop *loop;
  void *cb_data;
  callback cb;
  int sockfd;
  socklen_t addrlen;
  ev_io watcher;
} dns_server;

/* Initialize proxy with:
 * callback function,
 * self-address, self-port,
 * callback function (just check whether the domain is in blacklist)
 * and user-defined data (response string in our case)
 */
void dns_server_init(dns_server *dns, struct ev_loop *loop, callback cb,
                     const char *listen_addr, int listen_port, void *data);

/* Checks whether domain is in the blacklist and forwards upstream or returns
 * response defined in "../config.h"
 */
void handle_dns_request(struct dns_server *dns, void *data,
                        struct sockaddr *addr, uint16_t tx_id, char *dns_req,
                        size_t dns_req_len);

// Sends a DNS response 'buffer' of length 'buflen' to 'raddr'.
void dns_server_respond(dns_server *dns, struct sockaddr *raddr, char *buffer,
                        size_t buflen);

void dns_server_stop(dns_server *dns);

void dns_server_cleanup(dns_server *dns);

#endif // SERVER_H
