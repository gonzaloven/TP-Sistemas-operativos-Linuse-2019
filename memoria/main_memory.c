/*
* main_memory.c
* Estructura administrativa de la memoria,
* Permite inizializar memoria, liberarla, 
* buscar un programa en la tabla de programas, 
* y buscar páginas libre
*/

#include "main_memory.h"

frame *main_memory = NULL;

t_list *program_list = NULL;
t_list *page_list = NULL;
t_list *segment_list = NULL;
t_log *metricas_logger = NULL;

int PAGE_SIZE = 0;

void muse_main_memory_init(int memory_size, int page_size)
{
	int i;
	int cant_frames_totales;
	int cant_pags_totales;
	int curr_page_num;	
	void *mem_ptr = main_memory;	
	
	PAGE_SIZE = page_size;
	main_memory = (frame *) malloc(memory_size); // aka UPCM
	cant_frames_totales = (memory_size / page_size);
	cant_pags_totales = (memory_size / page_size);	//TODO Esto estaría mal	

	program_list = list_create();
	page_list = list_create();
	segment_list = list_create();

	metricas_logger = log_create(MUSE_LOG_PATH, "METRICAS", true, LOG_LEVEL_TRACE);

	printf("Dividiento la memoria en frames...\n");
	for(i=0; i<cant_frames_totales; i++)
	{	
		main_memory[i].metadata.is_free = true ;
		main_memory[i].metadata.size = 0;
		main_memory[i].data = mem_ptr + i*page_size; 
		// supongamos que la main_memory[0] apunta a la posicion 0 en memoria
		// y que los frames son de 32 bytes
		// main_memory[0].data va a ser igual a 0 + 0 * 32 = 0
		// main_memory[1].data va a ser igual a 0 + 1 * 32 = 32
		//osea data vendría a ser la base de los frames
	}		

	printf("Inicializando tabla de paginas...\n");
	page *pag = (page *) malloc(page_size); //cant_pags_totales se cambió por page_size
	
	for(curr_page_num=0; curr_page_num < cant_pags_totales; curr_page_num++)
	{
		/*	Se modificó xq según lo que había hecho el otro flaco:

		pag->is_present = true;
		pag->page_num = curr_page_num;
		pag->fr = &main_memory[i];
		curr_page_num += i;
		list_add(page_list, pag);

		La página 0 apunta al frame &main_memory[0], la próxima página será:
		La página 1 apunta al frame &main_memory[1], la próxima página será:
		La página 2 apunta al frame &main_memory[2], la próxima página será:
		La página 4 apunta al frame &main_memory[3], la próxima página será:
		La página 7 apunta al frame &main_memory[4], ???
		*/

		pag->is_present = true;
		pag->page_num = curr_page_num;
		pag->fr = &main_memory[curr_page_num];		
		list_add(page_list, pag);
	}

	log_trace(metricas_logger, "Cantidad Total de Memoria:%d ", memory_size);
	log_trace(metricas_logger, "Cantidad Total de Frames:%d ", cant_frames_totales);
}

void muse_main_memory_stop()
{
	free(main_memory);
	list_destroy(page_list);
	list_destroy(program_list);
	list_destroy(segment_list);
}

int search_program(uint32_t pid)
{
	int i=0;
	program *prog;
	while(i<list_size(program_list))
	{
		prog = list_get(program_list, i);
		if(prog->pid == pid) {return i;}
		i++;
	}
	return -1;
}

page *find_free_page()
{
	int i=0;
	int page_list_size = list_size(page_list)
	page *pag;
	while(i < page_list_size)
	{
		pag = list_get(page_list, i);
		if(pag->fr->metadata.is_free) {return pag;}
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
	int total_size;
	int total_pages_needed;

	total_pages_needed = (size / PAGE_SIZE) + ((size % PAGE_SIZE) != 0); // ceil( size / PAGE_SIZE )
	total_size = total_pages_needed * PAGE_SIZE;

	if((prog_id = search_program(pid)) == -1)
	{
		//si el programa no está en la lista de programas	
		//lo creamos en memoria y creamos un nuevo segmento

		prog = (program *) malloc(sizeof(program));
		seg = (segment *) malloc(sizeof(segment));

		//OLD: seg->free_size = size - 32;

		prog->pid = pid;
		prog->segment_table = list_create();
		
		seg->free_size = total_size;
		seg->page_table = list_create();
		
		seg_id = list_add(prog->segment_table, seg);
		page_id = list_add(seg->page_table, find_free_page());
		
		list_add(program_list, prog);

		logical_address = seg_id * 10 + page_id; //TODO de dónde sale este cálculo
		
	}else /*	si el programa está en la lista de programas */		
	{  	
		prog = list_get(program_list, prog_id);
		if((seg_id = segment_with_free_space(prog, size)) != -1 ) //si hay espacio en su segmento
		{
			seg = list_get(prog->segment_table, seg_id);	//seg = segmento con espacio
			pag = find_free_page();
			if(pag == NULL)
			{
				printf("All pages have been used!\n");
				return 0;
			}
			seg->free_size -= 32;
			list_add(prog->segment_table, pag);
		}
		else //si no hay espacio en su segmento mando el programa a otro segmento
		{
			seg = (segment *) malloc(sizeof(segment));
			seg->free_size = size - 32;
			seg->page_table = list_create();
			page_id = list_add(seg->page_table, find_free_page());
			seg_id = list_add(prog->segment_table, seg);
			logical_address = seg_id * 10 + page_id; //TODO chequear
		}
	}	  	
	return logical_address;
}

int segment_with_free_space(program *prog, int size)
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


uint8_t memory_free(uint32_t virtual_address, uint32_t pid)
{
	//uint8_t seg_index = virual_address >> ;
	//uint8_t page_index = virtual_address & PAGE_MASK >> 
	return 0;
}

// No se usan ni dst ni src
uint32_t memory_get(void *dst, uint32_t src, size_t numBytes, uint32_t pid)
{
	uint32_t destination = 0;

	// Busco el programa, despues busco el segmento 0 del programa y de ese segmento busco su página 0
	program *prg = list_get(program_list, search_program(pid));
	segment *seg = list_get(prg->segment_table, 0);
	page *pag = list_get(seg->page_table, 0);

	// copio "numBytes" bytes desde la dirección "pag->fr->data" a la direccion &0 de la memoria de MUSE
	memcpy(&destination, pag->fr->data, numBytes);
	return destination;

	//How does memcpy works
	// Copies "numBytes" bytes from address "from" to address "to"
	void * memcpy(void *to, const void *from, size_t numBytes);
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
	// Apalancándonos en el mismo mecanismo que permite el swapping de páginas,
	// la funcionalidad de memoria compartida que proveerá MUSE (a través de sus 
	// funciones de muse_map) se realizará sobre un archivo compartido, en vez 
	// del archivo de swap. Esta distinción deberá estar plasmada en la tabla de segmentos.

	/* Idea Original:*/
	int file_descriptor = open(path, O_RDONLY);
   	void* map = mmap(NULL, length, PROT_NONE, flags, file_descriptor, 0);
   	printf(map, length);   
	return *(int*) map; 
}

uint32_t memory_sync(uint32_t addr, size_t len, uint32_t pid)
{
	return 0;
}

int memory_unmap(uint32_t dir, uint32_t pid)
{
	/* Idea Original:*/
	void *dir2 = &dir;
	int unmap_result = munmap(dir2,  1 << 10); //TODO: ¿¿ 1 << 10 ??
  	if (unmap_result == 0 ) {
		printf("Could not unmap");
		//log_error(muse_logger,"Could not unmap");
		return -1;
	}
	return 0;
}

uint32_t muse_add_segment_to_program(program *prog, int segm_size, uint32_t pid)
{
	return 0;
}
