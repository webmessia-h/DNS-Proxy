#include "../include/dns-client.h"
#include <fcntl.h>
#include <sys/types.h>

static void handle_resolver_read(struct ev_loop *loop, ev_io *watcher,
                                 int revents) {
  if (!(revents & EV_READ))
    return;

  dns_client *client = (dns_client *)watcher->data;
  char buffer[REQUEST_MAX];
  struct sockaddr_storage src_addr;
  socklen_t src_addrlen = sizeof(src_addr);

  ssize_t received = recvfrom(watcher->fd, buffer, sizeof(buffer), 0,
                              (struct sockaddr *)&src_addr, &src_addrlen);

  if (received < 0) {
    perror("Error receiving DNS response");
    return;
  }

  if (client->cb) {
    client->cb(client->cb_data, buffer, received);
  }
}

void dns_client_init(dns_client *client, struct ev_loop *loop, response_cb cb,
                     void *data, HashMap *map) {
  client->loop = loop;
  client->cb = cb;
  client->cb_data = data;

  for (int i = 0; i < RESOLVERS; i++) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    int status = getaddrinfo(upstream_resolver[i], "53", &hints, &res);
    if (status != 0) {
      fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
      return;
    }

    client->resolvers[i].socket =
        socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (client->resolvers[i].socket < 0) {
      perror("Failed to create socket");
      freeaddrinfo(res);
      return;
    }

    int flags = fcntl(client->resolvers[i].socket, F_GETFL, 0);
    fcntl(client->resolvers[i].socket, F_SETFL, flags | O_NONBLOCK);

    memcpy(&client->resolvers[i].addr, res->ai_addr, res->ai_addrlen);
    client->resolvers[i].addrlen = res->ai_addrlen;

    freeaddrinfo(res);

    ev_io_init(&client->resolvers[i].read_observer, handle_resolver_read,
               client->resolvers[i].socket, EV_READ);
    client->resolvers[i].read_observer.data = client;
    ev_io_start(client->loop, &client->resolvers[i].read_observer);
  }

  return;
}

void dns_client_send_request(dns_client *client, const char *dns_req,
                             size_t req_len) {
  if (sizeof(upstream_resolver) == 0)
    return;

  // Simple round-robin selection of upstream resolver
  static int current_resolver = 0;
  current_resolver = (current_resolver + 1) % RESOLVERS;

  resolver *res = &client->resolvers[current_resolver];

  ssize_t sent = sendto(res->socket, dns_req, req_len, 0,
                        (struct sockaddr *)&res->addr, res->addrlen);

  if (sent < 0) {
    perror("Error sending DNS request");
    return;
  }

  return;
}

void dns_client_cleanup(dns_client *client) {
  for (int i = 0; i < RESOLVERS; i++) {
    ev_io_stop(client->loop, &client->resolvers[i].read_observer);
    close(client->resolvers[i].socket);
  }
}
