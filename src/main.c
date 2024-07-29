#include "../include/config.h"
#include "../include/dns-server.h"
#include "../include/hash.h"
#include "../include/https-client.h"
#include <signal.h>
#include <stdio.h>

static struct ev_loop *loop;
static dns_server server;

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
  for (int i = 0; i < BLACKLIST_SIZE; i++) {
    insert(map, BLACKLIST[i]);
  }
  return map;
}

int main(void) {
  loop = EV_DEFAULT;
  HashMap *map = create_hash_map();
  blacklist_hashmap(map);
  if (map == NULL) {
    fprintf(stderr, "Failed to create blacklist hashmap\n");
    printf("sizeof map: %lu\t", sizeof(*map));
    return 1;
  }
  ev_signal signal_observer;

  ev_signal_init(&signal_observer, sigint_cb, SIGINT);
  ev_signal_start(loop, &signal_observer);
  dns_server_init(&server, loop, handle_dns_request, address, port,
                  BLACKLISTED_RESPONSE, map);

  printf("DNS proxy started. Press Ctrl+C to stop.\n");
  ev_run(loop, 0);

  dns_server_stop(&server);
  dns_server_cleanup(&server);

  return 0;
}
