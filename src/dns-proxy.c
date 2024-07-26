#include "../include/dns-proxy.h"
#include "stdio.h"

// Creates and bind a listening UDP socket for incoming requests.
static int get_listen_sock(const char *listen_addr, int listen_port,
                           unsigned int *addrlen) {
  struct addrinfo *ai = NULL;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  /* prevent DNS lookups if leakage is our worry */
  hints.ai_flags = AI_NUMERICHOST;

  int res = getaddrinfo(listen_addr, NULL, &hints, &ai);
  if (res != 0) {
    printf("Error parsing listen address %s:%d (getaddrinfo): %s", listen_addr,
           listen_port, gai_strerror(res));
    if (ai) {
      freeaddrinfo(ai);
    }
    return -1;
  }

  struct sockaddr_in *saddr = (struct sockaddr_in *)ai->ai_addr;

  *addrlen = ai->ai_addrlen;
  saddr->sin_port = htons(listen_port);

  int sock = socket(ai->ai_family, SOCK_DGRAM, 0);
  if (sock < 0) {
    printf("Error creating socket");
  }

  res = bind(sock, ai->ai_addr, ai->ai_addrlen);
  if (res < 0) {
    printf("Error binding %s:%d: %s (%d)", listen_addr, listen_port,
           strerror(errno), res);
  }

  freeaddrinfo(ai);

  printf("Listening on %s:%d", listen_addr, listen_port);
  return sock;
}
