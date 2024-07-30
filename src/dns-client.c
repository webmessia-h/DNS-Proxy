#include "../include/dns-client.h"
#include <fcntl.h>

static void handle_resolver_read(struct ev_loop *loop, ev_io *watcher,
                                 int revents) {
  if (!(revents & EV_READ))
    return;

  dns_client *client = (dns_client *)watcher->data;
  char *buffer = (char *)calloc(1, REQUEST_MAX + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Failed buffer allocation\n");
    return;
  }
  struct sockaddr_storage saddr;
  socklen_t src_addrlen = sizeof(saddr);

  ssize_t len = recvfrom(watcher->fd, buffer, sizeof(buffer), 0,
                         (struct sockaddr *)&saddr, &src_addrlen);

  if (len < 0) {
    fprintf(stderr, "Recvfrom failed: %s\n", strerror(errno));
    free(buffer);
    return;
  }
  if (len < (int)sizeof(uint16_t)) {
    fprintf(stderr, "Malformed response received (too short)\n");
    free(buffer);
    return;
  }

  uint16_t tx_id = ntohs(*((uint16_t *)buffer));

  client->cb(client->cb_data, buffer, len, tx_id);
}

void dns_client_init(dns_client *client, struct ev_loop *loop, res_callback cb,
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

    ev_io_init(&client->resolvers[i].observer, handle_resolver_read,
               client->resolvers[i].socket, EV_READ);
    client->resolvers[i].observer.data = client;
    ev_io_start(client->loop, &client->resolvers[i].observer);
  }

  return;
}

void dns_client_send_request(dns_client *client, const char *dns_req,
                             size_t req_len, uint16_t tx_id) {
  if (sizeof(upstream_resolver) == 0)
    return;

  // Simple round-robin selection of upstream resolver
  static int current_resolver = 0;
  current_resolver = (current_resolver + 1) % RESOLVERS;

  resolver *res = &client->resolvers[current_resolver];

  ssize_t sent = sendto(res->socket, dns_req, req_len, 0,
                        (struct sockaddr *)&res->addr, res->addrlen);

  if (sent < 0) {
    printf("sendto client failed: %s", strerror(errno));
    return;
  }

  return;
}

void dns_client_cleanup(dns_client *client) {
  for (int i = 0; i < RESOLVERS; i++) {
    ev_io_stop(client->loop, &client->resolvers[i].observer);
    close(client->resolvers[i].socket);
  }
}
