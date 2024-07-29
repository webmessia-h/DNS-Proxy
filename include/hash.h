#include "../include/config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
  char *key;
  struct Node *next;
} Node;

typedef struct {
  Node *table[BLACKLIST_SIZE];
} HashMap;

unsigned long hash(const char *str);

HashMap *createHashMap();

void insert(HashMap *map, const char *key);

int compare(const char *s1, const char *s2);

int search(HashMap *map, const char *key);

void freeHashMap(HashMap *map);
