
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <malloc.h>
#include <debug.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * Metadata stored immediately before each allocation.
 *
 * size: Payload capacity in bytes.
 * next: Next block in the allocator's linked list.
 * free: 1 if available for reuse, 0 if allocated.
 */
typedef struct block
{
    size_t size;
    struct block *next;
    int free;
} block_t;

/*
 * Head of the allocator's linked list.
 *
 * Assumptions:
 * - The allocator is not thread-safe.
 * - Allocations are managed using sbrk().
 * - Blocks are retained for reuse rather than returned to the OS.
 */
static block_t *head = NULL;

/**
 * Finds the first free block large enough for the requested size.
 *
 * @param size Minimum payload capacity required.
 * @return Suitable free block, or NULL if none exists.
 */
static block_t *find_free_block(size_t size)
{
    block_t *current = head;

    while (current != NULL)
    {
        assert(current->free == 0 || current->free == 1);

        if (current->free && current->size >= size)
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

/**
 * Appends a newly allocated block to the linked list.
 *
 * @param new_block Initialized block to append.
 */
static void append_block(block_t *new_block)
{
    assert(new_block != NULL);
    assert(new_block->next == NULL);
    assert(new_block->free == 0);

    if (head == NULL)
    {
        head = new_block;
        return;
    }

    block_t *current = head;

    while (current->next != NULL)
    {
        current = current->next;
    }

    current->next = new_block;
}

/**
 * Allocates at least s bytes of memory.
 *
 * Uses first-fit allocation to reuse available blocks.
 * Requests additional heap memory with sbrk() if necessary.
 *
 * @param s Number of payload bytes requested.
 * @return Pointer to usable memory, or NULL on failure
 *         or when s is zero.
 */
void *mymalloc(size_t s)
{
    if (s == 0)
    {
        return NULL;
    }

    /*
     * Ensure the header and payload fit in the signed
     * increment type accepted by sbrk().
     */
    if (s > (size_t)INTPTR_MAX - sizeof(block_t))
    {
        return NULL;
    }

    block_t *block = find_free_block(s);

    if (block != NULL)
    {
        block->free = 0;

        debug_printf("Malloc %zu bytes\n", s);

        return block + 1;
    }

    /*
     * Allocate space for both the metadata header
     * and the requested payload.
     */
    block = sbrk(sizeof(block_t) + s);

    if (block == (void *)-1)
    {
        return NULL;
    }

    block->size = s;
    block->next = NULL;
    block->free = 0;

    append_block(block);

    debug_printf("Malloc %zu bytes\n", s);

    return block + 1;
}

/**
 * Allocates zero-initialized memory for an array.
 *
 * Checks multiplication overflow before requesting memory
 * through mymalloc().
 *
 * @param nmemb Number of array elements.
 * @param s Size of each element in bytes.
 * @return Pointer to zero-initialized memory, or NULL
 *         on allocation failure or zero total size.
 */
void *mycalloc(size_t nmemb, size_t s)
{
    if (s != 0 && nmemb > SIZE_MAX / s)
    {
        return NULL;
    }

    size_t total = nmemb * s;

    void *p = mymalloc(total);

    if (p == NULL)
    {
        return NULL;
    }

    memset(p, 0, total);

    debug_printf("Calloc %zu bytes\n", total);

    return p;
}

/**
 * Marks an allocated block as available for reuse.
 *
 * The block remains in the linked list rather than
 * being returned to the operating system.
 *
 * @param ptr Pointer previously returned by this allocator,
 *            or NULL. Non-NULL pointers must refer to a
 *            valid allocation that has not already been freed.
 */
void myfree(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    /*
     * The metadata header is immediately before
     * the payload pointer returned by mymalloc().
     */
    block_t *block = (block_t *)ptr - 1;

    assert(block->free == 0);

    block->free = 1;

    debug_printf("Freed %zu bytes\n", block->size);
}
