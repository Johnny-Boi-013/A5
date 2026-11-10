#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include <malloc.h>
#include <stdio.h>

// Include any other headers we need here

#include <unistd.h>
#include <string.h>
#include <stdint.h>

// NOTE: You should NOT include <stdlib.h> in your final implementation.
// malloc.h turns every call to malloc/calloc/free in this file into a call
// to mymalloc/mycalloc/myfree, so calling them here would recurse forever.

#include <debug.h> // definition of debug_printf

typedef struct block
{
  size_t size;
  struct block *next;
  int free;
} block_t;

static block_t *head = NULL;

void *mymalloc(size_t s) {
  if (s == 0)
  {
    return NULL;
  }

  if (s > SIZE_MAX - sizeof(block_t))
  {
    return NULL;
  }

  block_t *current = head;

  while (current != NULL)
  {
    if (current->free == 1 && current->size >= s)
    {
      current->free = 0;

      return current + 1;
    }

    current = current->next;
  }

  block_t *new_block = sbrk(s + sizeof(block_t));

  if (new_block == (void *) - 1)
  {
    return NULL;
  }

  new_block->size = s;
  new_block->next = NULL;
  new_block->free = 0;

  if (head == NULL)
  {
    head = new_block;
  } else
  {
    block_t *current = head;

    while (current->next != NULL)
    {
      current = current->next;
    }

    current->next = new_block;
  }

  debug_printf("Malloc %zu bytes\n", s);

  return new_block + 1;
}

void *mycalloc(size_t nmemb, size_t s) {
  if (s != 0 && nmemb > SIZE_MAX / s) {
    return NULL;
  }

  size_t total = nmemb * s;

  void *p = mymalloc(total);

  if (p == NULL)
  {
    return NULL;
  }
}

void myfree(void *ptr) {
  if (ptr == NULL)
  {
    return;
  }

  block_t *block = (block_t *)ptr - 1;

  block->free = 1;

  size_t size = block->size;

  debug_printf("Freed %zu bytes\n", size);
}
