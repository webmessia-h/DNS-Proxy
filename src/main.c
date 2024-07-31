#include "../include/dns-proxy.h"
#include <signal.h>
#include <stdio.h>

static struct ev_loop *loop;
static dns_server server;
static dns_client client;
static dns_proxy proxy;
static struct Options opts;
HashEntry *blacklist = NULL;
TransactionHashEntry *transactions = NULL;

static void sigint_cb(struct ev_loop *loop, ev_signal *obs, int revents) {
  fprintf(stderr, "Received SIGINT, stopping...\n");
  ev_break(loop, EVBREAK_ALL);
}

static void populate_blacklist(void) {
  for (int i = 0; i < BLACKLISTED_DOMAINS; i++) {
    add_blacklist_entry(BLACKLIST[i]);
  }
}

int main(void) {

  loop = EV_DEFAULT;
  options_init(&opts);
  populate_blacklist();

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
