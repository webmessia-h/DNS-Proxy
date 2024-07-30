#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define BLACKLISTED_DOMAINS 3
extern const char *blacklist[BLACKLISTED_DOMAINS];
#define RESOLVERS 3
extern const char *upstream_resolver[RESOLVERS];

enum {
  REQUEST_MAX = 1500,
  MAX_DOMAIN_LENGTH = 253,
  DNS_HEADER_SIZE = 12,
  DNS_CLASS_IN = 1
};

struct Options {
  const char *listen_addr;
  uint16_t listen_port;
  // Blacklist must contain strings in format "example.com"
  int BLACKLISTED_RESPONSE;
};
typedef struct Options options;

void options_init(struct Options *opt);

#define BUFFER_SIZE /*desired buffer size*/

#endif // CONFIG_H
