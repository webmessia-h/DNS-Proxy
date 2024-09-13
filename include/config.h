#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define BLACKLISTED_DOMAINS 4
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
  REQUEST_MAX = 512,
  MAX_DOMAIN_LENGTH = 253,
  DNS_HEADER_SIZE = 12,
  DNS_CLASS_IN = 1,
};

typedef struct options {
  const char *listen_addr;
  uint16_t listen_port;
  uint16_t fallback_port;
  uint8_t BLACKLISTED_RESPONSE;
} options;

void options_init(struct options *opt);

#endif // CONFIG_H
