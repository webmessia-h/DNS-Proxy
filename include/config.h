#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define BLACKLISTED_DOMAINS 3
extern char *BLACKLIST[BLACKLISTED_DOMAINS];
#define RESOLVERS 3
extern char *upstream_resolver[RESOLVERS];

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
