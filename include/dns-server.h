#ifndef SERVER_H
#define SERVER_H

#include "../include/hash.h"
#include "include.h"

// DNS header flags
typedef enum header_flags {
  DNS_FLAG_QR = 0x8000,
  DNS_FLAG_OPCODE = 0x7800,
  DNS_FLAG_AA = 0x0400,
  DNS_FLAG_TC = 0x0200,
  DNS_FLAG_RD = 0x0100,
  DNS_FLAG_RA = 0x0080,
  DNS_FLAG_Z = 0x0070,
  DNS_FLAG_RCODE = 0x000F
} header_flags;

// Opcodes
typedef enum opcodes {
  DNS_OPCODE_QUERY = 0,
  DNS_OPCODE_IQUERY = 1,
  DNS_OPCODE_STATUS = 2
} opcodes;

// Rcodes
typedef enum response_codes {
  DNS_RCODE_NOERROR = 0,
  DNS_RCODE_FORMERR = 1,
  DNS_RCODE_SERVFAIL = 2,
  DNS_RCODE_NXDOMAIN = 3,
  DNS_RCODE_NOTIMP = 4,
  DNS_RCODE_REFUSED = 5
} response_codes;

typedef enum DNS_types {
  DNS_TYPE_A = 1,
  DNS_TYPE_NS = 2,
  DNS_TYPE_CNAME = 5,
  DNS_TYPE_SOA = 6,
  DNS_TYPE_PTR = 12,
  DNS_TYPE_MX = 15,
  DNS_TYPE_TXT = 16,
  DNS_TYPE_AAAA = 28
} DNS_types;

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
  uint16_t add_count;  // Number of resource entries
} dns_header;
typedef struct {
  char *qname;     // Domain name
  uint16_t qtype;  // Query type
  uint16_t qclass; // Query class
} dns_question;
#pragma pack(pop)

struct dns_server;

typedef void (*req_callback)(struct dns_server *dns, void *data, HashMap *map,
                             struct sockaddr *addr, uint16_t tx_id,
                             char *dns_req, size_t dns_req_len);

typedef struct dns_server {
  struct ev_loop *loop;
  void *cb_data;
  req_callback cb;
  int sockfd;
  socklen_t addrlen;
  ev_io observer;
  HashMap *blacklist;
} dns_server;

/* Initialize self with:
 * callback function,
 * self-address, self-port,
 * callback function (just check whether the domain is in blacklist)
 * and user-defined data (response string in our case)
 */
void dns_server_init(dns_server *srv, struct ev_loop *loop, req_callback cb,
                     const char *listen_addr, int listen_port, void *data,
                     HashMap *map);

bool is_blacklisted(const char *domain, HashMap *map);

bool parse_domain_name(const char *dns_req, size_t dns_req_len, size_t offset,
                       char *domain, size_t domain_max_len);
// Sends a DNS response 'buffer' of length 'buflen' to 'raddr'.
void dns_server_respond(dns_server *srv, struct sockaddr *raddr, char *buffer,
                        size_t buflen);
// Stops ev_io loop
void dns_server_stop(dns_server *srv);
// Closes socket file descriptor
void dns_server_cleanup(dns_server *srv);

#endif // SERVER_H
