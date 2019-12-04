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

void *MAIN_MEMORY = NULL;

/*int floor(float num)
{
    return (int)num;
}*/

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
	heap_metadata* metadata;
	int curr_page_num;	
	void *mem_ptr = MAIN_MEMORY;
	PAGE_SIZE = page_size;

	MAIN_MEMORY = (void *) malloc(memory_size); // aka UPCM

	metadata = (heap_metadata*) malloc(METADATA_SIZE);
	TOTAL_FRAME_NUM = (memory_size / PAGE_SIZE);
	//BITMAP = (bool*) malloc(sizeof(bool)*TOTAL_FRAME_NUM); //1 if free;
	BITMAP[TOTAL_FRAME_NUM];

	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{	
		BITMAP[i] = 1;
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
	log_trace(metricas_logger, "Cantidad de Frames libres: %d / %d ", frames_libres, TOTAL_FRAME_NUM);

	return frames_libres;
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

/*
void addr_new_segment(uint32_t size_total, uint32_t pid){
	segment* seg;
	heap_metadata* metadata1;
	int nro_pag;

	int nro_prog = search_program(pid);
	program *prog = list_get(program_list, nro_prog);
	list_add(prog->segment_table,seg);
	page* pag;
	page* pagina;

	int framesNeeded = frames_needed(size_total);

	if(number_of_free_frames()>size_total)
	{
		int nor_frame_libre = proximo_frame_libre();

		pag = page_with_free_size(PAGE_SIZE);
		nro_pag = list_add(seg->page_table, pag);

		pagina = list_get(seg->page_table, nro_pag);

		pagina->fr = MAIN_MEMORY + nor_frame_libre;

		metadata1 = (heap_metadata*) (MAIN_MEMORY + nor_frame_libre*PAGE_SIZE);
		metadata1->is_free = 1;
		metadata1->size= size_total;

		//vamos a pedir todo el resto de paginas que necesitemos
		for(int i=0 ; i < framesNeeded - 1 ; i++ )
		{
			pag = *page_with_free_size(PAGE_SIZE);
			list_add(seg->page_table, pag);
		}
	}
}
*/

//is_first_page y totalsize son los valores que se usan en la 1ra pagina pedida para meter la metadata
//is_last_page y freesize (es el espacio libre que le queda a la última pag) la usaremos para la última metadata 
//(no, is_first_page y is_last_page no son mutuamente excluyentes)
page* page_with_free_size(){
	heap_metadata* metadata;
	heap_metadata* metadata2;

	int curr_frame_num;

	page* pag = (page *) malloc(PAGE_SIZE);

	for(curr_frame_num=0; true ; curr_frame_num++)
	{	
		if (curr_frame_num == TOTAL_FRAME_NUM){
			log_debug(debug_logger, "Nos quedamos sin frames libres, aplicando algoritmo de reemplazo");
			//curr_frame_num = dame_nro_frame_reemplazado();
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
	//return NULL;
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
	int nro_de_segmento;
	int nro_de_pag;
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
			
			for(nro_de_pag = 0; nro_de_pag < cantidad_de_paginas_en_segmento; nro_de_pag++)
			{
				pag =  list_get(seg->page_table,nro_de_pag);
				U = pag->is_used;
				M = pag->is_modify;
				if (nro_paso == 1){
					if (!U && !M){
						pag->is_present = false;
						//pag->fr = direccion_disco_donde_mandamos(pag);
						log_debug(debug_logger, "Se eligio como victimica a la pagina %d del segmento %d",
								nro_de_pag, nro_de_segmento);
						return (BITMAP[1] = 1);						
					} 
				}
				if (nro_paso == 2){
					if (!U && M){
						pag->is_present = false;
						//pag->fr = direccion_disco_donde_mandamos(pag);
						log_debug(debug_logger, "Se eligio como victimica a la pagina %d del segmento %d",
								nro_de_pag, nro_de_segmento);
						return (BITMAP[1] = 1);
					}
					U = 0;
				}		
				if (nro_paso == 3){
					nro_paso = 0;
				}
				
			}
		}
		nro_paso++;		
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

heap_metadata* buscar_metadata_por_direccion(int direccionLogica, segment* segmentoBuscado){
	heap_metadata* metadataBuscada;

	int paginaBuscada = floor((direccionLogica - segmentoBuscado->base) / PAGE_SIZE);
	int offset = (direccionLogica - segmentoBuscado->base) % PAGE_SIZE;

	page* paginaAEscribir = list_get(segmentoBuscado->page_table, paginaBuscada);

	metadataBuscada = (heap_metadata*) ((paginaAEscribir->fr) + offset);

	return metadataBuscada;
}

//Es la misma que la de abajo pero repite la logica porque esta hecha para usarse sola, es una boludez pero bueno
//Estaria  bueno saber como hacer para que no repita
heap_metadata* proxima_metadata(int paginaABuscar, int offset, segment* segmento){
	page* paginaInicial = list_get(segmento->page_table, paginaABuscar);
	heap_metadata* metadataActual = (heap_metadata*) ((paginaInicial->fr) + offset);

	heap_metadata* metadataSiguiente;

	int bytesAMoverme = metadataActual->size + METADATA_SIZE;
	//Si la proxima metadata queda en otra pagina, me muevo hasta ahi
	if(bytesAMoverme > PAGE_SIZE){

		int cantidadDePaginasAMoverme = floor(bytesAMoverme/PAGE_SIZE);
		int offsetDentroDeLaPagina = (bytesAMoverme % PAGE_SIZE);

		log_debug(debug_logger, "Tengo que buscar: paginasAMoverme: %d, offsetdentrodePagina: %d, estoy en la pagina: %d, offset: %d",
				cantidadDePaginasAMoverme,
				offsetDentroDeLaPagina,
				paginaABuscar,
				offset);

		page* paginaSiguiente = list_get(segmento->page_table, paginaABuscar + cantidadDePaginasAMoverme);

		if (paginaSiguiente == NULL) return NULL;

		heap_metadata* proximaMetadata = (heap_metadata*) ((paginaSiguiente->fr) + offsetDentroDeLaPagina);

		if((offsetDentroDeLaPagina + METADATA_SIZE) > PAGE_SIZE){
			heap_metadata metadataCopia;

			page* proximaPagina = list_get(segmento->page_table, (paginaABuscar + cantidadDePaginasAMoverme + 1));
			void* punteroAlFrameSiguiente = proximaPagina->fr;
			int tamanioMetadataCortada = (PAGE_SIZE) - offsetDentroDeLaPagina;

			memcpy(&metadataCopia, proximaMetadata, tamanioMetadataCortada);
			memcpy((void*)(&metadataCopia) + 2, punteroAlFrameSiguiente, METADATA_SIZE - tamanioMetadataCortada);

			memcpy(proximaMetadata, &metadataCopia, tamanioMetadataCortada);
			memcpy(punteroAlFrameSiguiente, (void*)(&metadataCopia) + 2, METADATA_SIZE - tamanioMetadataCortada);

			metadataSiguiente = proximaMetadata;
		}else{
			metadataSiguiente = proximaMetadata;
		}

		log_debug(debug_logger, "Metadata siguiente tiene: size: %d, is_free: %d",
								metadataSiguiente->size,
								metadataSiguiente->is_free);

	//En este caso la proxima metadata queda dentro de la misma pagina
	}else{
		page* paginaActual = list_get(segmento->page_table, paginaABuscar);

		metadataSiguiente = (heap_metadata*) ((paginaActual->fr) + bytesAMoverme);
	}

	return metadataSiguiente;
}

heap_metadata* metadata_siguiente(int paginaABuscar, int offset, int *cantidadDePaginasAMoverme, int *offsetDentroDeLaPagina, segment* segmento){
	page* paginaInicial = list_get(segmento->page_table, paginaABuscar);
	heap_metadata* metadataActual = (heap_metadata*) ((paginaInicial->fr) + offset);

	heap_metadata* metadataSiguiente;

	int bytesAMoverme = metadataActual->size + METADATA_SIZE;
	//Si la proxima metadata queda en otra pagina, me muevo hasta ahi
	if(bytesAMoverme > PAGE_SIZE){

		*cantidadDePaginasAMoverme = floor(bytesAMoverme/PAGE_SIZE);
		*offsetDentroDeLaPagina = (bytesAMoverme % PAGE_SIZE);

		log_debug(debug_logger, "Tengo que buscar: paginasAMoverme: %d, offsetdentrodePagina: %d, estoy en la pagina: %d, offset: %d",
				*cantidadDePaginasAMoverme,
				*offsetDentroDeLaPagina,
				paginaABuscar,
				offset);

		if((paginaABuscar + *cantidadDePaginasAMoverme) > segmento->page_table->elements_count){
			return NULL;
		}

		page* paginaSiguiente = list_get(segmento->page_table, paginaABuscar + *cantidadDePaginasAMoverme);

		if (paginaSiguiente == NULL) return NULL;

		heap_metadata* proximaMetadata = (heap_metadata*) ((paginaSiguiente->fr) + *offsetDentroDeLaPagina);

		if((*offsetDentroDeLaPagina + METADATA_SIZE) > PAGE_SIZE){
			heap_metadata metadataCopia;

			page* proximaPagina = list_get(segmento->page_table, (paginaABuscar + cantidadDePaginasAMoverme + 1));
			void* punteroAlFrameSiguiente = proximaPagina->fr;
			int tamanioMetadataCortada = (PAGE_SIZE) - *offsetDentroDeLaPagina;

			memcpy(&metadataCopia, proximaMetadata, tamanioMetadataCortada);
			memcpy((void*)(&metadataCopia) + 2, punteroAlFrameSiguiente, METADATA_SIZE - tamanioMetadataCortada);

			memcpy(proximaMetadata, &metadataCopia, tamanioMetadataCortada);
			memcpy(punteroAlFrameSiguiente, (void*)(&metadataCopia) + 2, METADATA_SIZE - tamanioMetadataCortada);

			metadataSiguiente = proximaMetadata;
		}else{
			metadataSiguiente = proximaMetadata;
		}

		log_debug(debug_logger, "Metadata siguiente tiene: size: %d, is_free: %d",
								metadataSiguiente->size,
								metadataSiguiente->is_free);

	//En este caso la proxima metadata queda dentro de la misma pagina
	}else{
		page* paginaActual = list_get(segmento->page_table, paginaABuscar);

		int bytesRestantesEnLaPagina = PAGE_SIZE - offset;

		if((bytesRestantesEnLaPagina - bytesAMoverme) >= METADATA_SIZE){
			metadataSiguiente = (heap_metadata*) ((paginaActual->fr) + bytesAMoverme + offset);
			*offsetDentroDeLaPagina = (bytesAMoverme % PAGE_SIZE);
		}else{
			return NULL;
		}
	}

	return metadataSiguiente;
}

int tiene_un_siguiente(heap_metadata* metadataActual, int paginaABuscar, int offset, segment* segmento){
	heap_metadata* metadataSiguiente;
	int cantidadDePaginasAMoverme = 0;
	int offsetDentroDeLaPagina = 0;

	metadataSiguiente = metadata_siguiente(paginaABuscar, offset, &cantidadDePaginasAMoverme, &offsetDentroDeLaPagina, segmento);

	if(metadataSiguiente == NULL){
		return 0;
	}

	return 1;
}

int ultima_metadata_segmento(int paginaABuscar, int offset, segment* segmento, int cantidadDePaginasMovidas, int offsetTotalMovido){
	int cantidadDePaginasAMoverme = 0;
	int offsetDentroDeLaPagina = 0;
	heap_metadata* metadataSiguiente;

	metadataSiguiente = metadata_siguiente(paginaABuscar, offset, &cantidadDePaginasAMoverme, &offsetDentroDeLaPagina, segmento);

	paginaABuscar = cantidadDePaginasAMoverme;
	offset = offsetDentroDeLaPagina;

	cantidadDePaginasMovidas += paginaABuscar;
	offsetTotalMovido += offset;

	if(!tiene_un_siguiente(metadataSiguiente, cantidadDePaginasAMoverme, offsetDentroDeLaPagina, segmento)){
		int direccionLogicaMetadataLibre = cantidadDePaginasMovidas * PAGE_SIZE + offsetTotalMovido;
		return direccionLogicaMetadataLibre;
	}else{
		return ultima_metadata_segmento(cantidadDePaginasAMoverme, offsetDentroDeLaPagina, segmento, cantidadDePaginasMovidas, offsetTotalMovido);
	}

}

//Esto cuando se usa la idea es pasarle inicialmente -> PaginaABuscar = 0, Offset = 0, Segmento = el segmento que estemos queriendo recorrer
int proxima_metadata_libre(int paginaABuscar, int offset, segment* segmento, int size, int cantidadDePaginasMovidas, int offsetTotalMovido){
	int cantidadDePaginasAMoverme = 0;
	int offsetDentroDeLaPagina = 0;

	heap_metadata* metadataSiguiente;

	if(paginaABuscar == 0 && offset == 0){
		heap_metadata* primeraMetadata;

		page* primeraPagina = list_get(segmento->page_table, 0);

		primeraMetadata = (heap_metadata *) (primeraPagina->fr);

		if(primeraMetadata->is_free && primeraMetadata->size >= size){
			return 0;
		}
	}

	metadataSiguiente = metadata_siguiente(paginaABuscar, offset, &cantidadDePaginasAMoverme, &offsetDentroDeLaPagina, segmento);

	paginaABuscar = cantidadDePaginasAMoverme;
	offset = offsetDentroDeLaPagina;

	cantidadDePaginasMovidas += paginaABuscar;
	offsetTotalMovido += offset;

	if(metadataSiguiente == NULL){
		return -1;
	}else if(metadataSiguiente->is_free && metadataSiguiente->size >= size){
		int direccionLogicaMetadataLibre = cantidadDePaginasMovidas * PAGE_SIZE + offsetTotalMovido;
		return direccionLogicaMetadataLibre;
	}else{
		return proxima_metadata_libre(cantidadDePaginasAMoverme, offsetDentroDeLaPagina, segmento, size, cantidadDePaginasMovidas, offsetTotalMovido);
	}
}

uint32_t memory_malloc(int size, uint32_t pid)
{	
	if (size <= 0) return -1;
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
	
	log_debug(debug_logger, "Se pidieron %d bytes, + metadata = %d ", size, total_size);
	
	//si el programa no está en la lista de programas agregamos el programa a la lista de programas y le creamos una nueva tabla de segmentos
	if((nro_prog = search_program(pid)) == -1)
	{
		prog = (program *) malloc(sizeof(program));		
		prog->pid = pid;
		prog->segment_table = list_create();
		nro_prog = list_add(program_list, prog);
		log_debug(debug_logger, "Se creo el prog n°%d de la lista de programas ", nro_prog);
	}	 

	prog = list_get(program_list, nro_prog);	
	
	//si hay espacio en su segmento, lo malloqueo ahí
	if(segment_with_free_space(prog, total_size) != -1)
	{
		log_debug(debug_logger, "Encontre espacio en el segmento, alloco aca");
		segment *segmentoConEspacio;
		heap_metadata *metadataBuscada;
		heap_metadata *metadataFinal;

		nro_seg = segment_with_free_space(prog, total_size);
		segmentoConEspacio = list_get(prog->segment_table, nro_seg);

		direccionLogicaMetadataLibre = proxima_metadata_libre(0, 0, segmentoConEspacio, total_size, 0, 0);

		log_debug(debug_logger, "La direccion logica de la metadata donde voy a allocar esta en ---> %d", direccionLogicaMetadataLibre);

		metadataBuscada = buscar_metadata_por_direccion(direccionLogicaMetadataLibre, segmentoConEspacio);

		//Alloco aca

		int sizeAnterior = metadataBuscada->size;

		metadataBuscada->is_free = 0;
		metadataBuscada->size = size;

		log_debug(debug_logger, "Nuevo size de la metadata ---> %d", metadataBuscada->size);

		int paginaDeLaMetadataLibre = floor((direccionLogicaMetadataLibre - segmentoConEspacio->base) / PAGE_SIZE);
		int offset = (direccionLogicaMetadataLibre - segmentoConEspacio->base) % PAGE_SIZE;

		metadataFinal = proxima_metadata(paginaDeLaMetadataLibre, offset, segmentoConEspacio);

		if((offset + METADATA_SIZE) > PAGE_SIZE){
			heap_metadata metadataCopia;

			page* proximaPagina = list_get(segmentoConEspacio->page_table, (paginaDeLaMetadataLibre + 1));
			void* punteroAlFrameSiguiente = proximaPagina->fr;
			int tamanioMetadataCortada = (PAGE_SIZE) - offset;

			memcpy(&metadataCopia, metadataFinal, tamanioMetadataCortada);
			memcpy((void*)(&metadataCopia) + 2, punteroAlFrameSiguiente, METADATA_SIZE - tamanioMetadataCortada);

			metadataCopia.is_free = 1;
			metadataCopia.size = sizeAnterior - total_size;

			memcpy(metadataFinal, &metadataCopia, tamanioMetadataCortada);
			memcpy(punteroAlFrameSiguiente, (void*)(&metadataCopia) + 2, METADATA_SIZE - tamanioMetadataCortada);
			log_debug(debug_logger, "La metadata final tiene ---> %d", metadataFinal->size);
		}else{
			metadataFinal->is_free = 1;
			metadataFinal->size = sizeAnterior - total_size;
			log_debug(debug_logger, "La metadata final tiene ---> %d", metadataFinal->size);
		}
	}
	//Si su ultimo segmento es heap y tiene memoria están los frames que necesitamos
	else if (list_size(prog->segment_table)>0 && ultimo_segmento_programa(prog)->is_heap && number_of_free_frames() >= total_pages_needed)
	{
		log_debug(debug_logger, "Intento agrandar el que tiene");
		segment *segmentoAAgrandar;
		segmentoAAgrandar = ultimo_segmento_programa(prog);

		//Busco la ultima metadata que deberia estar libre y la agrando
		heap_metadata* ultimaMetadata;

		int direccionLogicaUltimaMetadata = ultima_metadata_segmento(0, 0, segmentoAAgrandar, 0, 0);

		ultimaMetadata = buscar_metadata_por_direccion(direccionLogicaUltimaMetadata, segmentoAAgrandar);

		int sizeAnterior = ultimaMetadata->size;
		float cantidadDePaginasAAgrandar = ceil(((float)(total_size - sizeAnterior))/PAGE_SIZE);

		ultimaMetadata->is_free = 1;
		ultimaMetadata->size = cantidadDePaginasAAgrandar * PAGE_SIZE;

		for(int i=0 ; i < cantidadDePaginasAAgrandar ; i++ )
		{
			log_debug(debug_logger, "Size solicitado para PAGE_WITH_FREE_SIZE: %d", PAGE_SIZE);
			pag = page_with_free_size();
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
		nro_seg = list_add(prog->segment_table,segmentoNuevo);
		log_debug(debug_logger, "Se creo el segmento n°%d ", nro_seg);	
		list_add(segment_list, segmentoNuevo);

		for(int i=0 ; i < cantidadDePaginasAAgregar ; i++ )
		{
			log_debug(debug_logger, "Size solicitado para PAGE_WITH_FREE_SIZE: %d", PAGE_SIZE);
			pag = page_with_free_size();
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
	
	metricas_por_socket_conectado(pid);
	number_of_free_frames();

	int direccionLogicaFinal = direccionLogicaMetadataLibre + METADATA_SIZE;
	log_debug(debug_logger, "Direccion logica final del MALLOC ----> %d", direccionLogicaFinal);


	return direccionLogicaFinal;
}


int segment_with_free_space(program *prog, int size)
{
	int i=0;
	segment *segmentoActual;
	heap_metadata* metadataActual;
	heap_metadata* primerMetadata;
	int cantidadDeSegmentos = list_size(prog->segment_table);

	if (cantidadDeSegmentos == 0) return -1;

	while(i < cantidadDeSegmentos)
	{		
		segmentoActual = list_get(prog->segment_table, i);

		if (segmentoActual->is_heap)
		{
			page* primerPagina = list_get(segmentoActual->page_table, 0);
			
			primerMetadata = (heap_metadata *) (primerPagina->fr);
			metadataActual = primerMetadata;

			if(!metadataActual->is_free){
				int direccionLogicaMetadataLibre = proxima_metadata_libre(0, 0, segmentoActual, size, 0, 0);

				if(direccionLogicaMetadataLibre != -1){
					metadataActual = buscar_metadata_por_direccion(direccionLogicaMetadataLibre, segmentoActual);
					log_debug(debug_logger, "Encontramos que el segmento %d tiene una metadata de %d de size", i, metadataActual->size);
					return i;
				}

			}else if(metadataActual->size >= size){
				log_debug(debug_logger, "Encontramos que el segmento %d tiene una metadata de %d de size", i, metadataActual->size);
				return i;
			}
		}
		i++;
	}
	log_debug(debug_logger, "No habia ningun segmento con %d de espacio libre", size);
	return -1;
}

void compactar_en_segmento(int posicionActual, heap_metadata* metadataInicial, heap_metadata* metadataActual, int paginaActualNumero, segment* segmento){
	/*heap_metadata* metadataSiguiente = metadata_siguiente(posicionActual, metadataActual, paginaActualNumero, segmento); TODO

	if(metadataSiguiente->is_free){
		metadataInicial->size += metadataSiguiente->size;
		metadataActual = metadataSiguiente;
		compactar_en_segmento(posicionActual, metadataInicial, metadataActual, paginaActualNumero, segmento);
	}else{
		metadataInicial = proxima_metadata_libre(posicionActual, metadataSiguiente, paginaActualNumero, segmento); TODO cambiar esto
		metadataActual = metadataInicial;
		compactar_en_segmento(posicionActual, metadataInicial, metadataActual, paginaActualNumero, segmento);
	}*/
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

		int cantidadDePaginasDelSegmento = list_size(segmentoActual->page_table);

		page* primerPagina = list_get(segmentoActual->page_table, 0);

		heap_metadata* primerMetadata = (primerPagina->fr);
		metadataInicialLibre = primerMetadata;

		if(!primerMetadata->is_free){
			int direccionLogicaMetadataLibre = proxima_metadata_libre(0, 0, segmentoActual, 0, 0, 0);
		}

		//compactar_en_segmento(posicionActual, metadataActual, metadataActual, 0, segmentoActual);
	}

}

uint8_t memory_free(uint32_t virtual_address, uint32_t pid)
{
	int nro_prog = search_program(pid);
	program *prog;
	segment *seg;
	page *pag;	
	heap_metadata* metadata;

	prog = list_get(program_list, nro_prog);
	int nro_seg = busca_segmento(prog,virtual_address);	
	seg = list_get(prog->segment_table,nro_seg);
	int base = seg->base;
	int numero_pagina = (virtual_address - base) / PAGE_SIZE;
	pag = list_get(seg->page_table,numero_pagina);
	int offset = (virtual_address - base) - (numero_pagina * PAGE_SIZE);
	printf("%d representa el seg %d pag %d offset %d \n",virtual_address,nro_seg,numero_pagina,offset);
	
	//free_page(pag);

	metadata = (pag->fr) + offset - METADATA_SIZE;
	metadata->is_free=1;

	compactar_espacios_libres(prog);

	return 0;
}

segment* ultimo_segmento_programa(program *prog){
	segment* seg;;
	int ultimo_elemento = list_size(prog->segment_table)-1;
	seg = list_get(prog->segment_table, ultimo_elemento );
	return seg;
}

void crear_nuevo_segmento_mmap(size_t length, void* map, uint32_t pid){
	segment* seg;
	page* pag;
	seg = (segment *) malloc(sizeof(segment));
	seg->is_heap = false;
	seg->page_table = list_create();
	//seg->base = (ultimo_segmento_programa(pid)->limit+1);
	seg->limit = (seg->base) + length;

	int nro_pag;
	int total_pages_needed;

	total_pages_needed = (length / PAGE_SIZE) + ((length % PAGE_SIZE) != 0); // ceil(length / PAGE_SIZE);

	//vamos a pedir todo el resto de paginas que necesitemos
	for(int i=0 ; i < total_pages_needed ; i++ )
	{
		pag = (page *) malloc(sizeof(page));
		pag->is_used = true;
		pag->is_modify = false;
		pag->is_present = false;
		nro_pag = list_add(seg->page_table, pag);
	}

	seg->archivo_mapeado = map;

	//list_add(prog->segment_list, seg);
	list_add(segment_list, seg);

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
		direccionFisicaBuscada = cargarPaginaAMemoria(segmentoBuscado, bytesNecesarios, paginaBuscada) + offset;
	}else{
		direccionFisicaBuscada = (uint32_t) ((paginaAEscribir->fr) + offset);
	}

	return direccionFisicaBuscada;
}
// Copia n bytes de MUSE a LIBMUSE
// No se usan ni dst ni src
uint32_t memory_get(void *dst, uint32_t src, size_t numBytes, uint32_t pid)
{
	int direccionFisicaBuscada = buscar_direccion_fisica(src, numBytes, pid);
	memcpy(dst, (void*) direccionFisicaBuscada, numBytes);

	return src;

}

//Copia n bytes de LIBMUSE a MUSE
uint32_t memory_cpy(uint32_t dst, void *src, int n, uint32_t pid)
{
	int direccionFisicaBuscada = buscar_direccion_fisica(dst, n, pid);
	memcpy((void*) direccionFisicaBuscada, src, n);
	
	return dst;
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

	crear_nuevo_segmento_mmap(length,map,pid);   

	return *(int*) map; 

	// How does mmap works:
	// mmap() creates a new mapping in the virtual address space of the calling process.
	// addr specifies address for the file, length specifies the length of the mapping 
	// void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);   
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

uint32_t memory_sync(uint32_t addr, size_t len, uint32_t pid)
{
	/*synchronize a file with a memory map*/
	//msync(addr, len, MS_SYNC);
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
