#include "../include/hash.h"
void add_blacklist_entry(const char *key) {
  HashEntry *entry = malloc(sizeof(HashEntry));
  if (entry) {
    entry->key = strdup(key);
    HASH_ADD_KEYPTR(hh, blacklist, entry->key, strlen(entry->key), entry);
  }
}

int find(const char *key) {
  HashEntry *entry;
  HASH_FIND_STR(blacklist, key, entry);
  return entry != NULL;
}

void delete_blacklist() {
  HashEntry *current_entry, *tmp;
  HASH_ITER(hh, blacklist, current_entry, tmp) {
    HASH_DEL(blacklist, current_entry);
    free(current_entry->key);
    free(current_entry);
  }
}

void add_transaction_entry(transaction_info *tx) {
  TransactionHashEntry *entry = malloc(sizeof(TransactionHashEntry));
  if (entry) {
    entry->key = tx->original_tx_id;
    entry->value = tx;
    HASH_ADD(hh, transactions, key, sizeof(uint16_t), entry);
  }
}

transaction_info *find_transaction(uint16_t tx_id) {
  TransactionHashEntry *entry;
  HASH_FIND(hh, transactions, &tx_id, sizeof(uint16_t), entry);
  return entry ? entry->value : NULL;
}

void delete_transaction(uint16_t tx_id) {
  TransactionHashEntry *entry;
  HASH_FIND(hh, transactions, &tx_id, sizeof(uint16_t), entry);
  if (entry) {
    HASH_DEL(transactions, entry);
    free(entry);
  }
}

void delete_all_transactions() {
  TransactionHashEntry *current_entry, *tmp;
  HASH_ITER(hh, transactions, current_entry, tmp) {
    HASH_DEL(transactions, current_entry);
    free(current_entry);
  }
}
