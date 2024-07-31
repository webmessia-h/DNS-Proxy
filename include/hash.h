#ifndef HASH_H
#define HASH_H
#include "config.h"
#include "include.h"
#include "uthash.h"

typedef struct transaction_info {
  uint16_t original_tx_id;
  struct sockaddr *client_addr;
  socklen_t client_addr_len;
} transaction_info;

typedef struct {
  char *key;
  UT_hash_handle hh; // makes this structure hashable
} HashEntry;

typedef struct {
  uint16_t key;
  transaction_info *value;
  UT_hash_handle hh; // makes this structure hashable
} TransactionHashEntry;

HashEntry *blacklist = NULL;
TransactionHashEntry *transactions = NULL;

void add_blacklist_entry(const char *key);
int find(const char *key);
void delete_blacklist();
void add_transaction_entry(transaction_info *tx);
transaction_info *find_transaction(uint16_t tx_id);
void delete_transaction(uint16_t tx_id);
void delete_all_transactions();

#endif // HASH_H
