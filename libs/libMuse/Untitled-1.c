// fork from https://github.com/danluu/malloc-tutorial/blob/master/malloc.c

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
// Don't include stdlb since the names will conflict?

struct block_meta
{
    size_t size;
    struct block_meta* next;
    int free;
};

#define META_SIZE sizeof(struct block_meta)

void* global_base = NULL;

// Iterate through blocks until we find one that's large enough.
// TODO: split block up if it's larger than necessary
struct block_meta* find_free_block(struct block_meta** last, size_t size)
{
    struct block_meta* current = global_base;
    while (current && !(current->free && current->size >= size))
    {
        *last = current;
        current = current->next;
    }
    return current;
}

struct block_meta* request_space(struct block_meta* last, size_t size)
{
    struct block_meta* block;
    block = sbrk(0);
    void* request = sbrk(size + META_SIZE);
    if (request == (void*)-1)
    {
        return NULL; // sbrk failed.
    }

    if (last)
    { // NULL on first request.
        last->next = block;
    }
    block->size = size;
    block->next = NULL;
    block->free = 0;
    return block;
}

// If it's the first ever call, global_base == NULL, request_space and set global_base
// Otherwise, if we can find a free block, use it.
// If not, request_space.
uint32_t malloc(size_t size)
{
    if (size <= 0)
    {
        return NULL;
    }

    struct block_meta* block;

    int nro_prog;
	uint32_t seg_id;
	uint32_t page_id;
	uint32_t offset = 0; //TODO: esto debería cambiarlo
	uint32_t logical_address = 0;
	page *pag;
	segment *seg;
	program *prog;
	int total_size = size + METADATA_SIZE;
	int total_pages_needed;
	int espacio_usado_de_ultima_pagina;
	
	total_pages_needed = (total_size / PAGE_SIZE) + ((total_size % PAGE_SIZE) != 0); // ceil(total_size / PAGE_SIZE)
	
    if (!global_base)
    { // First call.
        block = request_space(NULL, size);
        if (!block) return NULL;        
        else global_base = block;
    }
    else
    {
        struct block_meta* last = global_base;
        block = find_free_block(&last, size);
        if (!block)
        { // Failed to find free block.
            block = request_space(last, size);
            if (!block) return NULL;
        }
        else
        { // Found free block
            block->free = 0;
        }
    }

    return &(block + 1);
}

void* calloc(size_t nelem, size_t elsize)
{
    size_t size = nelem * elsize;
    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

// TODO: maybe do some validation here.
struct block_meta* get_block_ptr(void* ptr)
{
    return (struct block_meta*)ptr - 1;
}

void free(void* ptr)
{
    if (!ptr) return;
    
    // TODO: consider merging blocks once splitting blocks is implemented.
    struct block_meta* block_ptr = get_block_ptr(ptr);
    block_ptr->free = 1;
}

void* realloc(void* ptr, size_t size)
{
    if (!ptr)
    {
        // NULL ptr. realloc should act like malloc.
        return malloc(size);
    }

    struct block_meta* block_ptr = get_block_ptr(ptr);
    if (block_ptr->size >= size)
    {
        // We have enough space. Could free some once we implement split.
        return ptr;
    }

    // Need to really realloc. Malloc new space and free old space.
    // Then copy old data to new space.
    void* new_ptr;
    new_ptr = malloc(size);
    if (!new_ptr)
    {
        return NULL; // TODO: set errno on failure.
    }
    memcpy(new_ptr, ptr, block_ptr->size);
    free(ptr);
    return new_ptr;
}