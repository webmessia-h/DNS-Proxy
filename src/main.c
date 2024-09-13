#include "config.h"
#include "dns-proxy.h"
#include "log.h"

static struct ev_loop *loop;
static dns_server server;
static dns_client client;
static dns_proxy proxy;
static struct options opts;
hash_entry *blacklist = NULL;
transaction_hash_entry *transactions = NULL;

static void sigint_cb(struct ev_loop *loop, ev_signal *obs, int revents) {
  LOG_DEBUG("sigint_cb(loop ptr: %p, obs ptr: %p, revents: %d)\n", loop, obs,
            revents);
  LOG_INFO("Received SIGINT, stopping...\n");
  ev_break(loop, EVBREAK_ALL);
  server_stop(&server);
  server_cleanup(&server);
  client_cleanup(&client);
  proxy_stop(&proxy);
  delete_blacklist();
  delete_all_transactions();
}

static void populate_blacklist(void) {
  LOG_DEBUG("populate_blacklist(void)\n");
  for (int i = 0; i < BLACKLISTED_DOMAINS; i++) {
    add_blacklist_entry(BLACKLIST[i]);
  }
}

int main(void) {

  loop = EV_DEFAULT;
  options_init(&opts);
  populate_blacklist();
  log_set_level(LOG_LEVEL_ERROR);

  ev_signal signal_observer;
  ev_signal_init(&signal_observer, sigint_cb, SIGINT);
  ev_signal_start(loop, &signal_observer);

  server_init(&server, loop, NULL, opts.listen_addr, opts.listen_port, NULL,
              blacklist);
  client_init(&client, loop, NULL, upstream_resolver, transactions);

  proxy_init(&proxy, &client, &server, loop);

  LOG_INFO("DNS proxy started. Press Ctrl+C to stop.\n");
  ev_run(loop, 0);
  ev_loop_destroy(loop);

  return 0;
}
