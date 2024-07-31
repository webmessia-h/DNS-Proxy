#include "../include/dns-proxy.h"
#include <signal.h>
#include <stdio.h>

static struct ev_loop *loop;
static dns_server server;
static dns_client client;
static dns_proxy proxy;
static struct Options opts;

static void sigint_cb(struct ev_loop *loop, ev_signal *obs, int revents) {
  fprintf(stderr, "Received SIGINT, stopping...\n");
  ev_break(loop, EVBREAK_ALL);
}

static HashMap *blacklist_hashmap(HashMap *map) {
  if (map == NULL) {
    fprintf(stderr, "Failed to create hashmap\n");
    printf("sizeof map: %lu\t", sizeof(*map));
    return NULL;
  }
  for (int i = 0; i < BLACKLISTED_DOMAINS; i++) {
    insert(map, blacklist[i]);
  }
  return map;
}

int main(void) {

  loop = EV_DEFAULT;
  options_init(&opts);
  HashMap *blacklist = create_hash_map();
  HashMap *transactions = create_hash_map();

  blacklist_hashmap(blacklist);
  if (blacklist == NULL) {
    fprintf(stderr, "Failed to create blacklist hashmap\n");
    return 1;
  }

  ev_signal signal_observer;
  ev_signal_init(&signal_observer, sigint_cb, SIGINT);
  ev_signal_start(loop, &signal_observer);

  server_init(&server, loop, NULL, opts.listen_addr, opts.listen_port,
              &opts.BLACKLISTED_RESPONSE, blacklist);
  client_init(&client, loop, NULL, upstream_resolver, transactions);

  proxy_init(&proxy, &client, &server, loop);

  printf("DNS proxy started. Press Ctrl+C to stop.\n");
  ev_run(loop, 0);

  server_stop(&server);
  server_cleanup(&server);
  client_cleanup(&client);
  proxy_stop(&proxy);
  return 0;
}
