#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define BLACKLISTED_DOMAINS 3
extern const char *BLACKLIST[BLACKLISTED_DOMAINS];
#define RESOLVERS 3
extern const char *upstream_resolver[RESOLVERS];
#define REDIRECT 0 // binary format, 0 -> redirect is not set, 1 -> otherwise

/* #define REDIRECT_COUNT
 * unused, you must uncomment and change type to
 * extern const char* redirect_to[REDIRECT_COUNT]
 * WARN but it requires a little of adjustment in
 * src/dns-proxy.c -> proxy_handle_request() function
 */

extern const char *redirect_to;

extern const uint8_t BLACKLISTED_RESPONSE;

enum {
  NOERROR = 0,  // all good
  FORMERR = 1,  // format error
  SERVFAIL = 2, // server failure
  NXDOMAIN = 3, // no such domain
  NOTIMP = 4,
  REFUSED = 5, // refused
  YXDOMAIN = 6,
  XRRSET = 7,
  NOTAUTH = 8,
  NOTZONE = 9 // not in responsibility zone
}; // DNS response codes

enum {
  REQUEST_AVG = 128,    // average DNS request packet size
  REQUEST_MAX = 512,    // reasonable for UDP
  RESPONSE_AVG = 128,   // average response size is 40 bytes
  RESPONSE_MAX = 512,   // reasonable for UDP
  DOMAIN_AVG = 50,      // most of domain names are 7-15 characters long
  DOMAIN_MAX = 253,     // will happen once in an eternity
  DNS_HEADER_SIZE = 12, // RFC
  DNS_CLASS_IN = 1,
}; // networking constants

struct options {
  const char *listen_addr;
  uint16_t listen_port;
  uint16_t fallback_port;
  uint8_t BLACKLISTED_RESPONSE;
  uint8_t log_level;
};

void options_init(struct options *opt);

#endif // CONFIG_H
