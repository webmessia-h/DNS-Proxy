#include "dns-server.h"
#include "config.h" /* Main configuration file */
#include "log.h"

// Creates and bind a listening UDP socket for incoming requests.
static inline int init_socket(const char *restrict listen_addr,
                              const uint16_t listen_port,
                              unsigned int *restrict addrlen) {
  LOG_TRACE("init_socket(listen_addr: %s, listen_port: %d, addrlen ptr: %p)\n",
            listen_addr, listen_port, addrlen);

  struct addrinfo *addrinfo = NULL;
  struct addrinfo hints;
  bool ok = true;
  memset(&hints, 0, sizeof(struct addrinfo));
  // Prevent DNS lookups if leakage is our worry
  hints.ai_flags = AI_NUMERICHOST;

  int res = getaddrinfo(listen_addr, NULL, &hints, &addrinfo);
  if (res != 0) {
    LOG_FATAL("Error parsing listen address %s:%d (getaddrinfo): %s\n",
              listen_addr, listen_port, strerror(res));
    if (addrinfo) {
      freeaddrinfo(addrinfo);
    }
    ok = false;
  }

  struct sockaddr_in *saddr = (struct sockaddr_in *)addrinfo->ai_addr;

  *addrlen = addrinfo->ai_addrlen;
  saddr->sin_port = htons(listen_port);

  int sockfd = socket(addrinfo->ai_family, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    LOG_FATAL("Error creating socket: %s", strerror(errno));
    freeaddrinfo(addrinfo);
    ok = false;
  }

  int opts = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts)) < 0) {
    LOG_ERROR("setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
    close(sockfd);
    freeaddrinfo(addrinfo);
    return -1;
  }

  // Set receive buffer size
  int bufsize = 4194304; // 4 MB
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) <
      0) {
    LOG_ERROR("setsockopt(SO_RCVBUF) failed: %s", strerror(errno));
  }

  // Set send buffer size
  if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) <
      0) {
    LOG_ERROR("setsockopt(SO_SNDBUF) failed: %s", strerror(errno));
  }

  res = bind(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen);
  if (res < 0) {
    LOG_FATAL("Error binding socket %s:%du: %s (%d)\n", listen_addr,
              listen_port, strerror(errno), res);
    close(sockfd);
    freeaddrinfo(addrinfo);
    ok = false;
  }

  freeaddrinfo(addrinfo);

  if (!ok) {
    exit(-1);
  }
  LOG_TRACE("Listening on %s:%d\n", listen_addr, listen_port);
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
 * @warning The function returns early on errors
 * @see dns_server
 *
 */
static void server_receive_request(struct ev_loop *loop, ev_io *obs,
                                   int revents) {
  LOG_TRACE("server_receive_request(loop ptr: %p, obs ptr: %p, revents: %d)",
            loop, obs, revents);
  struct dns_server *srv = NULL;
  srv = (struct dns_server *)obs->data;

  char buffer[REQUEST_AVG + 1];
  memset(buffer, 0, sizeof(buffer));

  struct sockaddr_storage raddr;
  socklen_t tmp_addrlen = srv->addrlen;
  ssize_t len = recvfrom(obs->fd, buffer, REQUEST_AVG, 0,
                         (struct sockaddr *)&raddr, &tmp_addrlen);
  if (len < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      LOG_ERROR("Recvfrom failed: %s\n", strerror(errno));
    }
    return;
  }
  if (len < (int)sizeof(uint16_t)) {
    return; // Silently drop malformed packets
  }

  uint16_t tx_id = ntohs(*((uint16_t *)buffer));
  srv->cb((void *)srv, srv->cb_data, (struct sockaddr *)&raddr, tx_id, buffer,
          len);
}

void server_init(struct dns_server *restrict srv, struct ev_loop *loop,
                 req_callback callback, const char *restrict listen_addr,
                 const uint16_t listen_port, void *restrict data,
                 const hash_entry *restrict blacklist) {
  LOG_TRACE("server_init(srv ptr: %p, loop ptr: %p, callback ptr: %p, "
            "listen_addr: %s, "
            "listen_port: %d, data ptr: %p, blacklist ptr: %p)\n",
            srv, loop, callback, listen_addr, listen_port, data, blacklist);

  srv->loop = loop;
  srv->sockfd = init_socket(listen_addr, listen_port, &srv->addrlen);
  if (srv->sockfd < 0) {
    LOG_FATAL("Failed to initialize socket\n");
    return;
  }
  srv->cb = callback;
  srv->cb_data = data;
  srv->blacklist = blacklist;

  ev_io_init(&srv->observer, server_receive_request, srv->sockfd, EV_READ);
  srv->observer.data = srv;
  ev_io_start(srv->loop, &srv->observer);
}

bool is_blacklisted(const char *domain) {
  LOG_TRACE("is_blacklisted(domain ptr: %p)", domain);
  if (find(domain) == 1) {
    return true;
  }

  return false;
}

bool parse_domain_name(const char *dns_req, const size_t dns_req_len,
                       const size_t offset, char *domain,
                       const size_t domain_max_len) {
  LOG_TRACE("parse_domain_name(dns_req ptr: %p, dns_req_len: %zu, offset %zu, "
            "domain ptr: %p, domain_max_len: %zu)",
            dns_req, dns_req_len, offset, domain, domain_max_len);
  size_t pos = offset;
  size_t domain_len = 0;

  while (pos < dns_req_len) {
    uint8_t label_len = (uint8_t)dns_req[pos];

    if (label_len == 0) {
      break; // End of domain name
    }

    if ((label_len & 0xC0) == 0xC0) {
      // Handle compression (jump to offset)
      if (pos + 2 > dns_req_len)
        return false;
      pos = ((label_len & 0x3F) << 8) | (uint8_t)dns_req[pos + 1];
      continue;
    }

    if (pos + label_len + 1 > dns_req_len) {
      return false;
    }

    if (domain_len + label_len + 1 > domain_max_len) {
      return false;
    }

    if (domain_len > 0) {
      domain[domain_len++] = '.';
    }

    memcpy(domain + domain_len, dns_req + pos + 1, label_len);
    domain_len += label_len;
    pos += label_len + 1;
  }

  domain[domain_len] = '\0';
  return true;
}

void server_send_response(const struct dns_server *restrict srv,
                          const struct sockaddr *restrict raddr,
                          const char *restrict buffer, const size_t buflen) {
  LOG_TRACE("server_send_response(srv ptr: %p, raddr ptr: %p, buffer ptr: %p, "
            "buflen: %zu)",
            srv, raddr, buffer, buflen);
  ssize_t sent = sendto(srv->sockfd, buffer, buflen, 0, raddr, srv->addrlen);
  if (sent < 0) {
    LOG_ERROR("sendto client failed: %s", strerror(errno));
  }
}

void server_stop(struct dns_server *restrict srv) {
  LOG_TRACE("server_stop(srv ptr: %p)", srv);
  ev_io_stop(srv->loop, &srv->observer);
}

void server_cleanup(const struct dns_server *restrict srv) {
  LOG_TRACE("server_cleanup(srv ptr: %p)", srv);
  close(srv->sockfd);
}
