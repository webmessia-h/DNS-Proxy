#include "dns-client.h"
#include "config.h" /* Main configuration file */
#include "log.h"

static void client_handle_timeout(struct ev_loop *loop, ev_timer *w,
                                  int revents);

static void client_receive_response(struct ev_loop *loop, ev_io *obs,
                                    int revents) {
  LOG_TRACE("client_receive_response(loop ptr: %p, obs ptr: %p, revents: %d)",
            loop, obs, revents);
  dns_client *client = (dns_client *)obs->data;

  char buffer[REQUEST_AVG + 1];
  memset(buffer, 0, sizeof(buffer));

  struct sockaddr_storage saddr;
  socklen_t src_addrlen = sizeof(saddr);

  ssize_t len = recvfrom(obs->fd, buffer, REQUEST_AVG, 0,
                         (struct sockaddr *)&saddr, &src_addrlen);

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
  client->cb((void *)client, client->cb_data, (struct sockaddr *)&saddr, tx_id,
             buffer, len);
}

void client_init(dns_client *restrict client, struct ev_loop *loop,
                 res_callback cb, void *restrict data,
                 transaction_hash_entry *restrict transactions) {
  LOG_TRACE(
      "client_init(client ptr: %p, loop ptr: %p, cb ptr: %p, data ptr: %p, "
      "transactions ptr: %p)\n",
      client, loop, cb, data, transactions);

  client->loop = loop;
  client->cb = cb;
  client->cb_data = data;
  client->transactions = transactions;

  int opt = 1;

  for (int i = 0; i < RESOLVERS; i++) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    int status = getaddrinfo(upstream_resolver[i], "53", &hints, &res);
    if (status != 0) {
      LOG_ERROR("getaddrinfo error: %s\n", strerror(status));
      free(res);
      return;
    }

    client->resolvers[i].socket =
        socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (client->resolvers[i].socket < 0) {
      LOG_ERROR("Error creating socket\n");
      freeaddrinfo(res);
      return;
    }

    // Set socket options
    if (setsockopt(client->resolvers[i].socket, SOL_SOCKET, SO_REUSEADDR, &opt,
                   sizeof(opt)) < 0) {
      LOG_ERROR("setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
    }

    int bufsize = 4194304; // 4 MB
    if (setsockopt(client->resolvers[i].socket, SOL_SOCKET, SO_RCVBUF, &bufsize,
                   sizeof(bufsize)) < 0) {
      LOG_ERROR("setsockopt(SO_RCVBUF) failed: %s", strerror(errno));
    }
    if (setsockopt(client->resolvers[i].socket, SOL_SOCKET, SO_SNDBUF, &bufsize,
                   sizeof(bufsize)) < 0) {
      LOG_ERROR("setsockopt(SO_SNDBUF) failed: %s", strerror(errno));
    }

    int flags = fcntl(client->resolvers[i].socket, F_GETFL, 0);
    if (flags == -1) {
      LOG_ERROR("fcntl(F_GETFL) failed\n");
      return;
    }

    if (fcntl(client->resolvers[i].socket, F_SETFL, flags | O_NONBLOCK) == -1) {
      LOG_ERROR("fcntl(F_SETFL) failed\n");
      return;
    }

    if (res->ai_addr) {
      memcpy(&client->resolvers[i].addr, res->ai_addr, res->ai_addrlen);
      client->resolvers[i].addrlen = res->ai_addrlen;
    } else {
      LOG_ERROR("Invalid address pointers for memcpy\n");
      return;
    }

    if (res) {
      freeaddrinfo(res);
    } else {
      LOG_ERROR("Attempted to free a null addrinfo structure\n");
    }

    if (client->resolvers[i].socket >= 0) {
      ev_io_init(&client->resolvers[i].observer, client_receive_response,
                 client->resolvers[i].socket, EV_READ);
    } else {
      LOG_ERROR("Invalid socket for ev_io_init\n");
    }

    client->resolvers[i].observer.data = client;

    client->timeout_s = 4; // 4 seconds timeout
    LOG_DEBUG("Initializing timeout timer with value: %f\n", client->timeout_s);
    ev_timer_init(&client->timeout_observer, client_handle_timeout,
                  client->timeout_s, 0.);
    LOG_DEBUG("Starting timeout timer\n");

    ev_io_start(client->loop, &client->resolvers[i].observer);
  }

  return;
}

void client_send_request(dns_client *restrict client,
                         const char *restrict dns_req, const size_t req_len,
                         const uint16_t tx_id) {
  LOG_TRACE("client_send_request(client ptr: %p, dns_req ptr: %p, req_len: "
            "%zu, tx_id: %u)",
            client, dns_req, req_len, tx_id);

  if (sizeof(upstream_resolver) == 0) {
    LOG_ERROR("sizeof upstream_resolver == 0\n");
  }

  // Simple round-robin selection of upstream resolver
  static int current_resolver = 0;
  current_resolver = (current_resolver + 1) % RESOLVERS;

  resolver *res = &client->resolvers[current_resolver];

  transaction_info *tx_info = find_transaction(tx_id);

  if (tx_info)
    tx_info->timestamp = time(NULL); // Record the send time

  ssize_t sent = sendto(res->socket, dns_req, req_len, 0,
                        (struct sockaddr *)&res->addr, res->addrlen);

  if (sent < 0) {
    LOG_ERROR("sendto resolver failed: %s\n", strerror(errno));

    // Additional error checking
    if (errno == EDESTADDRREQ) {
      LOG_ERROR("Socket failure state: ");
      int error = 0;
      socklen_t len = sizeof(error);
      if (getsockopt(res->socket, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
        LOG_ERROR("Unable to get socket error\n");
      else if (error != 0)
        LOG_ERROR("Socket error: %s\n", strerror(error));
      else
        LOG_ERROR("No error reported\n");
    }
    return;
  }
  return;
}

static inline void client_handle_timeout(struct ev_loop *loop, ev_timer *w,
                                         int revents) {
  dns_client *client = (dns_client *)w->data;
  LOG_TRACE("client_handle_timeout(loop ptr: %p, w ptr: %p, revents: %d)\n",
            loop, w, revents);

  time_t current_time = time(NULL);
  transaction_hash_entry *entry, *tmp;

  HASH_ITER(hh, client->transactions, entry, tmp) {
    if (difftime(current_time, entry->value->timestamp) > client->timeout_s) {
      LOG_WARN("Transaction %u timed out\n", entry->key);
      // Notify the proxy about the timeout
      client->cb(client, client->cb_data, NULL, entry->key, NULL, 0);

      // Remove the timed-out transaction
      delete_transaction(entry->value->original_tx_id);
    }
  }

  // Restart the timer for the next check
  ev_timer_again(loop, w);
}

void client_cleanup(dns_client *restrict client) {
  LOG_TRACE("client_cleanup(client ptr: %p)\n", client);
  for (int i = 0; i < RESOLVERS; i++) {
    ev_io_stop(client->loop, &client->resolvers[i].observer);
    close(client->resolvers[i].socket);
  }
}
