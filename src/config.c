#include "../include/config.h"

typedef enum {
  NOERROR = 0,
  FORMERR = 1,
  SERVFAIL = 2,
  NXDOMAIN = 3,
  NOTIMP = 4,
  REFUSED = 5,
  YXDOMAIN = 6,
  XRRSET = 7,
  NOTAUTH = 8,
  NOTZONE = 9
} dns_response; // DNS response codes

const char *upstream_resolver[] = {"8.8.8.8", "1.1.1.1", "8.8.4.4"};
const char *blacklist[] = {"microsoft.com", "google.com", "github.com"};

void options_init(struct Options *opts) {
  opts->listen_addr = "127.0.0.1";
  opts->listen_port = 53;
  opts->BLACKLISTED_RESPONSE = NXDOMAIN;
}
