#include "dns-server.h"
#include "config.h" /* Main configuration file */
#include "log.h"

// Creates and bind a listening UDP socket for incoming requests.
static inline int init_socket(const char *restrict listen_addr,
                              const int listen_port,
                              unsigned int *restrict addrlen) {
  LOG_DEBUG("init_socket(listen_addr: %s, listen_port: %d, addrlen ptr: %p)\n",
            listen_addr, listen_port, addrlen);
  struct addrinfo *ai = NULL;
  struct addrinfo hints;
  bool ok = true;
  memset(&hints, 0, sizeof(struct addrinfo));
  // Prevent DNS lookups if leakage is our worry
  hints.ai_flags = AI_NUMERICHOST;

  int res = getaddrinfo(listen_addr, NULL, &hints, &ai);
  if (res != 0) {
    LOG_FATAL("Error parsing listen address %s:%d (getaddrinfo): %s\n",
              listen_addr, listen_port, strerror(res));
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
    LOG_FATAL("Error creating socket: %s", strerror(errno));
    freeaddrinfo(ai);
    ok = false;
  }

  res = bind(sockfd, ai->ai_addr, ai->ai_addrlen);
  if (res < 0) {
    LOG_FATAL("Error binding %s:%d: %s (%d)\n", listen_addr, listen_port,
              strerror(errno), res);
    close(sockfd);
    freeaddrinfo(ai);
    ok = false;
  }

  freeaddrinfo(ai);

  if (!ok)
    exit(-1);
  LOG_DEBUG("Listening on %s:%d\n", listen_addr, listen_port);
  return sockfd;
}

/**
 * @brief Callback function for receiving DNS requests
 *
 * This function is called by the event loop when data is available on the
 * server socket. It receives the DNS request, performs basic validation, and
 * invokes the server's callback.
 *
 * @param loop The event loop (unused in this function)
 * @param obs The I/O watcher object
 * @param revents The received events (unused in this function)
 *
 * @note This function allocates memory for the request buffer, which is freed
 * by the callback.
 * @warning The function returns early on errors, freeing the buffer if
 * necessary.
 */
static void server_receive_request(struct ev_loop *loop, ev_io *obs,
                                   int revents) {
  LOG_DEBUG("server_receive_request(loop ptr: %p, obs ptr: %p, revents: %d)",
            loop, obs, revents);
  dns_server *srv = (dns_server *)obs->data;
  char *buffer = (char *)calloc(1, REQUEST_MAX + 1);
  if (buffer == NULL) {
    LOG_ERROR("Failed buffer allocation\n");
    return;
  }
  struct sockaddr_storage raddr;
  socklen_t tmp_addrlen = srv->addrlen;
  ssize_t len = recvfrom(obs->fd, buffer, REQUEST_MAX, 0,
                         (struct sockaddr *)&raddr, &tmp_addrlen);
  if (len < 0) {
    LOG_ERROR("Recvfrom failed: %s\n", strerror(errno));
    if (buffer != NULL)
      free(buffer);
    return;
  }
  if (len < (int)sizeof(uint16_t)) {
    LOG_ERROR("Malformed request received (too short)\n");
    if (buffer != NULL)
      free(buffer);
    return;
  }
  uint16_t tx_id = ntohs(*((uint16_t *)buffer));
  srv->cb((void *)srv, srv->cb_data, (struct sockaddr *)&raddr, tx_id, buffer,
          len);
}

void server_init(dns_server *restrict srv, struct ev_loop *loop,
                 req_callback cb, const char *restrict listen_addr,
                 const int listen_port, void *restrict data,
                 const hash_entry *restrict blacklist) {
  LOG_DEBUG(
      "server_init(srv ptr: %p, loop ptr: %p, cb ptr: %p, listen_addr: %s, "
      "listen_port: %d, data ptr: %p, blacklist ptr: %p)\n",
      srv, loop, cb, listen_addr, listen_port, data, blacklist);
  srv->loop = loop;
  srv->sockfd = init_socket(listen_addr, listen_port, &srv->addrlen);
  if (srv->sockfd < 0) {
    LOG_FATAL("Failed to initialize socket\n");
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
  LOG_DEBUG("is_blacklisted(domain ptr: %p)", domain);
  for (int i = 0; i < BLACKLISTED_DOMAINS; i++) {
    if (find(domain) == 1) {
      return true;
    }
  }
  return false;
}

bool parse_domain_name(const char *dns_req, const size_t dns_req_len,
                       const size_t offset, char *domain,
                       const size_t domain_max_len) {
  LOG_DEBUG("parse_domain_name(dns_req ptr: %p, dns_req_len: %zu, offset %zu, "
            "domain ptr: %p, domain_max_len: %zu)",
            dns_req, dns_req_len, offset, domain, domain_max_len);
  size_t pos = offset;
  size_t domain_len = 0;

  while (pos < dns_req_len && dns_req[pos] != 0) {
    uint8_t label_len = (uint8_t)dns_req[pos];

    // Check for DNS compression
    if ((label_len & 0xC0) == 0xC0) {
      if (pos + 2 > dns_req_len) {
        LOG_ERROR("Malformed packet\n");
        return false; // Malformed packet
      }
      uint16_t jump_pos = ((label_len & 0x3F) << 8) | (uint8_t)dns_req[pos + 1];
      if (jump_pos >= pos) {
        LOG_ERROR("Invalid forward reference\n");
        return false;
      }
      pos = jump_pos;
      continue;
    }

    if (label_len > 63 || pos + label_len + 1 > dns_req_len) {
      LOG_ERROR("Invalid label length or out of bounds\n");
      return false;
    }

    if (domain_len + label_len + 2 > domain_max_len) {
      LOG_ERROR("Domain name would exceed buffer\n");
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
    LOG_ERROR("Reached end of packet without null terminator\n");
    return false;
  }

  domain[domain_len] = '\0';
  return true;
}

void server_send_response(const dns_server *restrict srv,
                          const struct sockaddr *restrict raddr,
                          const char *restrict buffer, const size_t buflen) {
  LOG_DEBUG("server_send_response(srv ptr: %p, raddr ptr: %p, buffer ptr: %p, "
            "buflen: %zu)",
            srv, raddr, buffer, buflen);
  ssize_t sent = sendto(srv->sockfd, buffer, buflen, 0, raddr, srv->addrlen);
  if (sent < 0) {
    printf("sendto client failed: %s", strerror(errno));
    if (buffer != NULL)
      free((void *)buffer);
  }
}

void server_stop(dns_server *restrict srv) {
  LOG_DEBUG("server_stop(srv ptr: %p)", srv);
  ev_io_stop(srv->loop, &srv->observer);
}

void server_cleanup(const dns_server *restrict srv) {
  LOG_DEBUG("server_cleanup(srv ptr: %p)", srv);
  close(srv->sockfd);
}
