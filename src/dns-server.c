#include "../include/dns-server.h"
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
    fprintf(stderr, "Error parsing listen address %s:%d (getaddrinfo): %s\n",
            listen_addr, listen_port, gai_strerror(res));
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
    perror("Error creating socket");

    freeaddrinfo(ai);

    return -1;
  }

  res = bind(sockfd, ai->ai_addr, ai->ai_addrlen);
  if (res < 0) {
    fprintf(stderr, "Error binding %s:%d: %s (%d)\n", listen_addr, listen_port,
            strerror(errno), res);
    close(sockfd);
    freeaddrinfo(ai);
    return -1;
  }

  freeaddrinfo(ai);

  printf("Listening on %s:%d\n", listen_addr, listen_port);
  return sockfd;
}

static void watcher_cb(struct ev_loop *loop, ev_io *w, int revents) {

  dns_server *dns = (dns_server *)w->data;
  char *buffer = (char *)calloc(1, REQUEST_MAX + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Failed buffer allocation\n");
    return;
  }
  struct sockaddr_storage raddr;
  socklen_t tmp_addrlen = dns->addrlen;
  ssize_t len = recvfrom(w->fd, buffer, REQUEST_MAX, 0,
                         (struct sockaddr *)&raddr, &tmp_addrlen);
  if (len < 0) {
    fprintf(stderr, "Recvfrom failed: %s\n", strerror(errno));
    free(buffer);
    return;
  }
  if (len < (int)sizeof(uint16_t)) {
    fprintf(stderr, "Malformed request received (too short).\n");
    free(buffer);
    return;
  }
  uint16_t tx_id = ntohs(*((uint16_t *)buffer));
  dns->cb(dns, dns->cb_data, (struct sockaddr *)&raddr, tx_id, buffer, len);
  free(buffer);
}

void dns_server_init(dns_server *dns, struct ev_loop *loop, callback cb,
                     const char *listen_addr, int listen_port, void *data) {
  dns->loop = loop;
  dns->sockfd = init_socket(listen_addr, listen_port, &dns->addrlen);
  if (dns->sockfd < 0) {
    fprintf(stderr, "Failed to initialize socket\n");
    return;
  }
  dns->cb = cb;
  dns->cb_data = data;

  // TODO: research more to know what to do here
  ev_io_init(&dns->watcher, watcher_cb, dns->sockfd, EV_READ);
  dns->watcher.data = dns;
  ev_io_start(dns->loop, &dns->watcher);
}

static bool is_blacklisted(const char *domain) {
  for (int i = 0; i < BLACKLIST_SIZE; i++) {
    if (strcasecmp(domain, BLACKLIST[i]) == 0) {
      return true;
    }
  }
  return false;
}

static bool parse_domain_name(const char *dns_req, size_t dns_req_len,
                              size_t offset, char *domain,
                              size_t domain_max_len) {
  size_t pos = offset;
  size_t domain_len = 0;

  while (pos < dns_req_len && dns_req[pos] != 0) {
    uint8_t label_len = (uint8_t)dns_req[pos];

    // Check for DNS compression
    if ((label_len & 0xC0) == 0xC0) {
      if (pos + 2 > dns_req_len) {
        return false; // Malformed packet
      }
      uint16_t jump_pos = ((label_len & 0x3F) << 8) | (uint8_t)dns_req[pos + 1];
      if (jump_pos >= pos) {
        return false; // Invalid forward reference
      }
      pos = jump_pos;
      continue;
    }

    if (label_len > 63 || pos + label_len + 1 > dns_req_len) {
      // Invalid label length or out of bounds
      return false;
    }

    if (domain_len + label_len + 2 > domain_max_len) {
      // Domain name would exceed buffer
      return false;
    }

    if (domain_len > 0) {
      domain[domain_len++] = '.';
    }

    memcpy(domain + domain_len, dns_req + pos + 1, label_len);
    domain_len += label_len;
    pos += label_len + 1;
  }

  if (pos >= dns_req_len) {
    // Reached end of packet without null terminator
    return false;
  }

  domain[domain_len] = '\0';
  return true;
}

void handle_dns_request(struct dns_server *dns, void *data,
                        struct sockaddr *addr, uint16_t tx_id, char *dns_req,
                        size_t dns_req_len) {
  // Assuming the DNS header is at the start of buffer (dns_req)
  dns_header *header = (dns_header *)dns_req;

  if (ntohs(header->qd_count) == 0) {
    printf("No questions in the request.\n");
    free(dns_req);
    return;
  }

  // Offset to the query section (DNS header is 12 bytes)
  size_t query_offset = sizeof(*header);
  char domain[MAX_DOMAIN_LENGTH];

  if (!parse_domain_name(dns_req, dns_req_len, query_offset, domain,
                         sizeof(domain))) {
    printf("Failed to parse domain name.\n");
    free(dns_req);
    return;
  }

  printf("Extracted domain: %s\n", domain);
  if (is_blacklisted(domain)) {
    header->rcode = *BLACKLISTED_RESPONSE;
    header->qr = 1; // This is the response
    header->ans_count = 0;
    dns_server_respond(dns, addr, dns_req, dns_req_len);
  } else {
    // forward to http client and send to upstream resolver
  }
  free(dns_req);
  return;
}

void dns_proxy_respond(dns_server *dns, struct sockaddr *raddr, char *buffer,
                       size_t buflen) {
  ssize_t len = sendto(dns->sockfd, buffer, buflen, 0, raddr, dns->addrlen);
  if (len == -1) {
    printf("sendto client failed: %s", strerror(errno));
  }
}

void dns_server_stop(dns_server *dns) { ev_io_stop(dns->loop, &dns->watcher); }

void dns_server_cleanup(dns_server *dns) { close(dns->sockfd); }
