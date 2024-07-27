#include "../include/dns-proxy.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "../config.h" /* Main configuration file */

// Creates and bind a listening UDP socket for incoming requests.
static int init_socket(const char *listen_addr, int listen_port,
                       unsigned int *addrlen) {
  struct addrinfo *ai = NULL;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  // Prevent DNS lookups if leakage is our worry
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

  int sockfd = socket(ai->ai_family, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    printf("Error creating socket");
  }

  res = bind(sockfd, ai->ai_addr, ai->ai_addrlen);
  if (res < 0) {
    printf("Error binding %s:%d: %s (%d)", listen_addr, listen_port,
           strerror(errno), res);
  }

  freeaddrinfo(ai);

  printf("Listening on %s:%d", listen_addr, listen_port);
  return sockfd;
}

static void watcher_cb(struct ev_loop __attribute__((unused)) * loop, ev_io *w,
                       int __attribute__((unused)) revents) {
  dns_proxy *dns = (dns_proxy *)w->data;

  char *buffer = (char *)calloc(1, REQUEST_MAX + 1);
  if (buffer == NULL) {
    printf("Failed buffer allocation");
    return;
  }
  struct sockaddr_storage raddr;
  /* recvfrom can write to addrlen */
  socklen_t tmp_addrlen = dns->addrlen;
  ssize_t len = recvfrom(w->fd, buffer, REQUEST_MAX, 0,
                         (struct sockaddr *)&raddr, &tmp_addrlen);
  if (len < 0) {
    printf("Recvfrom failed: %s", strerror(errno));
    free(buffer);
    return;
  }

  if (len < (int)sizeof(uint16_t)) {
    printf("Malformed request received (too short).");
    free(buffer);
    return;
  }

  uint16_t tx_id = ntohs(*((uint16_t *)buffer));
  dns->cb(dns, dns->cb_data, (struct sockaddr *)&raddr, tx_id, buffer, len);
  free(buffer);
}

void dns_proxy_init(dns_proxy *dns, struct ev_loop *loop, callback cb,
                    const char *listen_addr, int listen_port, void *data) {
  dns->loop = loop;
  dns->sockfd = init_socket(listen_addr, listen_port, &dns->addrlen);
  dns->cb = cb;
  dns->cb_data = data;

  // TODO: research more to know what to do here
  ev_io_init(&dns->watcher, watcher_cb, dns->sockfd, EV_READ);
  dns->watcher.data = dns;
  ev_io_start(dns->loop, &dns->watcher);
}

static bool is_blacklisted(const char *domain) {
  for (int i = 0; i < BLACKLIST_SIZE; i++) {
    if (strcmp(domain, BLACKLIST[i]) == 0) {
      return true;
    }
  }
  return false;
}

static int parse_domain_name(const char *packet, size_t packet_len,
                             size_t offset, char *domain,
                             size_t domain_max_len) {
  size_t pos = offset;
  size_t domain_len = 0;
  while (pos < packet_len && packet[pos] != 0) {
    uint8_t label_len = packet[pos];
    if (label_len > 63 || pos + label_len >= packet_len) {
      // Invalid label length or out of bounds
      return -1;
    }
    if (domain_len + label_len + 1 >= domain_max_len) {
      // Domain name would exceed buffer
      return -1;
    }
    if (domain_len > 0) {
      domain[domain_len++] = '.';
    }
    memcpy(domain + domain_len, packet + pos + 1, label_len);
    domain_len += label_len;
    pos += label_len + 1;
  }
  domain[domain_len] = '\0';
  return 0;
}

void handle_dns_request(struct dns_proxy *dns, void *data,
                        struct sockaddr *addr, uint16_t tx_id, char *dns_req,
                        size_t dns_req_len) {
  // Assuming the DNS header is at the start of buf
  dns_request *request = (dns_request *)dns_req;

  if (ntohs(request->q_count) == 0) {
    printf("No questions in the request.\n");
    free(dns_req);
    return;
  }

  // Offset to the query section (DNS header is 12 bytes)
  size_t query_offset = sizeof(dns_request);
  char domain[256];

  if (parse_domain_name(dns_req, dns_req_len, query_offset, domain,
                        sizeof(domain)) != 0) {
    printf("Failed to parse domain name.\n");
    free(dns_req);
    return;
  }

  printf("Extracted domain: %s\n", domain);
  is_blacklisted(domain);
  free(dns_req); // Free allocated buffer
  return;
}

void dns_proxy_respond(dns_proxy *dns, struct sockaddr *raddr, char *buffer,
                       size_t buflen) {
  ssize_t len = sendto(dns->sockfd, buffer, buflen, 0, raddr, dns->addrlen);
  if (len == -1) {
    printf("sendto client failed: %s", strerror(errno));
  }
}

void dns_server_stop(dns_proxy *dns) { ev_io_stop(dns->loop, &dns->watcher); }

void dns_server_cleanup(dns_proxy *dns) { close(dns->sockfd); }
