#include "main_memory.h"

frame *main_memory = NULL;
int curr_page_num = 0;

t_list *pages=NULL;
t_list *program_list = NULL;

void muse_main_memory_init(int memory_size,int page_size)
{
	main_memory = (frame *)malloc(memory_size);
	page *p = (page*)malloc(memory_size / page_size); //cambiar esto
	program_list = list_create();

	void *mem_ptr = main_memory;
	int i;

	printf("Initializing frames\n");
	for(i=0;i<memory_size/page_size;i++)
	{	
		main_memory[i].metadata.is_free = true ;
		main_memory[i].metadata.size = 0;
		main_memory[i].data = mem_ptr + i*page_size;
	}
	i=0;
	printf("Initializing page table");
	pages = list_create();
	for(i=0;i<memory_size/page_size;i++)
	{
		p->is_present = true;
		p->page_num = curr_page_num;
		p->fr = &main_memory[i];
		curr_page_num += i;
		list_add(pages,p);
	}
}

void muse_main_memory_stop()
{
	free(main_memory);
	list_destroy(pages);
	list_destroy(program_list);
}

int search_program(uint32_t pid)
{
	int i = 0;
	program *p;
	while(i<list_size(program_list) )
	{
		p = list_get(program_list,i);
		if(p->pid == pid)
		{
			return i;
		}
		i++;
	}
	return -1;
}

int has_segment_with_free_space(program *p,int size)
{
	int i=0;
	segment *s;
	while(i < list_size(p->segment_table))
	{
		s = list_get(p->segment_table,i);
		if(s->free_size >= size)
		{
			return i;
		}
		i++;
	}
	return -1;
}

page *find_free_page()
{
	int i=0;
	page *p;
	while(i<list_size(pages))
	{
		p = list_get(pages,i);
		if(p->fr->metadata.is_free)
		{
			return p;
		}
	}
	return NULL;
}

uint32_t muse_malloc(int size,uint32_t pid)
{
	int prog_id;
	uint32_t seg_id;
	uint32_t page_id;
	uint32_t logical_address = 0;
	page *p;
	segment *s;
	program *prog;

	if((prog_id = search_program(pid)) != -1)
	{
		prog = list_get(program_list,prog_id);
		if((seg_id = has_segment_with_free_space(prog,size)) != -1 )
		{
			s = list_get(prog->segment_table,seg_id);
			p = find_free_page();
			if(p == NULL)
			{
				printf("All pages have been used!\n");
				return 0;
			}
			s->free_size -= 32;
			list_add(prog->segment_table,p);

		}
		else
		{
			s = (segment *)malloc(sizeof(segment));
			s->free_size = size - 32;
			s->page_table = list_create();
			page_id = list_add(s->page_table,find_free_page());
			seg_id = list_add(prog->segment_table,s);
			//logical_addr
		}
	}else
	{
		prog = (program *) malloc(sizeof(program));
		s = (segment *)malloc(sizeof(segment));
		s->free_size = size - 32;
		s->page_table = list_create();

		prog->pid = pid;
		prog->segment_table = list_create();

		page_id = list_add(s->page_table,find_free_page());
		seg_id = list_add(prog->segment_table,s);
		list_add(program_list,prog);
		logical_address = seg_id * 10 + page_id;
	}
	return logical_address;
}


uint8_t muse_free(uint32_t virtual_address,uint32_t pid)
{
	//uint8_t seg_index = virual_address >> ;
	//uint8_t page_index = virtual_address & PAGE_MASK >> 
	return 0;
}

uint32_t muse_get(void *dst,uint32_t src,size_t n,uint32_t pid)
{
	int i= search_program(pid);
	uint32_t destination = 0;

	program *prg = list_get(program_list,i);
	segment *s = list_get(prg->segment_table,0);
	page *p = list_get(s->page_table,0);
	memcpy(&destination,p->fr->data,n);
	return destination;
}

uint32_t muse_cpy(uint32_t dst,void *src,int n,uint32_t pid)
{
	int i= search_program(pid);
	program *prg = list_get(program_list,i);
	segment *s = list_get(prg->segment_table,0);
	page *p = list_get(s->page_table,0);
	memcpy(p->fr->data,src,n);
	
	return 0;
}

uint32_t muse_map(char *path,size_t length,int flags,uint32_t pid)
{
	return 0;
}

uint32_t muse_sync(uint32_t addr,size_t len,uint32_t pid)
{
	return 0;
}

int muse_unmap(uint32_t dir,uint32_t pid)
{
	return 0;
}

uint32_t muse_add_segment_to_program(program *p,int segm_size,uint32_t pid)
{
	return 0;
}



