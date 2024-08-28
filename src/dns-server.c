#include "dns-server.h"
#include "config.h" /* Main configuration file */

// Creates and bind a listening UDP socket for incoming requests.
static int init_socket(const char *listen_addr, int listen_port,
                       unsigned int *addrlen) {
  struct addrinfo *ai = NULL;
  struct addrinfo hints;
  bool ok = true;
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
    ok = false;
  }

  struct sockaddr_in *saddr = (struct sockaddr_in *)ai->ai_addr;

  *addrlen = ai->ai_addrlen;
  saddr->sin_port = htons(listen_port);

  int sockfd = socket(ai->ai_family, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "Error creating socket: %s", strerror(errno));
    freeaddrinfo(ai);
    ok = false;
  }

  res = bind(sockfd, ai->ai_addr, ai->ai_addrlen);
  if (res < 0) {
    fprintf(stderr, "Error binding %s:%d: %s (%d)\n", listen_addr, listen_port,
            strerror(errno), res);
    close(sockfd);
    freeaddrinfo(ai);
    ok = false;
  }

  freeaddrinfo(ai);

  if (!ok)
    exit(errno);
  printf("Listening on %s:%d\n", listen_addr, listen_port);
  return sockfd;
}

static void server_receive_request(struct ev_loop *loop, ev_io *obs,
                                   int revents) {

  dns_server *srv = (dns_server *)obs->data;
  char *buffer = (char *)calloc(1, REQUEST_MAX + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Failed buffer allocation\n");
    return;
  }
  struct sockaddr_storage raddr;
  socklen_t tmp_addrlen = srv->addrlen;
  ssize_t len = recvfrom(obs->fd, buffer, REQUEST_MAX, 0,
                         (struct sockaddr *)&raddr, &tmp_addrlen);
  if (len < 0) {
    fprintf(stderr, "Recvfrom failed: %s\n", strerror(errno));
    free(buffer);
    return;
  }
  if (len < (int)sizeof(uint16_t)) {
    fprintf(stderr, "Malformed request received (too short)\n");
    free(buffer);
    return;
  }
  uint16_t tx_id = ntohs(*((uint16_t *)buffer));
  srv->cb(srv, srv->cb_data, (struct sockaddr *)&raddr, tx_id, buffer, len);
}

void server_init(dns_server *srv, struct ev_loop *loop, req_callback cb,
                 const char *listen_addr, int listen_port, void *data,
                 HashEntry *blacklist) {
  srv->loop = loop;
  srv->sockfd = init_socket(listen_addr, listen_port, &srv->addrlen);
  if (srv->sockfd < 0) {
    fprintf(stderr, "Failed to initialize socket\n");
    return;
  }
  srv->cb = cb;
  srv->cb_data = data;
  srv->blacklist = blacklist;

  ev_io_init(&srv->observer, server_receive_request, srv->sockfd, EV_READ);
  srv->observer.data = srv;
  ev_io_start(srv->loop, &srv->observer);
}

bool is_blacklisted(const char *domain) {
  for (int i = 0; i < BLACKLISTED_DOMAINS; i++) {
    if (find(domain) == 1) {
      return true;
    }
  }
  return false;
}

bool parse_domain_name(const char *dns_req, size_t dns_req_len, size_t offset,
                       char *domain, size_t domain_max_len) {
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

void server_send_response(dns_server *srv, struct sockaddr *raddr, char *buffer,
                          size_t buflen) {
  ssize_t sent = sendto(srv->sockfd, buffer, buflen, 0, raddr, srv->addrlen);
  if (sent < 0) {
    printf("sendto client failed: %s", strerror(errno));
  }
}

void server_stop(dns_server *srv) { ev_io_stop(srv->loop, &srv->observer); }

void server_cleanup(dns_server *srv) { close(srv->sockfd); }
