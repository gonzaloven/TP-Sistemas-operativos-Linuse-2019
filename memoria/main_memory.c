/*
* main_memory.c
* Estructura administrativa de la memoria,
* Permite inizializar memoria, liberarla, 
* buscar un programa en la tabla de programas, 
* y buscar páginas libre
*/

#include "main_memory.h"
#include <fcntl.h> //for open() funct
#include <sys/mman.h> //for mmap() & munmap()
#include <sys/stat.h>

#define METADATA_SIZE sizeof(struct HeapMetadata)

void *MAIN_MEMORY = NULL;

t_list *program_list = NULL;
t_list *segment_list = NULL;
t_log *metricas_logger = NULL;
t_log *debug_logger = NULL;

int PAGE_SIZE = 0;
int TOTAL_FRAME_NUM = 0;
bool BITMAP[];


void muse_main_memory_init(int memory_size, int page_size)
{
	int i;
	int curr_page_num;	
	void *mem_ptr = MAIN_MEMORY;
	PAGE_SIZE = page_size;
	MAIN_MEMORY = (void *) malloc(memory_size); // aka UPCM
	metadata = (heap_metadata*) malloc (METADATA_SIZE);
	TOTAL_FRAME_NUM = (memory_size / PAGE_SIZE);
	BITMAP[TOTAL_FRAME_NUM]; //1 if free;

	program_list = list_create();
	segment_list = list_create();

	debug_logger = log_create(MUSE_LOG_PATH,"DEBUG", true,LOG_LEVEL_TRACE);
	metricas_logger = log_create(MUSE_LOG_PATH, "METRICAS", true, LOG_LEVEL_TRACE);

	printf("Dividiento la memoria en frames...\n");
	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{	
		//MAIN_MEMORY[i] = mem_ptr + i * PAGE_SIZE;
		BITMAP[i] = 1;
	}

	log_trace(metricas_logger, "Tamaño de pagina = tamaño de frame: %d", PAGE_SIZE);
	log_trace(metricas_logger, "Tamaño de metadata: %d", METADATA_SIZE);	
	log_trace(metricas_logger, "Cantidad Total de Memoria:%d", memory_size);
	number_of_free_frames();	
}

void muse_main_memory_stop()
{
	int i;
	for(i=0; i<TOTAL_FRAME_NUM; i++)
		free MAIN_MEMORY[i];
	free(MAIN_MEMORY);

	list_destroy(program_list);
	list_destroy(segment_list);

	log_destroy(metricas_logger);
	log_destroy(debug_logger);
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

int number_of_free_frames(){
	int memoria_libre = 0; 
	int frames_libres = 0;
	int i;

	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{	
		frames_libres += (BITMAP[i]);
	}

	memoria_libre = frames_libres * PAGE_SIZE;

	log_trace(metricas_logger, "Cantidad de Memoria libre:%d ", memoria_libre);	
	log_trace(metricas_logger, "Cantidad de Frames libres:%d / %d ", frames_libres, TOTAL_FRAME_NUM);

	return frames_libres;
}

page *page_with_free_size(int size)
{
	if (size>PAGE_SIZE)
	{
		log_debug(debug_logger, "No podes pedir frames de %d",size);
		return NULL;
	}

	metadata = (heap_metadata*) malloc (METADATA_SIZE);

	int curr_frame_num;

	page *pag = (page *) malloc(PAGE_SIZE); 

	for(curr_frame_num=0; curr_frame_num < TOTAL_FRAME_NUM; curr_frame_num++)
	{	
		if (BITMAP[curr_frame_num])
		{
			BITMAP[curr_frame_num] = 0;
			pag->is_present = true;
			pag->is_modify = false;
			pag->fr = &MAIN_MEMORY[curr_frame_num];
			MAIN_MEMORY[curr_frame_num] = metadata;
			metadata->is_free=0;
			metadata->free_size= PAGE_SIZE-size;

			return pag;
		}
	}

	log_debug(debug_logger, "Nos quedamos sin frames libres");
	return NULL;
}

void free_page(page *pag)
{
	int curr_frame_num;

	for(curr_frame_num=0; curr_frame_num < TOTAL_FRAME_NUM; curr_frame_num++)
	{	
		if (pag->fr == &MAIN_MEMORY[curr_frame_num])
		{
			BITMAP[curr_frame_num] = 0;
			MAIN_MEMORY[curr_frame_num] = metadata;
			metadata->is_free = 0;
			pag->is_present = true;
			pag->is_modify = false;
			pag->fr = NULL;
		}
	}	
}

void metricas_por_socket_conectado(uint32_t pid){
	int nro_prog = search_program(pid);
	program *prog = list_get(program_list, nro_prog);

	int nro_de_seg;

	int cantidad_de_segmentos_asignados = list_size(prog->segment_table);
	int cantidad_de_segmentos_totales = list_size(segment_list);

	log_trace(metricas_logger, "El programa n° %d tiene asignados %d de %d segmentos en el sistema", 
			nro_prog, cantidad_de_segmentos_asignados, cantidad_de_segmentos_totales);
	number_of_free_frames();
}

uint32_t memory_malloc(int size, uint32_t pid)
{	
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
	
	if((nro_prog = search_program(pid)) == -1)
	{
		//si el programa no está en la lista de programas	
		//agregamos el programa a la lista de programas y le creamos un nuevo segmento

		prog = (program *) malloc(sizeof(program));
		
		prog->pid = pid;
		prog->segment_table = list_create();
		nro_prog = list_add(program_list, prog);	

		log_debug(debug_logger, "Se creo el prog n°%d de la lista de programas ", nro_prog);
	}	 

	prog = list_get(program_list, nro_prog);

	seg_id = segment_with_free_space(prog, size);
	if(seg_id != -1) 
	{	//si hay espacio en su segmento, malloqueo ahí
		
		seg = list_get(prog->segment_table, seg_id);	//seg = segmento con espacio		
		seg->free_size -= size;
		list_add(prog->segment_table, pag);		
	}
	
	
	/* TODO: crear caso
	else if (puedo_agrandar_un_segmento)
		 lo agrando 
	*/

	else 	//si no hay espacio en su último segmento, le creo uno
	{

		total_size = (total_pages_needed * METADATA_SIZE) + size;
		espacio_usado_de_ultima_pagina = (total_size % PAGE_SIZE );
		
		log_debug(debug_logger, "Se pidieron %d bytes que vendrian a ser %d pagina/s ", size, (size/PAGE_SIZE)+1);

		if(total_pages_needed>((size/PAGE_SIZE)+1))
		{
			log_debug(debug_logger, "Sin embargo, si consideramos que la metadata ocupa %d bytes", METADATA_SIZE);
			log_debug(debug_logger, "Se necesitarían %d bytes osea %d pagina/s y la ultima pagina solo usariía %d bytes", 
				total_size, total_pages_needed),espacio_usado_de_ultima_pagina;
		}

		seg = (segment *) malloc(sizeof(segment));
		seg->free_size = PAGE_SIZE - espacio_usado_de_ultima_pagina;
		seg->page_table = list_create();
		seg->is_heap = true;
		
		if (total_pages_needed>1)
		{
			pag = page_with_free_size(PAGE_SIZE)
			//page_id = prog.segmento_utilizado.page_table.1er_pagina_pedida
			page_id = list_add(seg->page_table, pag);

			//vamos a pedir todo el resto de paginas que necesitemos
			for(int i=0 ; i < total_pages_needed-2 ; i++ )
			{
				pag = page_with_free_size(PAGE_SIZE);
				list_add(seg->page_table, pag);
			}

			list_add(seg->page_table, page_with_free_size(espacio_usado_de_ultima_pagina));
		}
		else
		{
			//page_id = prog.segmento_utilizado.page_table.1er_pagina_pedida
			page_id = list_add(seg->page_table, page_with_free_size(espacio_usado_de_ultima_pagina));
		}
		
		// seg_id = prog.segment_table.segmento_pedido
		seg_id = list_add(prog->segment_table, seg);

		number_of_free_frames();

		list_add(segment_list, seg);	
	}
	
	metricas_por_socket_conectado(pid);
	logical_address = (page_id * PAGE_SIZE) + offset; //TODO ??
	return logical_address;
}

int segment_with_free_space(program *prog, int size)
{
	int i=0;
	segment *seg;
	int cantidadDeSegmentos = list_size(prog->segment_table);

	if (cantidadDeSegmentos==0) return -1;

	int max_free_size = 0;
	int total_free_size = 0;	

	while(i < cantidadDeSegmentos)
	{
		seg = list_get(prog->segment_table, i);
		total_free_size += seg->free_size;
		if(seg->free_size >= max_free_size) max_free_size = seg->free_size;
		if(seg->free_size >= size) {return i;}
		i++;
	}
	log_debug(debug_logger, "No habia ningun segmento con %d de espacio libre, el maximo fue de %d", size, max_free_size);
	log_debug(debug_logger, "Se encontraron en total %d bytes libres en el programa", total_free_size);
	return -1;
}


uint8_t memory_free(uint32_t virtual_address, uint32_t pid)
{
	int nro_prog = search_program(pid);
	program *prog;
	segment *seg;
	page *pag;	
	prog = list_get(program_list, nro_prog);
	int nro_seg = busca_segmento(prog,virtual_address);	
	seg = list_get(prog->segment_table,nro_seg);
	int base = seg->base;
	int numero_pagina = (virtual_address - base) / PAGE_SIZE;
	pag = list_get(seg->page_table,numero_pagina);
	int offset = (virtual_address - base) - (numero_pagina * PAGE_SIZE);
	printf("%d representa el seg %d pag %d offset %d \n",virtual_address,nro_seg,numero_pagina,offset);
	
	free_page(numero_pagina);


	// otro flaco:
	//uint8_t seg_index = virual_address >> ;
	//uint8_t page_index = virtual_address & PAGE_MASK >> 
	return 0;
}

int busca_segmento(program *prog, uint32_t va)
{
	int i=0;
	segment *seg;
	int cantidadDeSegmentos = list_size(prog->segment_table);

	if (cantidadDeSegmentos==0) return -1;

	uint32_t virtual_address_base = 0;
	uint32_t virtual_address_final = 0;
	
	while(i < cantidadDeSegmentos)
	{
		seg = list_get(prog->segment_table, i);	
		virtual_address_final += list_size(seg->page_table) * PAGE_SIZE;

		if(va>=virtual_address_base && va<=virtual_address_final)
		{
			printf("Se encontro el seg %d que tiene las direcciones logicas desde %d hasta %d \n",
					i, virtual_address_base, virtual_address_final);
			seg->base = virtual_address_base;		
			return i;
		} 
		else virtual_address_base = virtual_address_final+1;
	}
	return -1;
}
// No se usan ni dst ni src
uint32_t memory_get(void *dst, uint32_t src, size_t numBytes, uint32_t pid)
{

	/* Cosas que hizo el otro flaco ?? */

	uint32_t destination = 0;

	// Busco el programa, despues busco el segmento 0 del programa y de ese segmento busco su página 0 TODO ?? 
	program *prg = list_get(program_list, search_program(pid));
	segment *seg = list_get(prg->segment_table, 0);
	page *pag = list_get(seg->page_table, 0);

	// copio "numBytes" bytes desde la dirección "pag->fr->data" a la direccion &0 de la memoria de MUSE
	memcpy(&destination, pag->fr, numBytes);
	return destination;

	/* 					 */

	// How does memcpy works:
	// Copies "numBytes" bytes from address "from" to address "to"
	// void * memcpy(void *to, const void *from, size_t numBytes);
}

uint32_t memory_cpy(uint32_t dst, void *src, int n, uint32_t pid)
{
	int i= search_program(pid);
	program *prg = list_get(program_list, i);
	segment *seg = list_get(prg->segment_table, 0);
	page *pag = list_get(seg->page_table, 0);
	memcpy(pag->fr, src, n);
	
	return 0;
}


// Apalancándonos en el mismo mecanismo que permite el swapping de páginas,
// la funcionalidad de memoria compartida que proveerá MUSE (a través de sus 
// funciones de muse_map) se realizará sobre un archivo compartido, en vez 
// del archivo de swap. Esta distinción deberá estar plasmada en la tabla de segmentos.
uint32_t memory_map(char *path, size_t length, int flags, uint32_t pid)
{
	/* Idea Original:*/
	int file_descriptor = open(path, O_RDONLY);
   	void* map = mmap(NULL, length, PROT_NONE, flags, file_descriptor, 0);
   	printf(map, length);

   	mapeame(length);

	return *(int*) map; 
}

void mapeame(size_t length){

}

uint32_t memory_sync(uint32_t addr, size_t len, uint32_t pid)
{
	/*synchronize a file with a memory map*/
	msync(addr, len, MS_SYNC);
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
