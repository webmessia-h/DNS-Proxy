#include "../include/hash.h"
#include <stdio.h>

unsigned long hash(const char *str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + tolower(c); // hash * 33 + c
  }

  return hash % BLACKLISTED_DOMAINS;
}

HashMap *create_hash_map() {
  HashMap *map = malloc(sizeof(HashMap));
  if (map == NULL) {
    return NULL;
  }
  memset(map->table, 0, sizeof(map->table));
  return map;
}

void insert(HashMap *map, const char *key) {
  unsigned long index = hash(key);
  Node *newNode = malloc(sizeof(Node));
  if (newNode == NULL) {
    return;
  }
  newNode->key = strdup(key);
  newNode->next = map->table[index];
  map->table[index] = newNode;
}

// strcasecmp-like function
int compare(const char *s1, const char *s2) {
  while (*s1 && *s2) {
    int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
    if (diff != 0) {
      return diff;
    }
    s1++;
    s2++;
  }
  return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int search(HashMap *map, const char *key) {
  unsigned long index = hash(key);
  Node *current = map->table[index];
  while (current != NULL) {
    if (compare(current->key, key) == 0) {
      return 1; // Found
    }
    current = current->next;
  }
  return 0; // Not found
}

void free_hash_map(HashMap *map) {
  for (int i = 0; i < BLACKLISTED_DOMAINS; i++) {
    Node *current = map->table[i];
    while (current != NULL) {
      Node *temp = current;
      current = current->next;
      free(temp->key);
      free(temp);
    }
  }
  free(map);
}
