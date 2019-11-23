/*
* main_memory.c
* Estructura administrativa de la memoria,
* Permite inizializar memoria, liberarla, 
* buscar un programa en la tabla de programas, 
* y buscar páginas libre
*/

#include "main_memory.h"

frame *main_memory = NULL;
int curr_page_num = 0;

t_list *page_list=NULL;
t_list *program_list = NULL;

t_log *metricas_logger = NULL;

void muse_main_memory_init(int memory_size, int page_size)
{
	metricas_logger = log_create(MUSE_LOG_PATH, "METRICAS", true, LOG_LEVEL_TRACE);

	main_memory = (frame *)malloc(memory_size);
	int cant_pags_totales = (memory_size / page_size);
	page *pag = (page*)malloc(cant_pags_totales); //cambiar esto
	program_list = list_create();

	void *mem_ptr = main_memory;
	int i;

	log_trace(metricas_logger, "Cantidad Total de paginas:%d ", cant_pags_totales);

	printf("Dividiento la memoria en frames...\n");
	for(i=0;i<cant_pags_totales;i++)
	{	
		main_memory[i].metadata.is_free = true ;
		main_memory[i].metadata.size = 0;
		main_memory[i].data = mem_ptr + i*page_size;
	}		

	printf("Inicializando tabla de paginas...\n");
	page_list = list_create();
	for(i=0;i<cant_pags_totales;i++)
	{
		pag->is_present = true;
		pag->page_num = curr_page_num;
		pag->fr = &main_memory[i];
		curr_page_num += i;
		list_add(page_list, pag);
	}
}

void muse_main_memory_stop()
{
	free(main_memory);
	list_destroy(page_list);
	list_destroy(program_list);
}

int search_program(uint32_t pid)
{
	int i=0;
	program *prog;
	while(i<list_size(program_list) )
	{
		prog = list_get(program_list, i);
		if(prog->pid == pid) {return i;}
		i++;
	}
	return -1;
}

int has_segment_with_free_space(program *prog, int size)
{
	int i=0;
	segment *seg;
	while(i < list_size(prog->segment_table))
	{
		seg = list_get(prog->segment_table, i);
		if(seg->free_size >= size) {return i;}
		i++;
	}
	return -1;
}

page *find_free_page()
{
	int i=0;
	page *pag;
	while(i<list_size(page_list))
	{
		pag = list_get(page_list, i);
		if(pag->fr->metadata.is_free)
		{
			return pag;
		}
	}
	return NULL;
}

uint32_t memory_malloc(int size, uint32_t pid)
{
	int prog_id;
	uint32_t seg_id;
	uint32_t page_id;
	uint32_t logical_address = 0;
	page *pag;
	segment *seg;
	program *prog;

	if((prog_id = search_program(pid)) != -1) //si el programa esta en program list
	{
		prog = list_get(program_list, prog_id);
		if((seg_id = has_segment_with_free_space(prog, size)) != -1 ) //si hay espacio en su segmento
		{
			seg = list_get(prog->segment_table, seg_id);
			pag = find_free_page();
			if(pag == NULL)
			{
				printf("All pages have been used!\n");
				return 0;
			}
			seg->free_size -= 32;
			list_add(prog->segment_table, pag);

		}
		else //mando el programa a otro segmento
		{
			seg = (segment *)malloc(sizeof(segment));
			seg->free_size = size - 32;
			seg->page_table = list_create();
			page_id = list_add(seg->page_table, find_free_page());
			seg_id = list_add(prog->segment_table, seg);
			logical_address = seg_id * 10 + page_id; //TODO chequear
		}
	}else /*	si el programa no está en la lista de programas 
				creamos el programa y creamos un nuevo segmento
		   */
	{
		prog = (program *) malloc(sizeof(program));
		seg = (segment *)malloc(sizeof(segment));
		seg->free_size = size - 32;
		seg->page_table = list_create();

		prog->pid = pid;
		prog->segment_table = list_create();

		page_id = list_add(seg->page_table, find_free_page());
		seg_id = list_add(prog->segment_table, seg);
		list_add(program_list, prog);
		logical_address = seg_id * 10 + page_id; //TODO de dónde sale este cálculo
	}
	return logical_address;
}


uint8_t memory_free(uint32_t virtual_address, uint32_t pid)
{
	//uint8_t seg_index = virual_address >> ;
	//uint8_t page_index = virtual_address & PAGE_MASK >> 
	return 0;
}

//No se que es dst pero no lo usa, idem src
uint32_t memory_get(void *dst, uint32_t src, size_t n, uint32_t pid)
{
	int i= search_program(pid);
	uint32_t destination = 0;

	program *prg = list_get(program_list, i);
	segment *seg = list_get(prg->segment_table, 0);
	page *pag = list_get(seg->page_table, 0);
	memcpy(&destination, pag->fr->data, n);
	return destination;
}

uint32_t memory_cpy(uint32_t dst, void *src, int n, uint32_t pid)
{
	int i= search_program(pid);
	program *prg = list_get(program_list, i);
	segment *seg = list_get(prg->segment_table, 0);
	page *pag = list_get(seg->page_table, 0);
	memcpy(pag->fr->data, src, n);
	
	return 0;
}

uint32_t memory_map(char *path, size_t length, int flags, uint32_t pid)
{
	/* Idea Original:
	 int file_descriptor = open(path, O_RDONLY);
   	void* map = mmap(NULL, length, PROT_NONE, flags, file_descriptor, 0);
   	printf(map, length);   
	return *(int*) map; */
	
	return 0;
}

uint32_t memory_sync(uint32_t addr, size_t len, uint32_t pid)
{
	return 0;
}

int memory_unmap(uint32_t dir, uint32_t pid)
{
	/* Idea Original:
	void *dir2 = &dir;
	int unmap_result = munmap(dir2,  1 << 10); //TODO: ¿¿ 1 << 10 ??
  	if (unmap_result == 0 ) {
		printf("Could not unmap");
		//log_error(muse_logger,"Could not unmap");
		return -1;
	}
	return 0;*/
}

uint32_t muse_add_segment_to_program(program *prog, int segm_size, uint32_t pid)
{
	return 0;
}
