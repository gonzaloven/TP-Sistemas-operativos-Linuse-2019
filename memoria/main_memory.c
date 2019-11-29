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

frame *main_memory = NULL;

t_list *program_list = NULL;
t_list *segment_list = NULL;
t_log *metricas_logger = NULL;
t_log *debug_logger = NULL;

int PAGE_SIZE = 0;
int TOTAL_FRAME_NUM = 0;
int METADATA_SIZE = 0;

void muse_main_memory_init(int memory_size, int page_size)
{
	int i;
	int curr_page_num;	
	void *mem_ptr = main_memory;	
	
	PAGE_SIZE = page_size;
	main_memory = (frame *) malloc(memory_size); // aka UPCM
	TOTAL_FRAME_NUM = (memory_size / PAGE_SIZE);
	METADATA_SIZE = sizeof(main_memory[0].metadata);		

	program_list = list_create();
	segment_list = list_create();

	debug_logger = log_create(MUSE_LOG_PATH,"DEBUG", true,LOG_LEVEL_TRACE);
	metricas_logger = log_create(MUSE_LOG_PATH, "METRICAS", true, LOG_LEVEL_TRACE);

	printf("Dividiento la memoria en frames...\n");
	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{	
		main_memory[i].metadata.is_free = true ;
		main_memory[i].metadata.free_size = PAGE_SIZE;
		main_memory[i].data = mem_ptr + i * PAGE_SIZE; 
		// supongamos que los frames son de 32 bytes
		// main_memory[0].data va a ser igual a 0 + 0 * 32 = 0
		// main_memory[1].data va a ser igual a 0 + 1 * 32 = 32
		//osea data vendría a ser la base de los frames
	}

	log_trace(metricas_logger, "Tamaño de pagina = tamaño de frame: %d", PAGE_SIZE);
	log_trace(metricas_logger, "Tamaño de metadata: %d", METADATA_SIZE);	
	log_trace(metricas_logger, "Cantidad Total de Memoria:%d", memory_size);
	number_of_free_frames();	
}

void muse_main_memory_stop()
{
	free(main_memory);
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

int number_of_free_frames(){
	int memoria_libre = 0; 
	int frames_libres = 0;
	int i;

	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{	
		frames_libres += (main_memory[i].metadata.is_free == true);
		memoria_libre += main_memory[i].metadata.free_size; 
	}

	log_trace(metricas_logger, "Cantidad de Memoria libre:%d ", memoria_libre);	
	log_trace(metricas_logger, "Cantidad de Frames libres:%d / %d ", frames_libres, TOTAL_FRAME_NUM);

	return frames_libres;
}

page *find_free_frame()
{
	int curr_frame_num;

	page *pag = (page *) malloc(PAGE_SIZE); 

	for(curr_frame_num=0; curr_frame_num < TOTAL_FRAME_NUM; curr_frame_num++)
	{	
		if (main_memory[curr_frame_num].metadata.is_free == true){
			main_memory[curr_frame_num].metadata.is_free = false;
			main_memory[curr_frame_num].metadata.free_size = 0; //TODO: si una pagina apunta a un frame y no usa todo su espacio (paginacion interna), consideramos igual que el frame tiene espacio libre = 0?
			pag->is_present = true;
			pag->page_num = 0;
			pag->fr = &main_memory[curr_frame_num];	
			return pag;
		}
	}
	log_debug(debug_logger, "NOS QUEDAMOS SIN FRAMES LIBRES ");
	return NULL;
}

void metricas_por_socket_conectado(uint32_t pid){
	int nro_prog = search_program(pid);
	program *prog = list_get(program_list, nro_prog);

	int nro_de_seg;

	int cantidad_de_segmentos_asignados = list_size(prog->segment_table);
	int cantidad_de_segmentos_totales = list_size(segment_list);

	log_trace(metricas_logger, "pid: %d tiene asignados %d de %d segmentos en el sistema", 
			nro_prog, cantidad_de_segmentos_asignados, cantidad_de_segmentos_totales);
	number_of_free_frames();
}

uint32_t memory_malloc(int size, uint32_t pid)
{	
	int nro_prog;
	uint32_t seg_id;
	uint32_t page_id;
	uint32_t logical_address = 0;
	page *pag;
	segment *seg;
	program *prog;
	int total_size;
	int total_pages_needed;
	
	total_pages_needed = (size / PAGE_SIZE) + ((size % PAGE_SIZE) != 0); // ceil( size / PAGE_SIZE )
	
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
	
	if(segment_with_free_space(prog, size) != -1) 
		//si hay espacio en su segmento, malloqueo ahí
	{
		seg_id = segment_with_free_space(prog, size);
		seg = list_get(prog->segment_table, seg_id);	//seg = segmento con espacio		
		seg->free_size -= size;
		list_add(prog->segment_table, pag);		
	}
	
	/* TODO: crear caso
	else if (puedo agrandar un segmento)
		 lo agrando 
	*/

	else 	//si no hay espacio en su último segmento, le creo uno
	{

		total_size = total_pages_needed * PAGE_SIZE;
		log_debug(debug_logger, "Se necesitan %d bytes que serian %d paginas ", total_size, total_pages_needed);

		seg = (segment *) malloc(sizeof(segment));
		seg->free_size = total_size - size;
		seg->page_table = list_create();
		seg->is_heap = true;
		
		//vamos a pedir todas las paginas que necesitemos
		for(int i=0 ; i < total_pages_needed ; i++ )
		{
			pag = find_free_frame();
			list_add(seg->page_table, pag);
		}
		number_of_free_frames();
		page_id = list_add(seg->page_table, find_free_frame());
		//la ultima pagina que pedimos será nuestro page_id TODO: no debería ser la 1ra? 
		list_add(prog->segment_table, seg);	
		seg_id = list_add(segment_list, seg);	
	}
	
	metricas_por_socket_conectado(pid);
	logical_address = seg_id * 10 + page_id; //TODO ??	  	
	return logical_address;
}

int segment_with_free_space(program *prog, int size)
{
	int i=0;
	segment *seg;

	int cantidadDeSegmentos = list_size(prog->segment_table);
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
	//uint8_t seg_index = virual_address >> ;
	//uint8_t page_index = virtual_address & PAGE_MASK >> 
	return 0;
}

// No se usan ni dst ni src
uint32_t memory_get(void *dst, uint32_t src, size_t numBytes, uint32_t pid)
{
	uint32_t destination = 0;

	// Busco el programa, despues busco el segmento 0 del programa y de ese segmento busco su página 0 TODO ?? 
	program *prg = list_get(program_list, search_program(pid));
	segment *seg = list_get(prg->segment_table, 0);
	page *pag = list_get(seg->page_table, 0);

	// copio "numBytes" bytes desde la dirección "pag->fr->data" a la direccion &0 de la memoria de MUSE
	memcpy(&destination, pag->fr->data, numBytes);
	return destination;

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
	memcpy(pag->fr->data, src, n);
	
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
