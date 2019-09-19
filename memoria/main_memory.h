#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include <inttypes.h>
#include <stddef.h>
#include <commons/collections/list.h>

typedef struct HeapMetadata
{
	uint32_t size;
	bool is_free;
}heap_metadata;


typedef struct page_s
{
	bool is_present;
	uint32_t frame;
}page;


typedef struct segment_s
{
	//segment_lock;
	heap_metadata metadata;
	uint16_t num_pages;
	uint32_t *base;
	uint32_t *limit;
}segment;

void muse_main_memory_init(int memory_size,int page_size);

void muse_main_memory_stop();


/* Allocates memory in UCM
 * @param size: size of the memory to allocate 
 * @return: virtual address
 */
uint32_t muse_malloc(int size);

/*
 * Frees a memory allocation
 * @param virtual_address: the address of memory to free
 * return: -1 -> SEGFAULT ,0 -> ok
 */
uint8_t muse_free(uint32_t virtual_address);

uint32_t muse_get(void *dst,uint32_t src,size_t n);

uint32_t muse_cpy(uint32_t dst,void *src,int n);

uint32_t muse_map(char *path,size_t length,int flags);

uint32_t muse_sync(uint32_t addr,size_t len);

int muse_unmap(uint32_t dir);

#endif
