#ifndef CONFIG_H
#define CONFIG_H

// Proxy adress
const char adress[] = "127.0.0.1";

// Proxy port
const int port = 53;

#define BLACKLIST_SIZE 3 // count of unwanted domains
// List of unwanted domains
const char *BLACKLIST[BLACKLIST_SIZE] = {"somebadsite.com", "microsoft.com",
                                         ""};
// Response if accessed unwanted domain
#define BLACKLISTED_RESPONSE "no bad domains today buddy"

#define BUFFER_SIZE /*desired buffer size*/

#endif // CONFIG_H
