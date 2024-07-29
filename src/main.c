// #include "../config.h"
#include "../include/dns-proxy.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static struct ev_loop *loop;
static dns_proxy proxy;

static void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents) {
  fprintf(stderr, "Received SIGINT, stopping...\n");
  ev_break(loop, EVBREAK_ALL);
}

int main(void) {
  loop = EV_DEFAULT;

  ev_signal signal_watcher;
  ev_signal_init(&signal_watcher, sigint_cb, SIGINT);
  ev_signal_start(loop, &signal_watcher);

  dns_proxy_init(&proxy, loop, handle_dns_request, "127.0.0.1", 53, NULL);

  if (proxy.sockfd < 0) {
    fprintf(stderr, "Failed to initialize DNS proxy\n");
    return 1;
  }

  printf("DNS proxy started. Press Ctrl+C to stop.\n");
  ev_run(loop, 0);

  dns_proxy_stop(&proxy);
  dns_proxy_cleanup(&proxy);

  return 0;
}
