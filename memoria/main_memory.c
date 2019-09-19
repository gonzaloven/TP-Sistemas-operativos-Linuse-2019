#include "main_memory.h"
#include <stdio.h>
#include <stdlib.h>

uint16_t FRAME_COUNT = 0;
uint16_t PAGE_SIZE = 0;

uint32_t *main_memory = NULL;

uint32_t *START_INDEX = NULL;
uint32_t *END_INDEX = NULL;

uint32_t curr_mem_ptr = NULL;
t_list *segment_table = NULL;
t_list *page_table = NULL;

int muse_get_pagenum(int vadd)
{
	return 0;
}

int muse_get_offset(int vadd)
{
	return 0;
}	

int muse_get_segmentnum(int vadd)
{
	return 0;
}

void muse_main_memory_init(int alloc_size,int page_size)
{
	segment_table = list_create();
	main_memory = malloc(alloc_size);
	curr_mem_ptr = main_memory;

	START_INDEX = main_memory;
	END_INDEX = main_memory + alloc_size;
	PAGE_SIZE = page_size;
	FRAME_COUNT = alloc_size / PAGE_SIZE;

	return;
}

void muse_main_memory_stop()
{
	free(main_memory);
	list_destroy(segment_table);
	return;
}

uint32_t muse_malloc(int size)
{
	segment seg;
	page pag;
	uint32_t laddr = 90;

	seg.metadata.size = size;
	seg.metadata.is_free = false;
	seg.base = curr_mem_ptr;
	//if(size > MAX_MEMORY_SIZE)
	seg.limit = curr_mem_ptr+size;
	
	//lock
	list_add(segment_table,&seg);
	//free
	return laddr;
}

uint8_t muse_free(uint32_t virtual_address)
{
	printf("\nmuse_free()\n");
	return 0;
}

uint32_t muse_get(void *dst,uint32_t src,size_t n)
{
	//TODO
	return 0;
}

uint32_t muse_cpy(uint32_t dst,void *src,int n)
{
	//TODO
	return 0;
}

uint32_t muse_map(char *path,size_t length,int flags)
{
	//TODO
	return 0;
}

uint32_t muse_sync(uint32_t addr,size_t len)
{
	//TODO
	return 0;
}

int muse_unmap(uint32_t dir)
{
	//TODO
	return 0;
}
