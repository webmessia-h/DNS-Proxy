#ifndef CONFIG_H
#define CONFIG_H

// Upstream DNS resolver adress
extern const char *upstreamDNS;
// Proxy adress
extern const char *address;
// Proxy port
extern const int port;
#define BLACKLIST_SIZE 3 // count of unwanted domains
// List of unwanted domains
extern const char *BLACKLIST[BLACKLIST_SIZE];
// Response if accessed unwanted domain
#define BLACKLISTED_RESPONSE "no bad domains today buddy"

#define BUFFER_SIZE /*desired buffer size*/

#endif // CONFIG_H
