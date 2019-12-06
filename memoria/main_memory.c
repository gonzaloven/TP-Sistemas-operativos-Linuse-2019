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
#include <math.h>

#define METADATA_SIZE sizeof(struct HeapMetadata)

#define SWAP_PATH "swapfile"

void *MAIN_MEMORY = NULL;

t_list *program_list = NULL;
t_list *segment_list = NULL;
t_list *lista_archivos_mmap = NULL;
t_log *metricas_logger = NULL;
t_log *debug_logger = NULL;

int PAGE_SIZE = 0;
int TOTAL_FRAME_NUM = 0;

// inicializadas only for debugging reasons
bool BITMAP[300];	
bool BITMAP_SWAP_FILE[300];
int TOTAL_FRAME_NUM_SWAP;

void muse_main_memory_init(int memory_size, int page_size, int swap_size)
{
	int i;
	// heap_metadata* metadata;
	// metadata = (heap_metadata*) malloc(METADATA_SIZE);

	int curr_page_num;	
	void *mem_ptr = MAIN_MEMORY;
	PAGE_SIZE = page_size;
	lista_archivos_mmap = list_create();

	MAIN_MEMORY = (void *) malloc(memory_size); // aka UPCM

	TOTAL_FRAME_NUM = memory_size / PAGE_SIZE;
	//BITMAP = (bool*) malloc(sizeof(bool)*TOTAL_FRAME_NUM); //1 if free;
	BITMAP[TOTAL_FRAME_NUM];

	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{	
		BITMAP[i] = 1;
	}

	FILE *swap_file;
	TOTAL_FRAME_NUM_SWAP = swap_size / PAGE_SIZE;

	swap_file = fopen(SWAP_PATH, "w");
	fclose(swap_file);

	for(i=0; i<TOTAL_FRAME_NUM_SWAP; i++)
	{
		BITMAP_SWAP_FILE[i] = 1;
	}

	program_list = list_create();
	segment_list = list_create();

	debug_logger = log_create(MUSE_LOG_PATH,"DEBUG", true,LOG_LEVEL_TRACE);
	metricas_logger = log_create(MUSE_LOG_PATH, "METRICAS", true, LOG_LEVEL_TRACE);

	log_trace(metricas_logger, "Tamaño de pagina = tamaño de frame: %d", PAGE_SIZE);
	log_trace(metricas_logger, "Tamaño de metadata: %d", METADATA_SIZE);	
	log_trace(metricas_logger, "Cantidad Total de Memoria:%d", memory_size);
	number_of_free_frames();
}


void muse_main_memory_stop()
{
	free(MAIN_MEMORY);
	remove(SWAP_PATH);

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
	int memoria_total = 0;
	int frames_libres_1 = 0;
	int frames_libres_2 = 0;
	int i;

	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{	
		frames_libres_1 += (BITMAP[i]);
	}

	memoria_libre = frames_libres_1 * PAGE_SIZE;
	memoria_total = TOTAL_FRAME_NUM * PAGE_SIZE;

	log_trace(metricas_logger, "Cantidad de Memoria libre en UPCM: %d / %d", memoria_libre, memoria_total);	
	log_trace(metricas_logger, "Cantidad de Frames libres en UPCM: %d / %d", frames_libres_1, TOTAL_FRAME_NUM);

	/* SWAP SPACE */

	memoria_libre = 0; 

	for(i=0; i < TOTAL_FRAME_NUM_SWAP; i++)
	{	
		frames_libres_2 += (BITMAP_SWAP_FILE[i]);
	}

	memoria_libre = frames_libres_2 * PAGE_SIZE;
	memoria_total = TOTAL_FRAME_NUM_SWAP * PAGE_SIZE;

	log_trace(metricas_logger, "Cantidad de Espacio libre en SWAP: %d / %d", memoria_libre, memoria_total);	
	log_trace(metricas_logger, "Cantidad de Frames libres en SWAP: %d / %d", frames_libres_2, TOTAL_FRAME_NUM_SWAP);

	/* FIN SWAP SPACE */

	return frames_libres_1 + frames_libres_2;
}

int proximo_frame_libre(){
	int i;

	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{	
		if((BITMAP[i])) return i;
	}
	return -1;
}

int frames_needed(int size_total){
	return (size_total/PAGE_SIZE + (size_total%PAGE_SIZE != 0));
}

page* page_with_free_size(){
	//heap_metadata* metadata;
	//heap_metadata* metadata2;

	int curr_frame_num;
	page* pag = (page *) malloc(PAGE_SIZE);

	for(curr_frame_num=0; true; curr_frame_num++)
	{	
		if (curr_frame_num == TOTAL_FRAME_NUM){
			log_debug(debug_logger, "Nos quedamos sin frames libres, aplicando algoritmo de reemplazo");
			curr_frame_num = dame_nro_frame_reemplazado();
		}

		if (BITMAP[curr_frame_num])
		{
			BITMAP[curr_frame_num] = 0;
			pag->is_present = 1;
			pag->is_used = 1;
			pag->is_modify = 0;
			pag->fr = MAIN_MEMORY + (curr_frame_num * PAGE_SIZE);			
			return pag;
		}
	}	
}

/*
	Teniendo al par (U , M)
	1. Se busca (0,0) avanzando el puntero pero sin poner U en 0
	2. Si no se encuentra, se busca (0,1) avanzando el puntero poniendo U en 0
	3. Si no se encuentra, se vuelve al paso 1)
	^
	|__ ESTO SERÍA int nro_paso

 */
int dame_nro_frame_reemplazado(){
	int cantidad_de_segmentos_totales = list_size(segment_list);
	int cantidad_de_paginas_en_segmento;
	int nro_de_segmento, nro_de_pag, nro_frame;
	int nro_paso = 1;
	page *pag;
	segment *seg;
	int U, M;	

	while(true)
	{
		for (nro_de_segmento = 0; nro_de_segmento < cantidad_de_segmentos_totales ; nro_de_segmento++)
		{
			seg = list_get(segment_list,nro_de_segmento);
			cantidad_de_paginas_en_segmento = list_size(seg->page_table);			
			log_debug(debug_logger, "--- SEGMENTO ANALIZADO ALG REEMPLAZO: %d", nro_de_segmento);
			
			for(nro_de_pag = 0; nro_de_pag < cantidad_de_paginas_en_segmento; nro_de_pag++)
			{
				pag = list_get(seg->page_table,nro_de_pag);
				if (!pag->is_present) continue;
				U = pag->is_used;
				M = pag->is_modify;
				log_debug(debug_logger, "Pagina analizada: %d; U = %d - M = %d", nro_de_pag, U, M);
				if (nro_paso == 1){
					if (!U && !M){	
						nro_frame = se_hace_la_vistima(pag, nro_de_pag, nro_de_segmento);
						return nro_frame;											
					}
				}
				if (nro_paso == 2){
					if (!U && M){
						nro_frame = se_hace_la_vistima(pag, nro_de_pag, nro_de_segmento);
						return nro_frame;	
					}
					pag->is_used = 0;
				}			
			}			
		}
		nro_paso++;
		if (nro_paso == 3)
			nro_paso = 1;
	}
}  

//devuelve el nro de frame de la víctima & la manda al archivo swap
int se_hace_la_vistima(page* pag, int nro_de_pag, int nro_de_segmento)
{
	log_debug(debug_logger, "Se eligio como víctima a la pagina %d del segmento %d",
							nro_de_pag, nro_de_segmento);
	int nro_frame_upcm;
	int nro_frame_swap;

	//Nosotros habiamos dicho que: pag->fr = MAIN_MEMORY + (curr_frame_num * PAGE_SIZE)
	nro_frame_upcm = (pag->fr - MAIN_MEMORY) / PAGE_SIZE;
	BITMAP[nro_frame_upcm] = 1; //declaro como libre ese nro de frame

 	nro_frame_swap = mandar_al_archivo_swap_toda_la_pagina_que_esta_en(nro_frame_upcm);

	if (nro_frame_swap == -1)
		log_debug(debug_logger, "Nos quedamos sin frames libres en el archivo swap");
	else{
		log_debug(debug_logger, "La pagina victima se mando satisfactoriamente al archivo swap");

		int dir_disco = nro_frame_swap * PAGE_SIZE;
		memcpy(pag->fr, &dir_disco, sizeof(int));
		pag->is_present = false;

		//pag->fr = (int) nro_frame_swap;
	}
	return nro_frame_upcm;
}

int mandar_al_archivo_swap_toda_la_pagina_que_esta_en(int nro_frame)
{
	FILE *swap_file;
	void* buffer = malloc(PAGE_SIZE);
	int nro_frame_swap = frame_swap_libre();
	if (nro_frame_swap != -1)
	{
		memcpy(buffer, MAIN_MEMORY + (nro_frame * PAGE_SIZE), PAGE_SIZE);

		swap_file = fopen(SWAP_PATH,"r+");

		fseek(swap_file, nro_frame_swap * PAGE_SIZE, SEEK_SET);
		fwrite(buffer, PAGE_SIZE, 1, swap_file);

		free(buffer);
		fclose(swap_file);
	}

	return nro_frame_swap;
}

int frame_swap_libre()
{	
	int i;
	for(i=0; i<TOTAL_FRAME_NUM_SWAP; i++)
	{
		if(BITMAP_SWAP_FILE[i])
		{ 
			BITMAP_SWAP_FILE[i] = 0;
			return i;
		}
	}
	return -1; // no hay frames libres
}

//TODO
void memory_close(uint32_t pid){
	int nro_prog = search_program(pid);

	//deberia sacarlo de la lista
	program *prog = list_get(program_list, nro_prog);

	//liberar su tabla de segmentos
	prog->segment_table;

	//liberar la tabla de paginas de cada segmento

	//liberar todos los frames usados

	//liberar archivos mmapeados que no sean shared
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

void modificar_metadata(int direccionLogica, segment* segmentoBuscado, int nuevoSize, int is_free){
	heap_metadata* metadataBuscada;

	int paginaBuscada = floor((direccionLogica - segmentoBuscado->base) / PAGE_SIZE);
	int offset = (direccionLogica - segmentoBuscado->base) % PAGE_SIZE;

	page* pagina = list_get(segmentoBuscado->page_table, paginaBuscada);

	metadataBuscada = (heap_metadata*) ((pagina->fr) + offset);

	if((offset + METADATA_SIZE) > PAGE_SIZE){
		heap_metadata metadataCopia;

		page* proximaPagina = list_get(segmentoBuscado->page_table, (paginaBuscada + 1));

		if(!proximaPagina->is_present){
			obtener_data_marco_heap(proximaPagina);
			log_debug(debug_logger, "-- LA METADATA ESTABA CORTADA, LA PAGINA SIG NO ESTABA PRESENTE, LA CARGO--");
		}

		if(!pagina->is_present){
			obtener_data_marco_heap(pagina);
			log_debug(debug_logger, "-- PAGINA NO PRESENTE, LA CARGO--");
		}
		void* punteroAlFrameSiguiente = proximaPagina->fr;
		int tamanioMetadataCortada = (PAGE_SIZE) - offset;

		memcpy(&metadataCopia, metadataBuscada, tamanioMetadataCortada);
		memcpy((void*)(&metadataCopia) + tamanioMetadataCortada, punteroAlFrameSiguiente, METADATA_SIZE - tamanioMetadataCortada);

		metadataCopia.is_free = is_free;
		metadataCopia.size = nuevoSize;

		memcpy(metadataBuscada, &metadataCopia, tamanioMetadataCortada);
		memcpy(punteroAlFrameSiguiente, (void*)(&metadataCopia) + tamanioMetadataCortada, METADATA_SIZE - tamanioMetadataCortada);
	}else{
		metadataBuscada->is_free = is_free;
		metadataBuscada->size = nuevoSize;
	}
}

heap_metadata* buscar_metadata_por_direccion(int direccionLogica, segment* segmentoBuscado){
	heap_metadata* metadataBuscada;

	int paginaBuscada = floor((direccionLogica - segmentoBuscado->base) / PAGE_SIZE);
	int offset = (direccionLogica - segmentoBuscado->base) % PAGE_SIZE;

	page* pagina = list_get(segmentoBuscado->page_table, paginaBuscada);

	if(!pagina->is_present){
		obtener_data_marco_heap(pagina);
		log_debug(debug_logger, "-- PAGINA NO PRESENTE, LA CARGO--");
	}

	metadataBuscada = (heap_metadata*) ((pagina->fr) + offset);

	if((offset + METADATA_SIZE) > PAGE_SIZE){
		heap_metadata metadataCopia;

		page* proximaPagina = list_get(segmentoBuscado->page_table, (paginaBuscada + 1));

		if(!proximaPagina->is_present){
			obtener_data_marco_heap(proximaPagina);
			log_debug(debug_logger, "-- LA METADATA ESTABA CORTADA, LA PAGINA SIG NO ESTABA PRESENTE, LA CARGO--");
		}

		void* punteroAlFrameSiguiente = proximaPagina->fr;
		int tamanioMetadataCortada = (PAGE_SIZE) - offset;

		memcpy(&metadataCopia, metadataBuscada, tamanioMetadataCortada);
		memcpy((void*)(&metadataCopia) + tamanioMetadataCortada, punteroAlFrameSiguiente, METADATA_SIZE - tamanioMetadataCortada);

		metadataBuscada->is_free = metadataCopia.is_free;
		metadataBuscada->size = metadataCopia.size;

		return metadataBuscada;
	}

	return metadataBuscada;
}

int ultima_metadata_segmento(int dirLogica, segment* segmentoActual){

	heap_metadata* metadataActual = buscar_metadata_por_direccion(dirLogica, segmentoActual);
	/*if (metadataActual == 0x0){
		log_debug(debug_logger, "La metadataActual esta apuntando a la direccion 0x0 x alguna razon");
		return -1;
	}*/

	int dirLogicaSiguienteMetadata = metadataActual->size + METADATA_SIZE + dirLogica;

	if(dirLogicaSiguienteMetadata == segmentoActual->limit){
		return dirLogica;
	}
	else{
		return ultima_metadata_segmento(dirLogicaSiguienteMetadata, segmentoActual);
	}
}

int proxima_metadata_libre_con_size(int dirLogica, segment* segmentoActual, int size){
	heap_metadata* metadataActual = buscar_metadata_por_direccion(dirLogica, segmentoActual);

	int direccionUltimaMetadata = ultima_metadata_segmento(segmentoActual->base, segmentoActual);

	int dirLogicaSiguienteMetadata = metadataActual->size + METADATA_SIZE + dirLogica;

	if(direccionUltimaMetadata == dirLogicaSiguienteMetadata){
		heap_metadata* ultimaMetadata = buscar_metadata_por_direccion(dirLogicaSiguienteMetadata, segmentoActual);
		if(ultimaMetadata->is_free && ultimaMetadata->size >= size){
			return dirLogicaSiguienteMetadata;
		}else{
			return -1;
		}
	}

	if(metadataActual->is_free && metadataActual->size >= size){
		return dirLogica;
	}else{
		return proxima_metadata_libre_con_size(dirLogicaSiguienteMetadata, segmentoActual, size);
	}

}

void listar_metadatas(int dirLogica, segment* segmento){
	heap_metadata* metadataActual = buscar_metadata_por_direccion(dirLogica, segmento);

	int direccionUltimaMetadata = ultima_metadata_segmento(segmento->base, segmento);
	int dirLogicaSiguienteMetadata = metadataActual->size + METADATA_SIZE + dirLogica;

	if(direccionUltimaMetadata == dirLogicaSiguienteMetadata){
		heap_metadata* ultimaMetadata = buscar_metadata_por_direccion(dirLogicaSiguienteMetadata, segmento);
		log_debug(debug_logger, "Ultima metadata --> Size: %d, is_free: %d, direccionLogica: %d", metadataActual->size, metadataActual->is_free, dirLogica);
	}

	log_debug(debug_logger, "Metadata --> Size: %d, is_free: %d, direccionLogica: %d", metadataActual->size, metadataActual->is_free, dirLogica);

	listar_metadatas(dirLogicaSiguienteMetadata, segmento);
}

int proxima_metadata_libre(int dirLogica, segment* segmentoActual){
	heap_metadata* metadataActual = buscar_metadata_por_direccion(dirLogica, segmentoActual);

	int direccionUltimaMetadata = ultima_metadata_segmento(segmentoActual->base, segmentoActual);

	int dirLogicaSiguienteMetadata = metadataActual->size + METADATA_SIZE + dirLogica;

	if(direccionUltimaMetadata == dirLogicaSiguienteMetadata){
		heap_metadata* ultimaMetadata = buscar_metadata_por_direccion(dirLogicaSiguienteMetadata, segmentoActual);
		if(ultimaMetadata->is_free){
			return dirLogicaSiguienteMetadata;
		}else{
			return -1;
		}
	}

	if(metadataActual->is_free){
		return dirLogica;
	}else{
		return proxima_metadata_libre(dirLogicaSiguienteMetadata, segmentoActual);
	}

}

uint32_t memory_malloc(int size, uint32_t pid)
{	
	if (size <= 0) return 0;
	if (size + METADATA_SIZE > number_of_free_frames() * PAGE_SIZE){
		log_debug(debug_logger, "No se puede maloquear tanto espacio en memoria");
		return 0;		
	} 

	int nro_prog;
	uint32_t nro_seg;
	uint32_t nro_pag;
	uint32_t offset = 0;
	int direccionLogicaMetadataLibre;
	page *pag;
	program *prog;
	heap_metadata *metadata; 
	int total_size = size + METADATA_SIZE;
	int total_pages_needed = (total_size / PAGE_SIZE) + ((total_size % PAGE_SIZE) != 0); // ceil(total_size / PAGE_SIZE)
	int espacio_que_queda_alocar = total_size;
	int espacio_q_quedara_libre = total_pages_needed * PAGE_SIZE - total_size;
	int is_first_page = 1;
	
	log_debug(debug_logger, "Se pidieron %d bytes, + metadata = %d", size, total_size);
	
	//si el programa no está en la lista de programas, se agrega y le creamos una nueva tabla de segmentos
	if((nro_prog = search_program(pid)) == -1)
	{
		prog = (program *) malloc(sizeof(program));		
		prog->pid = pid;
		prog->segment_table = list_create();
		nro_prog = list_add(program_list, prog);
		log_debug(debug_logger, "Se creo el prog n°%d de la lista de programas", nro_prog);
	}

	prog = list_get(program_list, nro_prog);	
	
	//si hay espacio en su segmento, lo malloqueo ahí
	if(segment_with_free_space(prog, total_size) != -1)
	{
		log_debug(debug_logger, "Encontre espacio en el segmento, alloco aca");
		segment *segmentoConEspacio;
		heap_metadata *metadataBuscada;

		nro_seg = segment_with_free_space(prog, total_size);
		segmentoConEspacio = list_get(prog->segment_table, nro_seg);

		direccionLogicaMetadataLibre = proxima_metadata_libre_con_size(segmentoConEspacio->base, segmentoConEspacio, total_size);

		log_debug(debug_logger, "La direccion logica de la metadata donde voy a allocar esta en ---> %d", direccionLogicaMetadataLibre);

		metadataBuscada = buscar_metadata_por_direccion(direccionLogicaMetadataLibre, segmentoConEspacio);

		int paginaDeLaMetadataLibre = floor((direccionLogicaMetadataLibre - segmentoConEspacio->base) / PAGE_SIZE);

		//Alloco aca

		int sizeAnterior = metadataBuscada->size;

		modificar_metadata(direccionLogicaMetadataLibre, segmentoConEspacio, size, 0);

		log_debug(debug_logger, "Nuevo size de la metadata ---> %d", metadataBuscada->size);

		int direccionLogicaMetadataFinal = metadataBuscada->size + METADATA_SIZE + direccionLogicaMetadataLibre;

		offset = (direccionLogicaMetadataFinal - segmentoConEspacio->base) % PAGE_SIZE;

		heap_metadata *metadataFinal = buscar_metadata_por_direccion(direccionLogicaMetadataFinal, segmentoConEspacio);

		modificar_metadata(direccionLogicaMetadataFinal, segmentoConEspacio, (sizeAnterior - total_size), 1);
		log_debug(debug_logger, "La metadata final tiene ---> %d", metadataFinal->size);

	}
	//Si su ultimo segmento es heap y tiene memoria están los frames que necesitamos
	else if (list_size(prog->segment_table)>0 && ultimo_segmento_programa(prog)->is_heap)
	{
		log_debug(debug_logger, "Intento agrandar el que tiene");
		segment *segmentoAAgrandar;
		segmentoAAgrandar = ultimo_segmento_programa(prog);

		//Busco la ultima metadata que deberia estar libre y la agrando
		heap_metadata* ultimaMetadata;

		int direccionLogicaUltimaMetadata = ultima_metadata_segmento(segmentoAAgrandar->base, segmentoAAgrandar);

		ultimaMetadata = buscar_metadata_por_direccion(direccionLogicaUltimaMetadata, segmentoAAgrandar);

		int sizeAnterior = ultimaMetadata->size;
		float cantidadDePaginasAAgrandar = ceil(((float)(total_size - sizeAnterior))/PAGE_SIZE);

		ultimaMetadata->is_free = 1;
		ultimaMetadata->size = cantidadDePaginasAAgrandar * PAGE_SIZE + sizeAnterior;

		for(int i=0 ; i < cantidadDePaginasAAgrandar ; i++ )
		{
			//log_debug(debug_logger, "Size solicitado para PAGE_WITH_FREE_SIZE: %d", PAGE_SIZE);
			pag = page_with_free_size();
			pag->is_used = 1;
			list_add(segmentoAAgrandar->page_table, pag);
			segmentoAAgrandar->limit += PAGE_SIZE;
		}

		log_debug(debug_logger, "Se pudo agrandar el segmento %d, intento alocar otra vez", segmentoAAgrandar->limit);

		return memory_malloc(size, pid);
	}
	//en ultimo caso: si no hay espacio ni se puede agrandar ningún segmento, le creo uno
	else 	
	{
		log_debug(debug_logger, "No pude agrandar ningun segmento asi que le creo uno");
		int cantidadDePaginasAAgregar = total_pages_needed;
		segment *segmentoNuevo;
		segmentoNuevo = (segment *) malloc(sizeof(segment));
		segmentoNuevo->is_heap = true;
		segmentoNuevo->page_table = list_create();
		segmentoNuevo->base = 0;
		segmentoNuevo->limit = 0;
		list_add(prog->segment_table, segmentoNuevo);			
		nro_seg = list_add(segment_list, segmentoNuevo);
		log_debug(debug_logger, "Se creo el segmento n° %d del sistema", nro_seg);

		for(int i=0; i < cantidadDePaginasAAgregar; i++ )
		{
			//log_debug(debug_logger, "Size solicitado para PAGE_WITH_FREE_SIZE: %d", PAGE_SIZE);
			pag = page_with_free_size();
			pag->is_used = 1;
			list_add(segmentoNuevo->page_table, pag);
			segmentoNuevo->limit += PAGE_SIZE;
		}

		page* primeraPagina = list_get(segmentoNuevo->page_table, 0);

		log_debug(debug_logger, "Limite del segmento nuevo ----> %d", segmentoNuevo->limit);

		heap_metadata* primerMetadata = (primeraPagina->fr);

		primerMetadata->is_free = 1;
		primerMetadata->size = (cantidadDePaginasAAgregar * PAGE_SIZE) - METADATA_SIZE;

		return memory_malloc(size, pid);
	}	
	
	int direccionLogicaFinal = direccionLogicaMetadataLibre + METADATA_SIZE;
	log_debug(debug_logger, "Direccion logica final del MALLOC ----> %d", direccionLogicaFinal); 

	metricas_por_socket_conectado(pid);

	return direccionLogicaFinal;
}

int segment_with_free_space(program *prog, int size)
{
	int i=0;
	segment *segmentoActual;
	int cantidadDeSegmentos = list_size(prog->segment_table);

	if (cantidadDeSegmentos == 0) return -1;

	while(i < cantidadDeSegmentos)
	{		
		segmentoActual = list_get(prog->segment_table, i);

		if (segmentoActual->is_heap)
		{
			int direccionLogicaEncontrada = proxima_metadata_libre_con_size(segmentoActual->base, segmentoActual, size);

			if(direccionLogicaEncontrada != -1){
				heap_metadata* metadataEncontrada = buscar_metadata_por_direccion(direccionLogicaEncontrada, segmentoActual);

				log_debug(debug_logger, "Encontramos que el segmento %d tiene una metadata de %d de size", i, metadataEncontrada->size);
				return i;
			}
		}
		i++;
	}
	log_debug(debug_logger, "No habia ningun segmento con %d de espacio libre", size);
	return -1;
}

bool tiene_siguiente(int direccionLogicaMetadata, segment* segmento){
	if(direccionLogicaMetadata == segmento->limit){
		return false;
	}else{
		return true;
	}
}

void compactar_en_segmento(int direccionLogicaMetadataLibreInicial, int direccionLogicaSiguiente, segment* segmento){

	heap_metadata* metadataLibreInicial = buscar_metadata_por_direccion(direccionLogicaMetadataLibreInicial, segmento);
	log_debug(debug_logger, "Metadata inicial de %d de size e is_free = %d", metadataLibreInicial->size, metadataLibreInicial->is_free);

	int direccionUltimaMetadata = ultima_metadata_segmento(segmento->base, segmento);

	//Si no tiene siguiente (la siguiente) entonces quiere decir que es la ultima
	if(direccionLogicaSiguiente == direccionUltimaMetadata)
	{
		heap_metadata* metadataSiguiente = buscar_metadata_por_direccion(direccionLogicaSiguiente, segmento);
		log_debug(debug_logger, "Metadata siguiente de %d de size e is_free = %d", metadataSiguiente->size, metadataSiguiente->is_free);
		log_debug(debug_logger, "Esta es la ultima");

		//Si esta libre la sumamos a la inicial y si no termino el algoritmo
		if(metadataSiguiente->is_free){

			int is_free = metadataLibreInicial->is_free;
			modificar_metadata(direccionLogicaMetadataLibreInicial, segmento, metadataLibreInicial->size + metadataSiguiente->size, is_free);

			log_debug(debug_logger, "Metadata libre ahora tiene %d de size e is_free = %d", metadataLibreInicial->size, metadataLibreInicial->is_free);
			int viejoSize = metadataSiguiente->size;
			modificar_metadata(direccionLogicaSiguiente, segmento, viejoSize, 1);

		}
	}
	else
	{
		//Si tiene siguiente entonces podemos seguir mas todavia
		heap_metadata* metadataSiguiente = buscar_metadata_por_direccion(direccionLogicaSiguiente, segmento);
		log_debug(debug_logger, "Metadata siguiente de %d de size e is_free = %d", metadataSiguiente->size, metadataSiguiente->is_free);

		//Si esta libre la sumamos a la inicial y repetimos el proceso
		if(metadataSiguiente->is_free){

			log_debug(debug_logger, "Esta libre asi que la compacto con la inicial");

			int is_free = metadataLibreInicial->is_free;
			modificar_metadata(direccionLogicaMetadataLibreInicial, segmento, metadataLibreInicial->size + metadataSiguiente->size, is_free);

			log_debug(debug_logger, "Metadata libre ahora tiene %d de size e is_free = %d", metadataLibreInicial->size, metadataLibreInicial->is_free);
			int viejoSize = metadataSiguiente->size;
			modificar_metadata(direccionLogicaSiguiente, segmento, viejoSize, 1);

			int direccionSiguienteSiguiente = direccionLogicaSiguiente + metadataSiguiente->size + METADATA_SIZE;

			compactar_en_segmento(direccionLogicaMetadataLibreInicial, direccionSiguienteSiguiente, segmento);

		//Si no entonces hay que buscar una nueva libre inicial en donde pararse para repetir el proceso
		}else{
			log_debug(debug_logger, "No esta libre asi que me fijo la proxima libre");
			direccionLogicaMetadataLibreInicial = proxima_metadata_libre(direccionLogicaSiguiente, segmento);

			if(direccionLogicaMetadataLibreInicial != -1){
				//Si tiene siguiente la proxima libre entonces podemos seguir, si no no hay que compactar ya
				if(direccionUltimaMetadata != direccionLogicaMetadataLibreInicial){
					metadataLibreInicial = buscar_metadata_por_direccion(direccionLogicaMetadataLibreInicial, segmento);
					log_debug(debug_logger, "Metadata nueva libre inicial de %d de size e is_free = %d", metadataLibreInicial->size, metadataLibreInicial->is_free);

					int direccionLogicaSiguiente = direccionLogicaMetadataLibreInicial + metadataLibreInicial->size + METADATA_SIZE;

					compactar_en_segmento(direccionLogicaMetadataLibreInicial, direccionLogicaSiguiente, segmento);
				}
			}
		}
	}
}

void compactar_espacios_libres(program *prog){
	int cantidad_de_segmentos = list_size(prog->segment_table);
	int posicionActual = 0;
	int paginaActualNumero = 0;
	int cantidadDePaginasAMoverme;
	int i;
	heap_metadata* metadataInicialLibre;

	for(i=0; i < cantidad_de_segmentos; i++){
		segment* segmentoActual = list_get(prog->segment_table, i);

		int direccionLogicaMetadataLibre = proxima_metadata_libre(segmentoActual->base, segmentoActual);

		if(direccionLogicaMetadataLibre != -1){
			heap_metadata* metadataLibreInicial = buscar_metadata_por_direccion(direccionLogicaMetadataLibre, segmentoActual);

			if(tiene_siguiente(direccionLogicaMetadataLibre, segmentoActual))
			{
				int dirLogicaSiguienteMetadata = metadataLibreInicial->size + METADATA_SIZE + direccionLogicaMetadataLibre;
				heap_metadata* metadataSiguiente = buscar_metadata_por_direccion(dirLogicaSiguienteMetadata, segmentoActual);

				compactar_en_segmento(direccionLogicaMetadataLibre, dirLogicaSiguienteMetadata, segmentoActual);
			}
		}
	}
}

uint8_t memory_free(uint32_t virtual_address, uint32_t pid)
{
	int nro_prog = search_program(pid);
	program *prog;
	segment *seg;

	log_debug(debug_logger, "direccion virtual pedida: %d", virtual_address);

	prog = list_get(program_list, nro_prog);
	int nro_seg = busca_segmento(prog,virtual_address);	
	seg = list_get(prog->segment_table,nro_seg);

//	int base = seg->base;
//	int numero_pagina = (virtual_address - base) / PAGE_SIZE;
//	pag = list_get(seg->page_table,numero_pagina);
//	int offset = (virtual_address - base) - (numero_pagina * PAGE_SIZE);
//	printf("%d representa el seg %d pag %d offset %d \n",virtual_address,nro_seg,numero_pagina,offset);

	int direccionLogicaMetadata = virtual_address - METADATA_SIZE;

	heap_metadata* metadata = buscar_metadata_por_direccion(direccionLogicaMetadata, seg);
	int viejoSize = metadata->size;
	modificar_metadata(direccionLogicaMetadata, seg, viejoSize, 1);

	compactar_espacios_libres(prog);

	//Para debug
	//listar_metadatas(seg->base, seg);

	return 0;
}

segment* ultimo_segmento_programa(program *prog){
	segment* seg;
	int ultimo_elemento = list_size(prog->segment_table)-1;
	seg = list_get(prog->segment_table, ultimo_elemento );
	return seg;
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

void* buscar_frame_libre(size_t bytesNecesarios){
	return NULL;
}

void* cargarPaginaAMemoria(segment* segmentoBuscado, size_t bytesNecesarios, int paginaBuscada){
	page* paginaACargar = list_get(segmentoBuscado->page_table, paginaBuscada);

	paginaACargar->is_present = 1;
	paginaACargar->fr = buscar_frame_libre(bytesNecesarios);

	return paginaACargar->fr;
}

uint32_t buscar_direccion_fisica(uint32_t direccionSolicitada, size_t bytesNecesarios, uint32_t pid){
	int i= search_program(pid);
	program *prg = list_get(program_list, i);
	segment* segmentoBuscado;

	uint32_t direccionFisicaBuscada;

	for(int i=0; i < prg->segment_table->elements_count; i++){
		segment *seg = list_get(prg->segment_table, i);

		//habria que ver si es una direccion invalida y enviar el error
		if(direccionSolicitada >= seg->base && direccionSolicitada < seg->limit){
			segmentoBuscado = seg;
			break;
		}
	}

	//Si no esta la pagina deberia reservar un frame, ir al disco y leer el contenido
	//Meterlo en el frame recien reservado y asignarlo a la pagina TODO

	int paginaBuscada = floor((direccionSolicitada - segmentoBuscado->base) / PAGE_SIZE);
	int offset = (direccionSolicitada - segmentoBuscado->base) % PAGE_SIZE;

	page* paginaAEscribir = list_get(segmentoBuscado->page_table, paginaBuscada);

	//Page fault
	if(!paginaAEscribir->is_present){
		direccionFisicaBuscada = (int)cargarPaginaAMemoria(segmentoBuscado, bytesNecesarios, paginaBuscada) + offset;
	}else{
		direccionFisicaBuscada = (uint32_t) ((paginaAEscribir->fr) + offset);
	}

	return direccionFisicaBuscada;
}
// Copia n bytes de MUSE a LIBMUSE
uint32_t memory_get(void *dst, uint32_t src, size_t numBytes, uint32_t pid)
{
	int direccionFisicaBuscada = buscar_direccion_fisica(src, numBytes, pid);
	memcpy(dst, (void*) direccionFisicaBuscada, numBytes);

	return src;
}

void* obtener_data_marco_heap(page* pagina){
    if(!pagina->is_present && number_of_free_frames() == 0){
    	/* page* page_mm = dame_nro_frame_reemplazado();

        void* buffer_page_mm = malloc(PAGE_SIZE);
        void* buffer_page_swap = malloc(PAGE_SIZE);

        memcpy(buffer_page_mm,MAIN_MEMORY + ((int)page_mm->fr * PAGE_SIZE),PAGE_SIZE);

        FILE* archivo_swap = fopen(SWAP_PATH,"r+");

        fseek(archivo_swap,(int) pagina->fr * PAGE_SIZE,SEEK_SET);
        fread(buffer_page_swap,PAGE_SIZE,1,archivo_swap);
        fseek(archivo_swap,(int) pagina->fr * PAGE_SIZE,SEEK_SET);
        fwrite(buffer_page_mm,PAGE_SIZE,1,archivo_swap);

        memcpy(MAIN_MEMORY + ((int)page_mm->fr * PAGE_SIZE),buffer_page_swap,PAGE_SIZE);

        page_mm->is_present = 0;
        page_mm->fr = pagina->fr;*/

        /*page* sacarFrame = page_with_free_size();
        pagina->fr = sacarFrame->fr;*/

    	int numFrame = dame_nro_frame_reemplazado();
    	liberar_frame_swap(pagina->fr);
    	log_debug(debug_logger, "Paso pagina de SWAP a MEMORIA PRINCIPAL -> libero frame de swap");
    	pagina->fr = (void*) (MAIN_MEMORY + numFrame * PAGE_SIZE);
        pagina->is_present = 1;
        pagina->is_used = 1;
    }
    else if(!pagina->is_present && number_of_free_frames() > 0){
        void* buffer_page_swap = malloc(PAGE_SIZE);

        FILE* archivo_swap = fopen(SWAP_PATH,"r+");

        fseek(archivo_swap,*(int*)pagina->fr,SEEK_SET);
        fread(buffer_page_swap,PAGE_SIZE,1,archivo_swap);

        page* sacarFrame = page_with_free_size();
        liberar_frame_swap(pagina->fr);
        log_debug(debug_logger, "Paso pagina de SWAP a MEMORIA PRINCIPAL -> libero frame de swap");
        pagina->fr = sacarFrame->fr;
        pagina->is_present = 1;
        pagina->is_used = 1;

        memcpy(pagina->fr,buffer_page_swap,PAGE_SIZE);

        free(buffer_page_swap);
        fclose(archivo_swap);
    }

    return pagina->fr;
}

void* obtener_data_marco_mmap(segment* segmento,page* pagina,int nro_pagina){
    if(!pagina->is_present && number_of_free_frames() == 0){
        /*page* page_mm = ejecutar_algoritmo_clock_modificado(); // ESTA FUNCTION FALTA ---------------

        void* buffer_page_mm = malloc(PAGE_SIZE);
        void* buffer_page_mmap = malloc(PAGE_SIZE);
        page* sacarFrame = page_with_free_size();
        int frame_libre = (int)sacarFrame->fr;

        memcpy(buffer_page_mm,MAIN_MEMORY + ((int)page_mm->fr * PAGE_SIZE),PAGE_SIZE);

        FILE* archivo_swap = fopen(SWAP_PATH,"r+");

        fseek(archivo_swap,frame_libre * PAGE_SIZE,SEEK_SET);
        fwrite(buffer_page_mm,PAGE_SIZE,1,archivo_swap);

        page_mm->fr = sacarFrame->fr;
        page_mm->is_present = 0;*/

    	void* buffer_page_mmap = malloc(PAGE_SIZE);

    	int numFrame = dame_nro_frame_reemplazado();
    	log_debug(debug_logger, "Paso pagina de ARCHIVO MAPPED a MEMORIA PRINCIPAL");
    	pagina->fr = (void*) numFrame;
        pagina->is_present = 1;
        pagina->is_used = 1;

        if((nro_pagina * PAGE_SIZE) <= segmento->limit){
            //fseek(segmento->archivo_mapeado,nro_pagina * PAGE_SIZE,SEEK_SET);

            int bytes_a_leer = (int)fmin(PAGE_SIZE,((nro_pagina * PAGE_SIZE) + PAGE_SIZE) - segmento->limit);

            //fread(buffer_page_mmap,bytes_a_leer,1,segmento->archivo_mapeado);
            //memcpy(pagina->fr,buffer_page_mmap,bytes_a_leer);

            memcpy(pagina->fr, segmento->archivo_mapeado->archivo + nro_pagina * PAGE_SIZE, bytes_a_leer);
            if(PAGE_SIZE > bytes_a_leer)
                memset(pagina->fr,'\0',PAGE_SIZE - bytes_a_leer);
        }
        else{
            // el primer byte a leer supera el tamano del archivo
            memset(pagina->fr,'\0',PAGE_SIZE);
        }

        free(buffer_page_mmap);
/*        agregar_frame_clock(pagina);

        free(buffer_page_mm);
        free(buffer_page_mmap);
        fclose(archivo_swap);*/
    }
    else if(!pagina->is_present && number_of_free_frames() > 0){
        void* buffer_page_mmap = malloc(PAGE_SIZE);

        page* sacarFrame = page_with_free_size();
        log_debug(debug_logger, "Paso pagina de MAPPED a MEMORIA PRINCIPAL");
        pagina->fr = sacarFrame->fr;
        pagina->is_present = 1;
        pagina->is_used = 1;

        //fseek(segmento->archivo_mapeado,nro_pagina * PAGE_SIZE,SEEK_SET);
        //fread(buffer_page_mmap,PAGE_SIZE,1,segmento->archivo_mapeado);

        //memcpy(pagina->fr,buffer_page_mmap,PAGE_SIZE);
        memcpy(pagina->fr, segmento->archivo_mapeado->archivo + nro_pagina * PAGE_SIZE, PAGE_SIZE);

        free(buffer_page_mmap);
    }

    return pagina->fr;
}

//Copia n bytes de LIBMUSE a MUSE
uint32_t memory_cpy(uint32_t dst, void *src, int n, uint32_t pid)
{
	page* paginaObtenida;
	int numProg = search_program(pid);
	program* program = list_get(program_list, numProg);
	log_debug(debug_logger, "El programa del pedido es %d", numProg);

	int numSeg = busca_segmento(program, dst);
	segment* segment = list_get(program->segment_table, numSeg);

	if(segment == NULL){
		// no se encontro el segmento, tiene que tirar seg fault
		log_debug(debug_logger, "Se busco el segmento %d, pero no se encontro", numSeg);
		return -1;
	}else{
		log_debug(debug_logger, "El segmento a escribir es %d", numSeg);
	}

	int numPage = floor((dst - segment->base) / PAGE_SIZE);

	int offset = (dst - segment->base) % PAGE_SIZE;

	log_debug(debug_logger, "La pagina a escribir es %d - Con offset %d", numPage, offset);

	int posicion_recorrida = offset - METADATA_SIZE;

	//calculo la cantidad de paginas necesarias que tiene que escribir
	int cantidad_paginas_necesarias = (int)ceil((double)(offset + n) / PAGE_SIZE);

	//me fijo si la metadata no se encuentra en esta pagina y si el seg es heap
	if(segment->is_heap && (posicion_recorrida < 0)){
		//tengo que obtener la pagina anterior para manejar la metadata
		int cant_pag_aux = (int)ceil((double)abs(posicion_recorrida)/PAGE_SIZE);
		cantidad_paginas_necesarias += cant_pag_aux;
		numPage -= cant_pag_aux;
		posicion_recorrida = (cant_pag_aux * PAGE_SIZE) + offset - METADATA_SIZE;
	}

	//controlo que la cantidad de paginas necesarias no supere a las reales, se podria psar por el ceil pero no deberia.
	cantidad_paginas_necesarias = (int)fmin((float)cantidad_paginas_necesarias, list_size(segment->page_table));
	log_debug(debug_logger, "Cantidad de paginas necesarias para leer la data %d", cantidad_paginas_necesarias);

	void* buffer = malloc(cantidad_paginas_necesarias * PAGE_SIZE);
	void* datos;
	heap_metadata metadata;

	for(int i = 0; i < cantidad_paginas_necesarias; i++){
		paginaObtenida = list_get(segment->page_table, numPage + i);
		if(segment->is_heap){
			datos = obtener_data_marco_heap(paginaObtenida);
		}else{
			datos = obtener_data_marco_mmap(segment, paginaObtenida, numPage + i);
		}

		memcpy(buffer + (PAGE_SIZE * i), datos, PAGE_SIZE);
	}

	if(segment->is_heap){
        // obtengo la metadata
		memcpy(&metadata.size,buffer + posicion_recorrida,sizeof(uint32_t));
		posicion_recorrida += sizeof(uint32_t);
        memcpy(&metadata.is_free,buffer + posicion_recorrida,sizeof(bool));
        posicion_recorrida += sizeof(bool);

        //borrar, es para debugg
        log_debug(debug_logger, "El bit de free de metadata es: %d", metadata.is_free);
        log_debug(debug_logger, "El size de la metadata es: %d", metadata.size);

        if(!metadata.is_free && (metadata.size >= n)){
            memcpy(buffer + posicion_recorrida,src,n);

            //borrar, estos dos debug es para ver si le llego bien
            char* texto = malloc(n);
            memcpy(texto, src,n);
            log_debug(debug_logger, "El valor del source es: %s", texto);
            log_debug(debug_logger, "La cantidad de bytes a copiar es: %d", n);

            // vuelvo a cargar los datos al upcm
            for(int x=0; x<cantidad_paginas_necesarias;x++){
            	paginaObtenida = list_get(segment->page_table,x + numPage);
                datos = obtener_data_marco_heap(paginaObtenida);
                memcpy(datos,buffer + PAGE_SIZE * x, PAGE_SIZE);
            }
        }
        else{
        	log_debug(debug_logger, "Posicion invalida, no se pudo realizar la copia");
            return -1;
        }
	}else{
		if((offset + (cantidad_paginas_necesarias * PAGE_SIZE)) >= n){
			memcpy(buffer + offset,src,n);

		                // vuelvo a cargar los datos al upcm
			for(int x=0; x<cantidad_paginas_necesarias;x++){
				paginaObtenida = list_get(segment->page_table,x + numPage);
				datos = obtener_data_marco_mmap(segment,paginaObtenida,x + numPage);
				memcpy(datos,buffer + PAGE_SIZE * x,PAGE_SIZE);
			}
		}else{
		 	// no puedo almacenar los datos pq ingreso a una posicion invalida
			log_debug(debug_logger, "Posicion invalida, no se pudo realizar la copia");
			return -1;
		 }
	}

	// esto es para debug del test 6
    char* texto = malloc(5);
    memcpy(texto, datos + 5,5);
	log_debug(debug_logger, "(SOLO FUNCIONA PARA TEST 6 EL DEBUG) Copie: %s", texto);
	log_debug(debug_logger, "Fin memory_cpy");
	
	return dst;
}

archivoMMAP* buscar_archivo_mmap(char* path){

	bool igualArchivo(archivoMMAP* archivo) {
		return !strcmp(archivo->pathArchivo, path);
	}
	return list_find(lista_archivos_mmap,(void*) igualArchivo);
}


void agregar_archivo_mapeado(FILE* archivoAAgregar, int pid, t_list* tabla_paginas){
	archivoMMAP* nuevoArchivoMMAP = malloc(sizeof(archivoMMAP));

	nuevoArchivoMMAP->archivo = archivoAAgregar;
	nuevoArchivoMMAP->programas = list_create();
	list_add(nuevoArchivoMMAP->programas, pid);
	nuevoArchivoMMAP->tabla_paginas = tabla_paginas;

	list_add(lista_archivos_mmap, nuevoArchivoMMAP);
}

int crear_nuevo_segmento_mmap(size_t length, uint32_t pid){
	int nro_prog = search_program(pid);
	program* prog = list_get(program_list, nro_prog);

	segment* seg;
	page* pag;
	seg = (segment *) malloc(sizeof(segment));
	seg->is_heap = false;
	seg->page_table = list_create();
	seg->limit = 0;
	int nro_pag;
	int total_pages_needed;

	int numeroDeSegmentos = list_size(prog->segment_table);

	if(numeroDeSegmentos > 0){
		segment* ultimoSegmento = list_get(prog->segment_table, numeroDeSegmentos - 1);
		seg->base = ultimoSegmento->limit + 1;
	}else{
		seg->base = 0;
	}

	list_add(prog->segment_table, seg);
	list_add(segment_list, seg);

	total_pages_needed = (length / PAGE_SIZE) + ((length % PAGE_SIZE) != 0); // ceil(length / PAGE_SIZE);

	//vamos a pedir todo el resto de paginas que necesitemos
	for(int i=0 ; i < total_pages_needed ; i++ )
	{
		pag = (page *) malloc(sizeof(page));
		pag->is_used = true;
		pag->is_modify = false;
		pag->is_present = false;
		nro_pag = list_add(seg->page_table, pag);
		seg->limit += PAGE_SIZE;
	}

	log_debug(debug_logger, "Limite del segmento nuevo ----> %d", seg->limit);

	return seg->base;
}

// Apalancándonos en el mismo mecanismo que permite el swapping de páginas,
// la funcionalidad de memoria compartida que proveerá MUSE (a través de sus 
// funciones de muse_map) se realizará sobre un archivo compartido, en vez 
// del archivo de swap. Esta distinción deberá estar plasmada en la tabla de segmentos.
uint32_t memory_map(char *path, size_t length, int flag, uint32_t pid)
{
	int nro_prog;
	program* prog;
	segment* segmentoNuevo;
	int direccionDelSegmento;
	archivoMMAP* archivoMappeado;

	if((nro_prog = search_program(pid)) == -1)
	{
		prog = (program *) malloc(sizeof(program));
		prog->pid = pid;
		prog->segment_table = list_create();
		nro_prog = list_add(program_list, prog);
		log_debug(debug_logger, "Se creo el prog n°%d de la lista de programas ", nro_prog);
	}

	log_debug(debug_logger, "path recibido: %s", path);

	archivoMappeado = buscar_archivo_mmap(path);

	if(archivoMappeado == NULL){
		archivoMMAP* archivoMappeadoNuevo = (archivoMMAP*)malloc(sizeof(archivoMMAP));
		int fileDescriptor = open(path, O_RDWR, 0);

		void* mmapArchivo = mmap(NULL, length, PROT_READ|PROT_WRITE, flag, fileDescriptor,0);

		int sizePath = strlen(path) + 1;
		archivoMappeadoNuevo->pathArchivo = malloc(sizePath);
		memcpy(archivoMappeadoNuevo->pathArchivo, path, sizePath);

		archivoMappeadoNuevo->archivo = mmapArchivo;
		archivoMappeadoNuevo->programas = list_create();
		archivoMappeadoNuevo->tabla_paginas = list_create();
		archivoMappeado = archivoMappeadoNuevo;

		list_add(lista_archivos_mmap, archivoMappeado);
	}

	direccionDelSegmento = crear_nuevo_segmento_mmap(length, pid);
	log_debug(debug_logger, "direccion del segmento ----> %d", direccionDelSegmento);

	int nroSegmentoNuevo = busca_segmento(prog, direccionDelSegmento);

	segmentoNuevo = list_get(prog->segment_table, nroSegmentoNuevo);
	segmentoNuevo->archivo_mapeado = archivoMappeado;
	archivoMappeado->tabla_paginas = segmentoNuevo->page_table;
	segmentoNuevo->tam_archivo_mmap = length;

	if(flag == MAP_SHARED){
		log_debug(debug_logger, "La flag de MAP es: MAP_SHARED");
		segmentoNuevo->tipo_map = 1;
		list_add(archivoMappeado->programas, pid);
	}else if(flag == MAP_PRIVATE){
		log_debug(debug_logger, "La flag de MAP es: MAP_PRIVATE");
		segmentoNuevo->tipo_map = 0;
	}else{
		log_debug(debug_logger, "No se reconocio la flag especificada");
		return -1;
	}

	log_debug(debug_logger, "Contenido del segmento mapeado: Base: %d, is_heap: %d, Limit: %d, TamanioArchivo: %d, TipoDeMap: %d",
			segmentoNuevo->base,
			segmentoNuevo->is_heap,
			segmentoNuevo->limit,
			segmentoNuevo->tam_archivo_mmap,
			segmentoNuevo->tipo_map);

	return 0;
}

/* mapea el archivo swap integramente */
char* get_file_content_from_swap(char* swapfile){
	char* fileMapeado;
	int fileDescriptor;
	struct stat estadoDelFile; //declaro una estructura que guarda el estado de un archivo

	fileDescriptor = open(swapfile, O_RDWR);
		/*Chequeo de apertura del file exitosa*/
			if (fileDescriptor==-1){
				log_error(debug_logger,"Fallo la apertura del file de datos");
				exit(-1);
			}
	if(fstat(fileDescriptor,&estadoDelFile)==-1){//guardo el estado del archivo de datos en la estructura
			log_error(debug_logger,"Fallo el fstat");
			exit(-1);
		}
	fileMapeado = mmap(0,estadoDelFile.st_size,(PROT_WRITE|PROT_READ|PROT_EXEC),MAP_SHARED,fileDescriptor,0);
	/*Chequeo de mmap exitoso*/
		if (fileMapeado==MAP_FAILED){
			log_error(debug_logger,"Fallo el mmap, no se pudo asignar la direccion de memoria para el archivo solicitado");
			exit(-1);
		}
	close(fileDescriptor); //Cierro el archivo

	return fileMapeado;
}


	/**
	* Descarga una cantidad `len` de bytes y lo escribe en el archivo en el FileSystem.
	* @param direccion Dirección a memoria mappeada.
	* @param len Cantidad de bytes a escribir.
	* @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
	* @note Si `len` es menor que el tamaño de la página en la que se encuentre, se deberá escribir la página completa.
	* @example	
		imagine we got uint32_t map = muse_map("hola.txt", filesize, MAP_PRIVATE);		
		so let's imagine we do the following:
			muse_sync(map, 200)
		so we're basicly just writing 200 bytes of "nothing" to map	
	*/

uint32_t memory_sync(uint32_t direccion, size_t len, uint32_t pid){return 0;}

/* FIJATE QUE OBTENER_DATOS_MMAP ES LA QUE YO ADAPTE QUE ES OBTENER_DATA_MMAP O ALGO ASI, NO HACE FALTA COPIARLA DEVUELTA
 * LE HABIA SACADO LO DEL CLOCK PORQUE VOS NO LO USABAS EN EL ALGORITMO, HABRIA QUE VER PARA QUE LO USAS
uint32_t memory_sync(uint32_t direccion, size_t len, uint32_t pid)
{
	program* prog;
	segment* seg, segmento_obtenido;

	//si el programa no está en la lista de programas, se agrega y le creamos una nueva tabla de segmentos
	if((nro_prog = search_program(pid)) == -1)
	{
		log_debug(debug_logger, "Que hacés pidiendo un muse_sync sin antes hacer un mmap?");
	}

	//busco que segmento tenga asignado esa direccion
	for(int i=0; i<list_size(prog->segment_table); i++)
	{
		segmento_obtenido = list_get(prog->segment_table,i);
		if((segmento_obtenido->base <= direccion) && (segmento_obtenido->limite >= direccion))
			seg = segmento_obtenido;			
	}

	int cantidad_paginas_necesarias = ceil(len / PAGE_SIZE);
	printf("cantidad_paginas_necesarias %d\n",cantidad_paginas_necesarias);

	//segmentation fault
	if((seg == NULL) || (cantidad_paginas_necesarias > list_size(seg->tabla_paginas)) return 2;

	//error (returen -1)
	if(!(seg->is_heap) || (direccion % PAGE_SIZE != 0)) return 3;


	int nro_pagina_obtenida = (direccion - seg->base) / PAGE_SIZE;
	printf("nro_pagina_obtenida: %d\n",nro_pagina_obtenida);

	page* pagina_obtenida;
	void* direccion_datos;
	int posicion_recorrida;
	void* buffer;

	buffer = malloc(cantidad_paginas_necesarias*PAGE_SIZE);

	for(int i=0; i<cantidad_paginas_necesarias; i++)
	{
		pagina_obtenida = list_get(seg->tabla_paginas,i + nro_pagina_obtenida);
		direccion_datos = dame_los_datos_de_mmap(seg, pagina_obtenida, i + nro_pagina_obtenida);
		memcpy(&buffer[PAGE_SIZE*i], direccion_datos, PAGE_SIZE);
	}

	//el primer byte a escribir no debería superar el tamaño del archivo
	if((nro_pagina_obtenida * PAGE_SIZE) <= seg->tam_archivo_mmap)
	{
		fseek(seg->archivo_mapeado, nro_pagina_obtenida * PAGE_SIZE, SEEK_SET);
		int nro_bytes = (int) fmin(len, seg->tam_archivo_mmap);
		printf("Bytes a escribir: %d\n",nro_bytes);
		fwrite(buffer, nro_bytes, 1, seg->archivo_mapeado);
		free(buffer);
		return 1;
	}
	free(buffer);
	return 3;
}

void* dame_los_datos_de_mmap(segment* seg, page* pag, int nro_pagina)
{	//
	if(!pag->bit_presencia && !espacio_en_MAIN_MEMORY()){
    	page* pag_memoria = ejecutar_algoritmo_clock_modificado();
		archivo_swap = fopen(SWAP_PATH,"r+");

    	void* buffer_pag_upcm = malloc(TAM_PAGINA);
    	void* buffer_pag_mmap = malloc(TAM_PAGINA);
    	int frame_swap_obtenido;


		for(i=0; i < TOTAL_FRAME_NUM; i++)
		{	
			if((BITMAP[i])) frame_swap_obtenido = i;
		}

    	memcpy(buffer_pag_upcm,&MAIN_MEMORY[pag_memoria->frame*TAM_PAGINA],TAM_PAGINA);		

		fseek(archivo_swap,frame_swap_obtenido*TAM_PAGINA,SEEK_SET);
		fwrite(buffer_pag_upcm,TAM_PAGINA,1,archivo_swap);

		pag_memoria->frame = frame_swap_obtenido;
    	pag_memoria->bit_presencia = 0;

      	pag->frame = proximo_frame_libre();
    	pag->bit_presencia = 1;
    	pag->bit_usado = 1;

    	// si este metodo no sirve, agregar un campo length_mmap en t_segmento
    	if((nro_pagina*TAM_PAGINA) <= seg->tam_archivo_mmap){
    		fseek(seg->archivo_mapeado,nro_pagina*TAM_PAGINA,SEEK_SET);

    		int bytes_a_leer = (int)fmin(TAM_PAGINA,((nro_pagina*TAM_PAGINA) + TAM_PAGINA) - seg->tam_archivo_mmap);

    		fread(buffer_pag_mmap,bytes_a_leer,1,seg->archivo_mapeado);
    		memcpy(&archivo_mapeado[pag->frame*TAM_PAGINA],buffer_pag_mmap,bytes_a_leer);

    		if(TAM_PAGINA > bytes_a_leer)
    			memset(&archivo_mapeado[pag->frame*bytes_a_leer],'\0',TAM_PAGINA - bytes_a_leer);
    	}
    	else{
    		// el primer byte a leer supera el tamano del archivo
    		memset(&archivo_mapeado[pag->frame*TAM_PAGINA],'\0',TAM_PAGINA);
    	}

    	agregar_frame_clock(pag);

    	free(buffer_pag_upcm);
    	free(buffer_pag_mmap);
    	fclose(archivo_swap);
	}
	else if(!pag->bit_presencia && espacio_en_upcm()){
    	void* buffer_pag_mmap = malloc(TAM_PAGINA);

    	pag->frame = proximo_frame_libre();
    	pag->bit_presencia = 1;
    	pag->bit_usado = 1;

		fseek(seg->archivo_mapeado,nro_pagina * TAM_PAGINA,SEEK_SET);
		fread(buffer_pag_mmap, TAM_PAGINA, 1, seg->archivo_mapeado);

    	memcpy(&archivo_mapeado[pag->frame*TAM_PAGINA], buffer_pag_mmap, TAM_PAGINA);

    	agregar_frame_clock(pag);

    	free(buffer_pag_mmap);
	}

	return (char*) archivo_mapeado + (pag->frame * TAM_PAGINA);
}

void agregar_frame_clock(page* pag){
	
	if(list_size(lista_clock) == cantidad_frames){
		list_replace(lista_clock,pag->frame,pag);
	}
	else{
		//list_add_in_index(lista_clock,pagina->frame,pagina);
		list_add_in_index(lista_clock,pag->frame,pag);
	}
}

void* convertir_de_nro_frame_a_posicion (int nro_frame)
{

}

int convertir_de_posicion_a_nro_de_frame (void* posicion)
{
	
}
*/

void liberar_frame(void* frame){
	int numeroFrame = (frame - MAIN_MEMORY)/PAGE_SIZE;

	BITMAP[numeroFrame] = 1;
}

void liberar_frame_swap(void* frame){
	int numeroFrame = (frame - MAIN_MEMORY)/PAGE_SIZE;

	BITMAP_SWAP_FILE[numeroFrame] = 1;
}

void eliminar_archivo_mmap(segment* segmentoABorrar){

	if(segmentoABorrar->page_table != NULL){

		void eliminar_pagina(page* pagina){
			if(pagina->is_present){
				liberar_frame(pagina->fr);
			}

		}
		list_destroy_and_destroy_elements(segmentoABorrar->page_table,(void*) eliminar_pagina);
	}
}

int memory_unmap(uint32_t dir, uint32_t pid)
{
	int nro_prog;
	program* prog;

	if((nro_prog = search_program(pid)) == -1)
	{
		prog = (program *) malloc(sizeof(program));
		prog->pid = pid;
		prog->segment_table = list_create();
		nro_prog = list_add(program_list, prog);
		log_debug(debug_logger, "Se creo el prog n°%d de la lista de programas ", nro_prog);
	}

	prog = list_get(program_list, nro_prog);
	int nroSegmento = busca_segmento(prog, dir);

	segment* segmentoBuscado = list_get(prog->segment_table, nroSegmento);

	if(segmentoBuscado == NULL){
	log_error(debug_logger, "Segmentation fault");
		return -1;
	}
	else if((segmentoBuscado->is_heap != 0) || (dir != segmentoBuscado->base)){
	log_error(debug_logger, "No es de MMAP el segmento buscado.");
		return -1;
	}

	archivoMMAP* archivoMapeado = segmentoBuscado->archivo_mapeado;
	log_debug(debug_logger, "El archivo mapeado tiene: Path: %s", archivoMapeado->pathArchivo);

	int igualPID(int pid) {
		return pid == prog->pid;
	}
	list_remove_by_condition(archivoMapeado->programas,(void*) igualPID);

	if(list_size(archivoMapeado->programas) == 0){
		list_destroy(archivoMapeado->programas);
		munmap(segmentoBuscado->archivo_mapeado, segmentoBuscado->tam_archivo_mmap);

		log_debug(debug_logger, "Eliminando archivo MMAP");
		bool igualArchivo(archivoMMAP* archivo) {
				return !strcmp(archivo->pathArchivo, archivoMapeado->pathArchivo);
		}
		list_remove_and_destroy_by_condition(lista_archivos_mmap,(void*) igualArchivo,(void*) eliminar_archivo_mmap);
	}

	int igualSegmento(segment* segmento){
		log_debug(debug_logger, "la dir del segmento es: %d", &segmento);
		log_debug(debug_logger, "la dir del segmentoBuscado es: %d", &segmentoBuscado);
		return &segmentoBuscado == &segmento;
	}
	list_remove_by_condition(segment_list,(void*) igualSegmento);

	log_debug(debug_logger, "Elimine todas las paginas", nro_prog);

	list_remove(prog->segment_table, nroSegmento);

	free(segmentoBuscado);

	//destruir la tabla de paginas

	log_debug(debug_logger, "Unmap terminado");

	return 0;
}
