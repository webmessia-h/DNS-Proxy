#ifndef HASH_H
#define HASH_H
#include "config.h"
#include "include.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_MAP_SIZE 32

typedef struct Node {
  char *key;
  struct Node *next;
} Node;

typedef struct {
  Node *table[HASH_MAP_SIZE];
} HashMap;

// Transaction info struct
#pragma pack(push, 1)
typedef struct transaction_info {
  uint16_t original_tx_id;
  struct sockaddr *client_addr;
  socklen_t client_addr_len;
} transaction_info;

typedef struct TransactionNode {
  uint16_t key; // Using original_tx_id as key
  transaction_info value;
  struct TransactionNode *next;
} TransactionNode;
#pragma pack(pop)

typedef struct {
  TransactionNode *table[HASH_MAP_SIZE];
} TransactionHashMap;

TransactionHashMap *create_transaction_hash_map();

void insert_transaction(TransactionHashMap *map, const transaction_info tx);

transaction_info *search_transaction(TransactionHashMap *map,
                                     uint16_t original_tx_id);

void free_transaction_hash_map(TransactionHashMap *map);

unsigned long hash(const char *str);

HashMap *create_hash_map();

void insert(HashMap *map, const char *key);

int compare(const char *s1, const char *s2);

int search(HashMap *map, const char *key);

void free_hash_map(HashMap *map);
#endif // HASH_H
