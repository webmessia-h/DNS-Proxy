#include "../include/config.h"
#include "../include/dns-server.h"
#include "../include/hash.h"
#include "../include/https-client.h"
#include <signal.h>
#include <stdio.h>

static struct ev_loop *loop;
static dns_server server;

static void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents,
                      HashMap *map) {
  fprintf(stderr, "Received SIGINT, stopping...\n");
  ev_break(loop, EVBREAK_ALL);
  freeHashMap(map);
}

static HashMap blacklist_hashmap() {
  HashMap *map = createHashMap();
  for (int i = 0; i < BLACKLIST_SIZE; i++) {
    insert(map, BLACKLIST[i]);
  }
  return *map;
}

int main(void) {
  loop = EV_DEFAULT;

  ev_signal signal_watcher;
  ev_signal_init(&signal_watcher, sigint_cb, SIGINT);
  ev_signal_start(loop, &signal_watcher);
  HashMap map = blacklist_hashmap();
  dns_server_init(&server, loop, handle_dns_request, address, port, &map);

  printf("DNS proxy started. Press Ctrl+C to stop.\n");
  ev_run(loop, 0);

  dns_server_stop(&server);
  dns_server_cleanup(&server);

  return 0;
}
