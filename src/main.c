#include "../config.h" /* main configuration file */
#include "../include/dns-proxy.h"

int main(void) {
  struct ev_loop *loop = EV_DEFAULT;
  struct dns_proxy proxy;
  void *callback = handle_dns_request;
  dns_proxy_init(&proxy, loop, callback, "8.8.8.8", 1234, BLACKLISTED_RESPONSE);
  return 0;
}

/*
 * Incompatible function pointer types passing
 * 'void (struct dns_proxy *, void*, struct sockaddr *, uint16_t, char *,
 * size_t)' (aka 'void (struct dns_proxy*, void *, struct sockaddr *, unsigned
 * short, char *, unsigned long)') to parameter of type 'dns_req_received' (aka
 * 'void (*)(struct dns_proxy *, void*, struct sockaddr *, unsigned short, char
 * *, unsigned long)') clang
 * (-Wincompatible-function-pointer-types) [6, 32]
 */
