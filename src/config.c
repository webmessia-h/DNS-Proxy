#include "config.h"
#include "log.h"

void options_init(struct options *opts) {
  log_set_level(LOG_LEVEL_INFO);
  LOG_TRACE("options_init(opts ptr: %p)\n", opts);
  opts->listen_addr = "127.0.0.1";
  opts->listen_port = 53;
  opts->fallback_port = 5353;
}

/* IF YOU WANT TO ADD ANY RESOLVER OR DOMAINS(TO BLACKLIST) THEN YOU MUST CHANGE
 * CORRESPONDING DEFINE'S IN THE ../include/config.h otherwise will cause
 * segmentation fault
 */
const char *upstream_resolver[] = {"8.8.8.8", "8.8.4.4",
                                   "9.9.9.9"}; //, "9.9.9.9"};
const char *BLACKLIST[] = {
    "youtube.com",
    "google.com",
    "microsoft.com",
};

const uint8_t BLACKLISTED_RESPONSE = NXDOMAIN;

// you must provide redirect domain name in format: "\x07example\x03com\x00"
// (length of fist label, second label and null terminator in HEX, conversion
// table below)
const char *redirect_to = "\x0Atorproject\x03org\x00";
/*
 * Hexadecimal <-> Decimal
 * Hexadecimal format:  \x*num**num*
 * Example 10(Decimal) = \x0A(Hexadecimal)
 * 0<->0
 * 1<->1
 * 2<->2
 * 3<->3
 * 4<->4
 * 5<->5
 * 6<->6
 * 7<->7
 * 8<->8
 * 9<->9
 * A<->10
 * B<->11
 * C<->12
 * D<->13
 * E<->14
 * F<->15
 */
