#include "hash.h"
#include "log.h"

void add_blacklist_entry(const char *key) {
  LOG_DEBUG("add_blacklist_entry(key: %s)\n", key);
  hash_entry *entry = malloc(sizeof(hash_entry));
  if (entry) {
    entry->key = strdup(key);
    HASH_ADD_KEYPTR(hh, blacklist, entry->key, strlen(entry->key), entry);
  }
}

int find(const char *key) {
  LOG_DEBUG("find(key: %s)\n", key);
  hash_entry *entry;
  HASH_FIND_STR(blacklist, key, entry);
  return entry != NULL;
}

void delete_blacklist(void) {
  LOG_DEBUG("delete_blacklist()\n");
  hash_entry *current_entry, *tmp;
  HASH_ITER(hh, blacklist, current_entry, tmp) {
    HASH_DEL(blacklist, current_entry);
    free(current_entry->key);
    free(current_entry);
  }
}

void add_transaction_entry(transaction_info *tx) {
  LOG_DEBUG("add_transaction_entry(tx ptr: %p)\n", tx);
  transaction_hash_entry *entry = malloc(sizeof(transaction_hash_entry));
  if (entry) {
    entry->key = tx->original_tx_id;
    entry->value = tx;
    HASH_ADD(hh, transactions, key, sizeof(uint16_t), entry);
  }
}

transaction_info *find_transaction(uint16_t tx_id) {
  LOG_DEBUG("find_transaction(tx_id: %u)\n", tx_id);
  transaction_hash_entry *entry;
  HASH_FIND(hh, transactions, &tx_id, sizeof(uint16_t), entry);
  return entry ? entry->value : NULL;
}

void delete_transaction(uint16_t tx_id) {
  LOG_DEBUG("delete_transaction(tx_id: %u)\n", tx_id);
  transaction_hash_entry *entry;
  HASH_FIND(hh, transactions, &tx_id, sizeof(uint16_t), entry);
  if (entry) {
    free(entry->value); // Free the transaction_info struct
    HASH_DEL(transactions, entry);
    free(entry); // Free the hash entry
  }
}

void delete_all_transactions(void) {
  LOG_DEBUG("delete_all_transactions()\n");
  transaction_hash_entry *current_entry, *tmp;
  HASH_ITER(hh, transactions, current_entry, tmp) {
    HASH_DEL(transactions, current_entry);
    free(current_entry->value);
    free(current_entry);
  }
}
