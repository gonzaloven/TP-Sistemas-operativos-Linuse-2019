/*
* main_memory.c
* Estructura administrativa de la memoria,
* Permite inizializar memoria, liberarla, 
* buscar un programa en la tabla de programas, 
* y buscar páginas libre
*/

#include "main_memory.h"

#define METADATA_SIZE sizeof(struct HeapMetadata)

#define SWAP_PATH "swapfile"

void *MAIN_MEMORY = NULL;
t_list *program_list = NULL;
t_list *lista_archivos_mmap = NULL;
t_log *metricas_logger = NULL;
t_log *debug_logger = NULL;

int paginaClock = 0;
int segmentoClock = 0;

int PAGE_SIZE = 0;
int TOTAL_FRAME_NUM = 0;

// inicializadas only for debugging reasons
bool BITMAP[140];
bool BITMAP_SWAP_FILE[2048];
int TOTAL_FRAME_NUM_SWAP;

void muse_main_memory_init(int memory_size, int page_size, int swap_size)
{
	int i;
	// heap_metadata* metadata;
	// metadata = (heap_metadata*) malloc(METADATA_SIZE);
	pthread_mutex_init(&mutex_bitmap_heap, NULL);
	pthread_mutex_init(&mutex_bitmap_mmap, NULL);
	pthread_mutex_init(&mutex_MM, NULL);
	pthread_mutex_init(&mutex_clock, NULL);
	pthread_mutex_init(&mutex_segment, NULL);
	pthread_mutex_init(&mutex_prox_lib, NULL);
	pthread_mutex_init(&mutex_carga_pag, NULL);

	int curr_page_num;	
	void *mem_ptr = MAIN_MEMORY;
	PAGE_SIZE = page_size;
	lista_archivos_mmap = list_create();

	MAIN_MEMORY = (void *) malloc(memory_size); // aka UPCM
	memset(MAIN_MEMORY, 0, memory_size);

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
	//segment_list = list_create();

	debug_logger = log_create(MUSE_LOG_PATH,"DEBUG", true,LOG_LEVEL_INFO);
	metricas_logger = log_create(MUSE_LOG_PATH, "METRICAS", true, LOG_LEVEL_INFO);

	log_info(metricas_logger, "Tamaño de pagina = tamaño de frame: %d", PAGE_SIZE);
	log_info(metricas_logger, "Tamaño de metadata: %d", METADATA_SIZE);
	log_info(metricas_logger, "Cantidad Total de Memoria:%d", memory_size);
	number_of_free_frames();
}


void muse_main_memory_stop()
{
	free(MAIN_MEMORY);
	remove(SWAP_PATH);

	list_destroy(lista_archivos_mmap);
	list_destroy_and_destroy_elements(program_list, (void*)destroy_program_list_elements);
	//list_destroy(segment_list);
	pthread_mutex_destroy(&mutex_bitmap_heap);
	pthread_mutex_destroy(&mutex_bitmap_mmap);
	pthread_mutex_destroy(&mutex_MM);
	pthread_mutex_destroy(&mutex_clock);
	pthread_mutex_destroy(&mutex_segment);
	pthread_mutex_destroy(&mutex_prox_lib);
	pthread_mutex_destroy(&mutex_carga_pag);

	log_destroy(metricas_logger);
	log_destroy(debug_logger);
}

void destroy_archivosmmap_list_elements(archivoMMAP* mmap){
	list_destroy(mmap->programas);
	free(mmap->pathArchivo);
	free(mmap);
}

void destroy_program_list_elements(program* prog){
	list_destroy_and_destroy_elements(prog->segment_table, (void*)destroy_segment_table_elements);
	free(prog);
	prog = NULL;
}

void destroy_segment_table_elements(segment* seg){
	list_destroy_and_destroy_elements(seg->page_table, (void*)destroy_page_table_elements);
	if(!seg->is_heap){
		list_destroy(seg->archivo_mapeado->programas);
		free(seg->archivo_mapeado->pathArchivo);
		free(seg->archivo_mapeado);
	}
	free(seg);
	seg = NULL;
}

void destroy_page_table_elements(page* pag){
	free(pag);
	pag = NULL;
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

void el_cliente_se_tomo_el_palo(uint32_t pid){
	log_debug(debug_logger,"El pid del programa es: %d parte 3",pid);
	int nro_prog = search_program(pid);
	program *prog = list_get(program_list, nro_prog);
	int memory_leaks = prog->using_memory;
	log_trace(metricas_logger, "LEAK SUMMARY: definitely lost: %d bytes", memory_leaks);
}

int number_of_free_frames_upcm(){
	int memoria_libre = 0;
	int memoria_total = 0;
	int frames_libres_1 = 0;
	int frames_libres_2 = 0;
	int i;

	pthread_mutex_lock(&mutex_bitmap_heap);
	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{
		frames_libres_1 += (BITMAP[i]);
	}
	pthread_mutex_unlock(&mutex_bitmap_heap);

	memoria_libre = frames_libres_1 * PAGE_SIZE;
	memoria_total = TOTAL_FRAME_NUM * PAGE_SIZE;

	log_info(metricas_logger, "Cantidad de Memoria libre en UPCM: %d / %d", memoria_libre, memoria_total);
	log_info(metricas_logger, "Cantidad de Frames libres en UPCM: %d / %d", frames_libres_1, TOTAL_FRAME_NUM);

	/* SWAP SPACE */

	memoria_libre = 0;

	pthread_mutex_lock(&mutex_bitmap_mmap);
	for(i=0; i < TOTAL_FRAME_NUM_SWAP; i++)
	{
		frames_libres_2 += (BITMAP_SWAP_FILE[i]);
	}
	pthread_mutex_unlock(&mutex_bitmap_mmap);

	memoria_libre = frames_libres_2 * PAGE_SIZE;
	memoria_total = TOTAL_FRAME_NUM_SWAP * PAGE_SIZE;

	log_info(metricas_logger, "Cantidad de Espacio libre en SWAP: %d / %d", memoria_libre, memoria_total);
	log_info(metricas_logger, "Cantidad de Frames libres en SWAP: %d / %d", frames_libres_2, TOTAL_FRAME_NUM_SWAP);

	/* FIN SWAP SPACE */

	return frames_libres_1;

}

int number_of_free_frames(){
	int memoria_libre = 0; 
	int memoria_total = 0;
	int frames_libres_1 = 0;
	int frames_libres_2 = 0;
	int i;

	pthread_mutex_lock(&mutex_bitmap_heap);
	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{	
		frames_libres_1 += (BITMAP[i]);
	}
	pthread_mutex_unlock(&mutex_bitmap_heap);

	memoria_libre = frames_libres_1 * PAGE_SIZE;
	memoria_total = TOTAL_FRAME_NUM * PAGE_SIZE;

	log_info(metricas_logger, "Cantidad de Memoria libre en UPCM: %d / %d", memoria_libre, memoria_total);
	log_info(metricas_logger, "Cantidad de Frames libres en UPCM: %d / %d", frames_libres_1, TOTAL_FRAME_NUM);

	/* SWAP SPACE */

	memoria_libre = 0; 

	pthread_mutex_lock(&mutex_bitmap_mmap);
	for(i=0; i < TOTAL_FRAME_NUM_SWAP; i++)
	{	
		frames_libres_2 += (BITMAP_SWAP_FILE[i]);
	}
	pthread_mutex_unlock(&mutex_bitmap_mmap);

	memoria_libre = frames_libres_2 * PAGE_SIZE;
	memoria_total = TOTAL_FRAME_NUM_SWAP * PAGE_SIZE;

	log_info(metricas_logger, "Cantidad de Espacio libre en SWAP: %d / %d", memoria_libre, memoria_total);
	log_info(metricas_logger, "Cantidad de Frames libres en SWAP: %d / %d", frames_libres_2, TOTAL_FRAME_NUM_SWAP);

	/* FIN SWAP SPACE */

	return frames_libres_1 + frames_libres_2;
}

int proximo_frame_libre(){
	int i;

	pthread_mutex_lock(&mutex_bitmap_heap);
	for(i=0; i < TOTAL_FRAME_NUM; i++)
	{	
		if((BITMAP[i])) return i;
	}
	pthread_mutex_unlock(&mutex_bitmap_heap);
	return -1;
}

int frames_needed(int size_total){
	return (size_total/PAGE_SIZE + (size_total%PAGE_SIZE != 0));
}

page* page_with_free_size(){
	int curr_frame_num;
	page* pag = malloc(sizeof(page));

	for(curr_frame_num=0; true; curr_frame_num++)
	{	
		if (curr_frame_num == TOTAL_FRAME_NUM){
			log_info(debug_logger, "Se quiso cargar una pagina pero nos quedamos sin frames upcm libres, aplicando algoritmo de reemplazo");
			curr_frame_num = dame_nro_frame_reemplazado();
		}

		pthread_mutex_lock(&mutex_bitmap_heap);
		if (BITMAP[curr_frame_num])
		{
			BITMAP[curr_frame_num] = 0;
			pag->is_present = 1;
			pag->is_used = 1;
			pag->is_modify = 0;
			pthread_mutex_lock(&mutex_MM);
			pag->fr = MAIN_MEMORY + (curr_frame_num * PAGE_SIZE);
			pthread_mutex_unlock(&mutex_MM);

			log_debug(debug_logger, "La direccion del frame libre es: %d", pag->fr);

			pthread_mutex_unlock(&mutex_bitmap_heap);
			return pag;
		}

		pthread_mutex_unlock(&mutex_bitmap_heap);
	}	
}

t_list* lista_de_segmentos(){
	program* prog;
	int i;
	t_list* lista_de_segmentos = list_create();

	for(i=0; i<list_size(program_list); i++){
		prog = list_get(program_list, i);
		list_add_all(lista_de_segmentos, prog->segment_table);
	}
	return lista_de_segmentos;
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
	pthread_mutex_lock(&mutex_clock);
	t_list *listaSeg = lista_de_segmentos();
	
	int cantidad_de_segmentos_totales = list_size(listaSeg);
	int cantidad_de_paginas_en_segmento;
	int nro_de_segmento;
	int nro_de_pag;
	segment *seg;

	seg = list_get(listaSeg,segmentoClock);
	cantidad_de_paginas_en_segmento = list_size(seg->page_table);

//	if(paginaClock == cantidad_de_paginas_en_segmento){
//		if(segmentoClock == cantidad_de_segmentos_totales){
//			nro_de_segmento = 0;
//			nro_de_pag = 0;
//		}
//		else{
//			nro_de_segmento = segmentoClock + 1;
//			nro_de_pag = 0;
//		}
//	}
//	else{
//		nro_de_segmento = segmentoClock;
//		nro_de_pag = paginaClock + 1;
//	}

	nro_de_segmento = segmentoClock;
	nro_de_pag = paginaClock + 1;

	int nro_frame;
	int nro_paso = 1;
	page *pag;
	int U, M;

	while(true)
	{
		for (nro_de_segmento; nro_de_segmento < cantidad_de_segmentos_totales ; nro_de_segmento++)
		{
			seg = list_get(listaSeg,nro_de_segmento);
			cantidad_de_paginas_en_segmento = list_size(seg->page_table);			
			log_debug(debug_logger, "--- SEGMENTO ANALIZADO ALG REEMPLAZO: %d - Tiene %d paginas", nro_de_segmento, cantidad_de_paginas_en_segmento);
			
			for(nro_de_pag; nro_de_pag < cantidad_de_paginas_en_segmento; nro_de_pag++)
			{
				pag = list_get(seg->page_table,nro_de_pag);
				if (!pag->is_present) continue;
				U = pag->is_used;
				M = pag->is_modify;
				if (nro_paso == 1){
					if (!U && !M){	
						nro_frame = se_hace_la_vistima(pag, nro_de_pag, nro_de_segmento);
						segmentoClock = nro_de_segmento;
						paginaClock = nro_de_pag;
						list_destroy(listaSeg);
						pthread_mutex_unlock(&mutex_clock);
						return nro_frame;											
					}
				}
				if (nro_paso == 2){
					if (!U && M){
						nro_frame = se_hace_la_vistima(pag, nro_de_pag, nro_de_segmento);
						segmentoClock = nro_de_segmento;
						paginaClock = nro_de_pag;
						list_destroy(listaSeg);
						pthread_mutex_unlock(&mutex_clock);
						return nro_frame;	
					}
					pag->is_used = 0;
				}			
			}
			nro_de_pag = 0;
		}
		nro_de_segmento = 0;
		nro_de_pag = 0;

		nro_paso++;
		if (nro_paso == 3)
			nro_paso = 1;
	}
	list_destroy(listaSeg);
}  

//devuelve el nro de frame de la víctima & la manda al archivo swap
int se_hace_la_vistima(page* pag, int nro_de_pag, int nro_de_segmento)
{
	log_info(debug_logger, "Se eligio como víctima a la pagina %d del segmento %d",
							nro_de_pag, nro_de_segmento);
	int nro_frame_upcm;
	int nro_frame_swap;

	//Nosotros habiamos dicho que: pag->fr = MAIN_MEMORY + (curr_frame_num * PAGE_SIZE)
	pthread_mutex_lock(&mutex_MM);
	nro_frame_upcm = (pag->fr - MAIN_MEMORY) / PAGE_SIZE;
	pthread_mutex_unlock(&mutex_MM);
	pthread_mutex_lock(&mutex_bitmap_heap);
	BITMAP[nro_frame_upcm] = 1; //declaro como libre ese nro de frame
	pthread_mutex_unlock(&mutex_bitmap_heap);

 	nro_frame_swap = mandar_al_archivo_swap_toda_la_pagina_que_esta_en(nro_frame_upcm);

	if (nro_frame_swap == -1)
		log_error(debug_logger, "Nos quedamos sin frames libres en el archivo swap");
	else{
		log_info(debug_logger, "La pagina victima se mando satisfactoriamente al archivo swap");

		pag->fr = nro_frame_swap;
		pag->is_present = false;

	}
	return nro_frame_upcm;
}

int mandar_al_archivo_swap_toda_la_pagina_que_esta_en(int nro_frame)
{
	log_debug(debug_logger, "MANDAR A SWAP - El nro de frame recibido es: %d", nro_frame);
	FILE *swap_file;
	void* buffer = malloc(PAGE_SIZE);
	int nro_frame_swap = frame_swap_libre();
	if (nro_frame_swap != -1)
	{
		pthread_mutex_lock(&mutex_MM);
		memcpy(buffer, MAIN_MEMORY + (nro_frame * PAGE_SIZE), PAGE_SIZE);
		pthread_mutex_unlock(&mutex_MM);

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
		pthread_mutex_lock(&mutex_bitmap_mmap);
		if(BITMAP_SWAP_FILE[i])
		{ 
			BITMAP_SWAP_FILE[i] = 0;
			pthread_mutex_unlock(&mutex_bitmap_mmap);
			return i;
		}
		pthread_mutex_unlock(&mutex_bitmap_mmap);
	}
	return -1; // no hay frames libres
}

void metricas_por_socket_conectado(uint32_t pid){
	int nro_prog = search_program(pid);
	program *prog = list_get(program_list, nro_prog);
	t_list *listaSeg = lista_de_segmentos();

	int nro_de_seg;

	int cantidad_de_segmentos_asignados = list_size(prog->segment_table);
	int cantidad_de_segmentos_totales = list_size(listaSeg);

	list_destroy(listaSeg);

	log_info(metricas_logger, "El programa n° %d tiene asignados %d de %d segmentos en el sistema",
			nro_prog, cantidad_de_segmentos_asignados, cantidad_de_segmentos_totales);
	number_of_free_frames();
}

void modificar_metadata(int direccionLogica, segment* segmentoBuscado, int nuevoSize, int is_free){
	log_debug(debug_logger, "Llamado a modificar_metadata con dir log: %d, nuevoSize: %d, isfree: %d",
			direccionLogica, nuevoSize, is_free);
	heap_metadata* metadataBuscada = NULL;

	int paginaBuscada = floor((direccionLogica - segmentoBuscado->base) / PAGE_SIZE);
	int offset = (direccionLogica - segmentoBuscado->base) % PAGE_SIZE;

	page* pagina = list_get(segmentoBuscado->page_table, paginaBuscada);

	if(!pagina->is_present){
		obtener_data_marco_heap(pagina);
		//log_debug(debug_logger, "-- PAGINA NO PRESENTE, LA CARGO--");
	}

	metadataBuscada = (heap_metadata*) ((pagina->fr) + offset);

	if((offset + METADATA_SIZE) > PAGE_SIZE){
		heap_metadata metadataCopia = { 0, true };

		page* proximaPagina = list_get(segmentoBuscado->page_table, (paginaBuscada + 1));

		if(!proximaPagina->is_present){
			obtener_data_marco_heap(proximaPagina);
			log_debug(debug_logger, "-- LLA METADATA ESTABA CORTADA, LA PAGINA SIG NO ESTABA PRESENTE, LA CARGO--");
		}

		void* punteroAlFrameSiguiente = proximaPagina->fr;
		int tamanioMetadataCortada = (PAGE_SIZE) - offset;
		log_debug(debug_logger, "Tamanio metadata cortada parte 1: %d", tamanioMetadataCortada);
		log_debug(debug_logger, "Tamanio metadata cortada parte 2: %d", METADATA_SIZE - tamanioMetadataCortada);

		memcpy(&metadataCopia, metadataBuscada, tamanioMetadataCortada);
		memcpy((void*)(&metadataCopia) + tamanioMetadataCortada, punteroAlFrameSiguiente, METADATA_SIZE - tamanioMetadataCortada);

		metadataCopia.is_free = is_free;
		metadataCopia.size = nuevoSize;

		memcpy(metadataBuscada, &metadataCopia, tamanioMetadataCortada);
		memcpy(punteroAlFrameSiguiente, (void*)(&metadataCopia) + tamanioMetadataCortada, METADATA_SIZE - tamanioMetadataCortada);

		//borra esto, es para logs nomas
		heap_metadata metadataCopia2;
		memcpy(&metadataCopia2, metadataBuscada, tamanioMetadataCortada);
		memcpy((void*)(&metadataCopia2) + tamanioMetadataCortada, punteroAlFrameSiguiente, METADATA_SIZE - tamanioMetadataCortada);
		log_debug(debug_logger, "Lo cargado en metadataBuscada es -> isfree: %d - size: %d",
				metadataCopia2.is_free, metadataCopia2.size);
	}else{
		metadataBuscada->is_free = is_free;
		metadataBuscada->size = nuevoSize;
	}
}

heap_metadata buscar_metadata_por_direccion(int direccionLogica, segment* segmentoBuscado){
	log_debug(debug_logger, "Llamado buscar_metadata_por_direccion");
	log_debug(debug_logger, "buscar_metadata_por_direccion. Dir Logica: %d", direccionLogica);
	heap_metadata* metadataBuscada = NULL;

	//log_debug(debug_logger, "-- PAGINA NO PRESENTE, LA CARGO--");

	int paginaBuscada = floor((direccionLogica - segmentoBuscado->base) / PAGE_SIZE);
	int offset;
	offset = (direccionLogica - segmentoBuscado->base) % PAGE_SIZE;

	log_debug(debug_logger, "La pagina buscada es la %d con offset %d", paginaBuscada, offset);

	page* pagina = list_get(segmentoBuscado->page_table, paginaBuscada);

	if(pagina == NULL){
		log_error(debug_logger, "La pagina buscada no existe");
	}

	if(!pagina->is_present){
		log_debug(debug_logger, "LA PAGINA BUSCADA NO ESTABA PRESENTE - LA CARGO - BUSCAR_METADATA_POR_DIR");
		obtener_data_marco_heap(pagina);
		//log_debug(debug_logger, "-- PAGINA NO PRESENTE, LA CARGO--");
	}

	metadataBuscada = (heap_metadata*) ((pagina->fr) + offset);
	log_debug(debug_logger, "Soy buscar metadata y el numero de frame que recibi para la pag es: %d",
			(pagina->fr - MAIN_MEMORY)/PAGE_SIZE);

	if((offset + METADATA_SIZE) > PAGE_SIZE){

		log_debug(debug_logger, "LA METADATA ESTA CORTADA - BUSCAR_METADATA_POR_DIR");
		heap_metadata metadataCopia;

		int tamanioMetadataCortada = (PAGE_SIZE) - offset;

		memcpy(&metadataCopia, metadataBuscada, tamanioMetadataCortada);

		page* proximaPagina = list_get(segmentoBuscado->page_table, (paginaBuscada + 1));

		if(!proximaPagina->is_present){
			log_debug(debug_logger, "-- ELA METADATA ESTABA CORTADA, LA PAGINA SIG NO ESTABA PRESENTE, LA CARGO--");
			obtener_data_marco_heap(proximaPagina);
		}

		//log_debug(debug_logger, "Soy buscar metadata y el numero de frame que recibi para la pag siguiente es: %d",
		//			(pagina->fr - MAIN_MEMORY)/PAGE_SIZE);
		void* punteroAlFrameSiguiente = proximaPagina->fr;

		//log_debug(debug_logger, "Tamanio metadata cortada parte 1: %d", tamanioMetadataCortada);
		//log_debug(debug_logger, "Tamanio metadata cortada parte 2: %d", METADATA_SIZE - tamanioMetadataCortada);
		//log_debug(debug_logger, "El frame de la pagina que tiene la parte 1 es: %d", (pagina->fr - MAIN_MEMORY)/PAGE_SIZE);
		//log_debug(debug_logger, "El frame de la pagina que tiene la parte 2 es: %d", (proximaPagina->fr - MAIN_MEMORY)/PAGE_SIZE);

		//memcpy(&metadataCopia, metadataBuscada, tamanioMetadataCortada);
		memcpy((void*)(&metadataCopia) + tamanioMetadataCortada, punteroAlFrameSiguiente, METADATA_SIZE - tamanioMetadataCortada);

		//log_debug(debug_logger, "La metadata copia tiene is_free: %d, size: %d", metadataCopia.is_free, metadataCopia.size);

		//log_warning(debug_logger, "Fin buscar_metadata_por_direccion");
		return metadataCopia;
	}else{
		log_debug(debug_logger, "Fin buscar_metadata_por_direccion");
		return *metadataBuscada;
	}
}

int ultima_metadata_segmento(int dirLogica, segment* segmentoActual){
	pthread_mutex_lock(&mutex_carga_pag);
	log_debug(debug_logger, "Func. Ult. Met. Seg. : Dir logica pedida: %d", dirLogica);

	heap_metadata metadataActual = buscar_metadata_por_direccion(dirLogica, segmentoActual);
	int ultima_metadata;

	int dirLogicaSiguienteMetadata;
	dirLogicaSiguienteMetadata = metadataActual.size + METADATA_SIZE + dirLogica;
	log_debug(debug_logger, "La direccion log de la metActual es: %d \nEl size de la metActual: %d",dirLogica, metadataActual.size);
	log_debug(debug_logger, "El limite del segmento es: ", segmentoActual->limit);
	if(dirLogicaSiguienteMetadata == segmentoActual->limit){
		pthread_mutex_unlock(&mutex_carga_pag);
		return dirLogica;
	}
	else{
		pthread_mutex_unlock(&mutex_carga_pag);
		ultima_metadata = ultima_metadata_segmento(dirLogicaSiguienteMetadata, segmentoActual);
		return ultima_metadata;
	}
}

int proxima_metadata_libre_con_size(int dirLogica, segment* segmentoActual, int size){
	pthread_mutex_lock(&mutex_prox_lib);
	heap_metadata metadataActual;
	metadataActual = buscar_metadata_por_direccion(dirLogica, segmentoActual);

	int direccionUltimaMetadata = ultima_metadata_segmento(segmentoActual->base, segmentoActual);

	if(dirLogica == direccionUltimaMetadata){
		if(metadataActual.size >=size){
			pthread_mutex_unlock(&mutex_prox_lib);
			return dirLogica;
		}
		else{
			pthread_mutex_unlock(&mutex_prox_lib);
			return -1;
		}
	}

	int dirLogicaSiguienteMetadata;
	dirLogicaSiguienteMetadata = metadataActual.size + METADATA_SIZE + dirLogica;

	log_debug(debug_logger, "La dir de la ultima metadata es: %d y de la siguiente a la actual es: %d", direccionUltimaMetadata,
			dirLogicaSiguienteMetadata);

	if(direccionUltimaMetadata == dirLogicaSiguienteMetadata){
		heap_metadata ultimaMetadata;
		ultimaMetadata = buscar_metadata_por_direccion(dirLogicaSiguienteMetadata, segmentoActual);
		if(ultimaMetadata.is_free && ultimaMetadata.size >= size){
			log_debug(debug_logger, "Retorne la siguiente dir logica %d", dirLogicaSiguienteMetadata);
			pthread_mutex_unlock(&mutex_prox_lib);
			return dirLogicaSiguienteMetadata;
		}else{
			log_debug(debug_logger, "ultimaMetadata libre: %d - ultMetadata size: %d - Size Requerido %d"
					,ultimaMetadata.is_free, ultimaMetadata.size, size);
			pthread_mutex_unlock(&mutex_prox_lib);
			return -1;
		}
	}

	if(metadataActual.is_free && metadataActual.size >= size){
		pthread_mutex_unlock(&mutex_prox_lib);
		return dirLogica;
	}else{
		pthread_mutex_unlock(&mutex_prox_lib);
		return proxima_metadata_libre_con_size(dirLogicaSiguienteMetadata, segmentoActual, size);
	}

}

void listar_metadatas(int dirLogica, segment* segmento){
	heap_metadata metadataActual;
	metadataActual = buscar_metadata_por_direccion(dirLogica, segmento);

	log_debug(debug_logger, "Limite segmento: %d", segmento->limit);

	int direccionUltimaMetadata = ultima_metadata_segmento(segmento->base, segmento);

	if(dirLogica == direccionUltimaMetadata){
		log_debug(debug_logger, "Ultima metadata --> Size: %d, is_free: %d, direccionLogica: %d", metadataActual.size, metadataActual.is_free, dirLogica);

	}else{
		int dirLogicaSiguienteMetadata = metadataActual.size + METADATA_SIZE + dirLogica;

		if(direccionUltimaMetadata == dirLogicaSiguienteMetadata){
			heap_metadata ultimaMetadata;
			ultimaMetadata = buscar_metadata_por_direccion(dirLogicaSiguienteMetadata, segmento);
			log_debug(debug_logger, "Metadata --> Size: %d, is_free: %d, direccionLogica: %d", metadataActual.size, metadataActual.is_free, dirLogica);
			log_debug(debug_logger, "Ultima metadata --> Size: %d, is_free: %d, direccionLogica: %d", ultimaMetadata.size, ultimaMetadata.is_free, dirLogicaSiguienteMetadata);
		}else{
			log_debug(debug_logger, "Metadata --> Size: %d, is_free: %d, direccionLogica: %d", metadataActual.size, metadataActual.is_free, dirLogica);

			listar_metadatas(dirLogicaSiguienteMetadata, segmento);
		}
	}
}

int proxima_metadata_libre(int dirLogica, segment* segmentoActual){
	heap_metadata metadataActual;
	metadataActual = buscar_metadata_por_direccion(dirLogica, segmentoActual);	

	int direccionUltimaMetadata = ultima_metadata_segmento(segmentoActual->base, segmentoActual);

	int dirLogicaSiguienteMetadata = metadataActual.size + METADATA_SIZE + dirLogica;

	if(direccionUltimaMetadata == dirLogicaSiguienteMetadata){
		heap_metadata ultimaMetadata;
		ultimaMetadata = buscar_metadata_por_direccion(dirLogicaSiguienteMetadata, segmentoActual);
		if(ultimaMetadata.is_free){
			return dirLogica;
		}else{
			return -1;
		}
	}

	if(metadataActual.is_free){
		return dirLogica;
	}else{
		return proxima_metadata_libre(dirLogicaSiguienteMetadata, segmentoActual);
	}

}

bool sePuedeAgrandar(segment* segmentoAAgrandar, program* prog){
	int tamanio_segmento = segmentoAAgrandar->limit - segmentoAAgrandar->base;
	int direccionLogicaProximoSegmento = segmentoAAgrandar->base + tamanio_segmento + 1;

	int nro_segmento_proximo = busca_segmento(prog, direccionLogicaProximoSegmento);

	return nro_segmento_proximo == -1;
}

uint32_t memory_malloc(int size, uint32_t pid)
{
	log_debug(debug_logger, "Ejecuto memory_malloc");
	if (size <= 0) return 0;
	if (size + METADATA_SIZE > number_of_free_frames() * PAGE_SIZE){
		log_error(debug_logger, "Memoria llena: no se puede allocar tanto espacio en memoria");
		log_error(debug_logger, "Segmentation Fault");
		return -1;		
	} 

	int nro_prog;
	uint32_t nro_seg;
	uint32_t nro_pag;
	uint32_t offset = 0;
	int direccionLogicaMetadataLibre;
	page *pag;
	program *prog;
	heap_metadata *metadata = NULL; 
	int total_size = size + METADATA_SIZE;
	int total_pages_needed = (total_size / PAGE_SIZE) + ((total_size % PAGE_SIZE) != 0); // ceil(total_size / PAGE_SIZE)
	int espacio_que_queda_alocar = total_size;
	int espacio_q_quedara_libre = total_pages_needed * PAGE_SIZE - total_size;
	int is_first_page = 1;
	
	log_info(debug_logger, "Se pidieron %d bytes, + metadata = %d, PID: %d", size, total_size, pid);
	
	//si el programa no está en la lista de programas, se agrega y le creamos una nueva tabla de segmentos
	if((nro_prog = search_program(pid)) == -1)
	{
		prog = (program *) malloc(sizeof(program));
		memset(prog, 0, sizeof(program));
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
		heap_metadata metadataBuscada;

		nro_seg = segment_with_free_space(prog, total_size);
		segmentoConEspacio = list_get(prog->segment_table, nro_seg);

		if(segmentoConEspacio == NULL){
			log_error(debug_logger, "Segmento nulo - ERROR -");
			return -1;
		}

		if(!segmentoConEspacio->is_heap){
			int pidEncontrada;

			int igualPID(int pid){
			    return pid == prog->pid;
			}

			pidEncontrada = (int)list_find(segmentoConEspacio->archivo_mapeado->programas,(void*) igualPID);
			if((void*)pidEncontrada == NULL){
				log_error(debug_logger, "El archivo mmap es privado y el programa no tiene permisos");
				return -1;
			}
		}

		direccionLogicaMetadataLibre = proxima_metadata_libre_con_size(segmentoConEspacio->base, segmentoConEspacio, total_size);

		log_debug(debug_logger, "La direccion logica de la metadata donde voy a allocar esta en ---> %d", direccionLogicaMetadataLibre);

		metadataBuscada = buscar_metadata_por_direccion(direccionLogicaMetadataLibre, segmentoConEspacio);

		int paginaDeLaMetadataLibre = floor((direccionLogicaMetadataLibre - segmentoConEspacio->base) / PAGE_SIZE);

		//Alloco aca

		int sizeAnterior = metadataBuscada.size;

		modificar_metadata(direccionLogicaMetadataLibre, segmentoConEspacio, size, 0);

		metadataBuscada = buscar_metadata_por_direccion(direccionLogicaMetadataLibre, segmentoConEspacio);

		log_debug(debug_logger, "Nuevo size de la metadata ---> %d", metadataBuscada.size);

		int direccionLogicaMetadataFinal = metadataBuscada.size + METADATA_SIZE + direccionLogicaMetadataLibre;

		log_debug(debug_logger, "Direccion logica metadata final ---> %d", direccionLogicaMetadataFinal);

		offset = (direccionLogicaMetadataFinal - segmentoConEspacio->base) % PAGE_SIZE;

		heap_metadata metadataFinal;
		metadataFinal = buscar_metadata_por_direccion(direccionLogicaMetadataFinal, segmentoConEspacio);

		modificar_metadata(direccionLogicaMetadataFinal, segmentoConEspacio, (sizeAnterior - total_size), 1);
		log_debug(debug_logger, "La metadata final tiene is free: %d - size: %d", metadataFinal.is_free, metadataFinal.size);

		int direccionLogicaUltimaMetadata = ultima_metadata_segmento(segmentoConEspacio->base, segmentoConEspacio);

		heap_metadata ultimaMetadata = buscar_metadata_por_direccion(direccionLogicaUltimaMetadata, segmentoConEspacio);

		log_debug(debug_logger, "Valores metadata free: %d, size: %d", ultimaMetadata.is_free, ultimaMetadata.size);

	}
	//Si su ultimo segmento es heap y tiene memoria están los frames que necesitamos
	else if (list_size(prog->segment_table)>0 && ultimo_segmento_programa(prog)->is_heap)
	{
		log_info(debug_logger, "El segmento no tenia suficiente espacio libre, intento agrandar");
		segment *segmentoAAgrandar;
		segmentoAAgrandar = ultimo_segmento_programa(prog);

		if(sePuedeAgrandar(segmentoAAgrandar, prog)){

		log_debug(debug_logger, "Se puede agrandar");
		heap_metadata ultimaMetadata;

		int direccionLogicaUltimaMetadata = ultima_metadata_segmento(segmentoAAgrandar->base, segmentoAAgrandar);
		log_debug(debug_logger, "Soy malloc y encontre dir ultima metadata: %d", direccionLogicaUltimaMetadata);

		ultimaMetadata = buscar_metadata_por_direccion(direccionLogicaUltimaMetadata, segmentoAAgrandar);
		log_debug(debug_logger, "Soy malloc y los datos de ult metadata son isfree: %d - size: %d",
				ultimaMetadata.is_free, ultimaMetadata.size);

		int sizeAnterior = ultimaMetadata.size;
		float cantidadDePaginasAAgrandar = ceil(((float)(total_size - sizeAnterior))/PAGE_SIZE);
		log_debug(debug_logger, "Soy malloc y la cant de paginas a agrandar es: %d", cantidadDePaginasAAgrandar);

		modificar_metadata(direccionLogicaUltimaMetadata, segmentoAAgrandar, cantidadDePaginasAAgrandar * PAGE_SIZE + sizeAnterior, 1);
		ultimaMetadata = buscar_metadata_por_direccion(direccionLogicaUltimaMetadata, segmentoAAgrandar);
		log_debug(debug_logger, "Soy malloc y la data de la nueva ult metadata es isfree: %d - size: %d",
				ultimaMetadata.is_free, ultimaMetadata.size);

		for(int i=0 ; i < cantidadDePaginasAAgrandar ; i++ )
		{
			log_debug(debug_logger, "Soy malloc y antes page_with_free_size()");
			pag = page_with_free_size();
			log_debug(debug_logger, "Soy malloc y despues page_with_free_size()");
			log_debug(debug_logger, "El frame de la pagina a cargar que obtengo de page_w_f_s es: %d", (pag->fr - MAIN_MEMORY) / PAGE_SIZE);
			pag->is_used = 1;
			list_add(segmentoAAgrandar->page_table, pag);
			segmentoAAgrandar->limit += PAGE_SIZE;
		}
		log_debug(debug_logger, "La dir de la ultima metadata es: %d - Su nuevo espacio libre es: %d", direccionLogicaUltimaMetadata, ultimaMetadata.size);

		log_info(debug_logger, "Se pudo agrandar el segmento con nuevo limite %d, procedo a allocar", segmentoAAgrandar->limit);

		heap_metadata metadata = buscar_metadata_por_direccion(direccionLogicaUltimaMetadata, segmentoAAgrandar);

		log_debug(debug_logger, "Valores metadata free: %d, size: %d", metadata.is_free, metadata.size);

		return memory_malloc(size, pid);
		}else{
			log_info(debug_logger, "No pude agrandar ningun segmento asi que le creo uno nuevo");

			int tamanioNecesario = size + METADATA_SIZE*2;
			int paginasNecesarias = (int)ceil((double)tamanioNecesario / (double)PAGE_SIZE);

			int cantidadDePaginasAAgregar = paginasNecesarias;
			segment *segmentoNuevo = NULL;
			segmentoNuevo = (segment *) malloc(sizeof(segment));
			segmentoNuevo->is_heap = true;
			segmentoNuevo->page_table = list_create();

			int numeroDeSegmentos = list_size(prog->segment_table);

			if(numeroDeSegmentos > 0){
				segment* ultimoSegmento = list_get(prog->segment_table, numeroDeSegmentos - 1);
				segmentoNuevo->base = ultimoSegmento->limit + 1;
			}else{
				segmentoNuevo->base = 0;
			}

			segmentoNuevo->limit = 0;
			list_add(prog->segment_table, segmentoNuevo);
			log_debug(debug_logger, "Se creo un segmento");

			for(int i=0; i < cantidadDePaginasAAgregar; i++ )
			{
				pag = page_with_free_size();
				pag->is_used = 1;
				list_add(segmentoNuevo->page_table, pag);
			}

			segmentoNuevo->limit = (paginasNecesarias * PAGE_SIZE) + segmentoNuevo->base;

			page* primeraPagina = list_get(segmentoNuevo->page_table, 0);

			log_info(debug_logger, "El limite del nuevo segmento es: %d", segmentoNuevo->limit);

			modificar_metadata(0, segmentoNuevo, (cantidadDePaginasAAgregar * PAGE_SIZE) - METADATA_SIZE, 1);

			heap_metadata metadata = buscar_metadata_por_direccion(0, segmentoNuevo);

			log_debug(debug_logger, "Valores metadata free: %d, size: %d", metadata.is_free, metadata.size);

			return memory_malloc(size, pid);
		}
	}
	//en ultimo caso: si no hay espacio ni se puede agrandar ningún segmento, le creo uno
	else 	
	{
		log_info(debug_logger, "No tiene ningun segmento asi que le creo uno");

		int tamanioNecesario = size + METADATA_SIZE*2;
		int paginasNecesarias = (int)ceil((double)tamanioNecesario / (double)PAGE_SIZE);

		int cantidadDePaginasAAgregar = paginasNecesarias;
		segment *segmentoNuevo = NULL;
		segmentoNuevo = (segment *) malloc(sizeof(segment));
		segmentoNuevo->is_heap = true;
		segmentoNuevo->page_table = list_create();
		segmentoNuevo->base = 0;
		segmentoNuevo->limit = 0;
		list_add(prog->segment_table, segmentoNuevo);			
		log_debug(debug_logger, "Se creo un segmento");

		for(int i=0; i < cantidadDePaginasAAgregar; i++ )
		{
			pag = page_with_free_size();
			pag->is_used = 1;
			list_add(segmentoNuevo->page_table, pag);
			segmentoNuevo->limit += PAGE_SIZE;
		}

		segmentoNuevo->limit += segmentoNuevo->base;

		page* primeraPagina = list_get(segmentoNuevo->page_table, 0);

		log_info(debug_logger, "El limite del nuevo segmento es: %d", segmentoNuevo->limit);

		modificar_metadata(0, segmentoNuevo, (cantidadDePaginasAAgregar * PAGE_SIZE) - METADATA_SIZE, 1);

		heap_metadata metadata = buscar_metadata_por_direccion(0, segmentoNuevo);

		log_debug(debug_logger, "Valores metadata free: %d, size: %d", metadata.is_free, metadata.size);

		return memory_malloc(size, pid);
	}	
	
	int direccionLogicaFinal = direccionLogicaMetadataLibre + METADATA_SIZE;
	log_debug(debug_logger, "Direccion logica final del MALLOC ----> %d", direccionLogicaFinal); 

	metricas_por_socket_conectado(pid);

	prog->using_memory += size;

	return direccionLogicaFinal;
}

int segment_with_free_space(program *prog, int size)
{
	pthread_mutex_lock(&mutex_segment);
	int i=0;
	segment *segmentoActual;
	int cantidadDeSegmentos = list_size(prog->segment_table);

	if (cantidadDeSegmentos == 0){
		pthread_mutex_unlock(&mutex_segment);
		return -1;
	}

	while(i < cantidadDeSegmentos)
	{		
		segmentoActual = list_get(prog->segment_table, i);

		if (segmentoActual->is_heap)
		{
			int direccionLogicaEncontrada = proxima_metadata_libre_con_size(segmentoActual->base, segmentoActual, size);
			if(direccionLogicaEncontrada == -1)
				log_debug(debug_logger, "Retorne -1");

			if(direccionLogicaEncontrada != -1){
				heap_metadata metadataEncontrada;
				metadataEncontrada = buscar_metadata_por_direccion(direccionLogicaEncontrada, segmentoActual);

				log_debug(debug_logger, "Encontramos que el segmento %d tiene una metadata de %d de size", i, metadataEncontrada.size);
				pthread_mutex_unlock(&mutex_segment);
				return i;
			}
		}
		i++;
	}
	log_debug(debug_logger, "No habia ningun segmento con %d de espacio libre", size);
	pthread_mutex_unlock(&mutex_segment);
	return -1;
}

void compactar_en_segmento(int direccionLogicaMetadataLibreInicial, int direccionLogicaSiguiente, segment* segmento){

	heap_metadata metadataLibreInicial;
	metadataLibreInicial = buscar_metadata_por_direccion(direccionLogicaMetadataLibreInicial, segmento);
	log_debug(debug_logger, "Metadata inicial de %d de size e is_free = %d", metadataLibreInicial.size, metadataLibreInicial.is_free);

	int direccionUltimaMetadata = ultima_metadata_segmento(segmento->base, segmento);

	//Si no tiene siguiente (la siguiente) entonces quiere decir que es la ultima
	if(direccionLogicaSiguiente == direccionUltimaMetadata)
	{
		heap_metadata metadataSiguiente;
		metadataSiguiente = buscar_metadata_por_direccion(direccionLogicaSiguiente, segmento);
		log_debug(debug_logger, "Metadata siguiente de %d de size e is_free = %d", metadataSiguiente.size, metadataSiguiente.is_free);
		log_debug(debug_logger, "Esta es la ultima");

		//Si esta libre la sumamos a la inicial y si no termino el algoritmo
		if(metadataSiguiente.is_free){

			int is_free = metadataLibreInicial.is_free;
			modificar_metadata(direccionLogicaMetadataLibreInicial, segmento, metadataLibreInicial.size + metadataSiguiente.size + METADATA_SIZE, is_free);

			log_debug(debug_logger, "Metadata libre ahora tiene %d de size e is_free = %d", metadataLibreInicial.size, metadataLibreInicial.is_free);
			int viejoSize = metadataSiguiente.size;
			modificar_metadata(direccionLogicaSiguiente, segmento, viejoSize, 1);
		}
	}
	else
	{
		//Si tiene siguiente entonces podemos seguir mas todavia
		heap_metadata metadataSiguiente;
		metadataSiguiente = buscar_metadata_por_direccion(direccionLogicaSiguiente, segmento);
		log_debug(debug_logger, "Metadata siguiente de %d de size e is_free = %d", metadataSiguiente.size, metadataSiguiente.is_free);

		//Si esta libre la sumamos a la inicial y repetimos el proceso
		if(metadataSiguiente.is_free){

			log_debug(debug_logger, "Esta libre asi que la compacto con la inicial");

			int is_free = metadataLibreInicial.is_free;
			modificar_metadata(direccionLogicaMetadataLibreInicial, segmento, metadataLibreInicial.size + metadataSiguiente.size + METADATA_SIZE, is_free);

			log_debug(debug_logger, "Metadata libre ahora tiene %d de size e is_free = %d", metadataLibreInicial.size, metadataLibreInicial.is_free);
			int viejoSize = metadataSiguiente.size;
			modificar_metadata(direccionLogicaSiguiente, segmento, viejoSize, 1);

			int direccionSiguienteSiguiente = direccionLogicaSiguiente + metadataSiguiente.size + METADATA_SIZE;

			compactar_en_segmento(direccionLogicaMetadataLibreInicial, direccionSiguienteSiguiente, segmento);

		//Si no entonces hay que buscar una nueva libre inicial en donde pararse para repetir el proceso
		}else{
			log_debug(debug_logger, "No esta libre asi que me fijo la proxima libre");
			direccionLogicaMetadataLibreInicial = proxima_metadata_libre(direccionLogicaSiguiente, segmento);

			if(direccionLogicaMetadataLibreInicial != -1){
				//Si tiene siguiente la proxima libre entonces podemos seguir, si no no hay que compactar ya
				if(direccionUltimaMetadata != direccionLogicaMetadataLibreInicial){
					metadataLibreInicial = buscar_metadata_por_direccion(direccionLogicaMetadataLibreInicial, segmento);
					log_debug(debug_logger, "Metadata nueva libre inicial de %d de size e is_free = %d", metadataLibreInicial.size, metadataLibreInicial.is_free);

					int direccionLogicaSiguiente = direccionLogicaMetadataLibreInicial + metadataLibreInicial.size + METADATA_SIZE;

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
	heap_metadata metadataLibreInicial;


	for(i=0; i < cantidad_de_segmentos; i++){
		segment* segmentoActual = list_get(prog->segment_table, i);
		int direccionUltimaMetadata = ultima_metadata_segmento(segmentoActual->base, segmentoActual);

		int direccionLogicaMetadataLibre = proxima_metadata_libre(segmentoActual->base, segmentoActual);

		if(direccionLogicaMetadataLibre != -1){
			metadataLibreInicial = buscar_metadata_por_direccion(direccionLogicaMetadataLibre, segmentoActual);

			if(direccionLogicaMetadataLibre != direccionUltimaMetadata)
			{
				int dirLogicaSiguienteMetadata = metadataLibreInicial.size + METADATA_SIZE + direccionLogicaMetadataLibre;

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

	log_info(debug_logger, "Direccion virtual pedida: %d, PID: %d", virtual_address, pid);

	prog = list_get(program_list, nro_prog);
	int nro_seg = busca_segmento(prog,virtual_address);	
	seg = list_get(prog->segment_table,nro_seg);

//	int base = seg->base;
//	int numero_pagina = (virtual_address - base) / PAGE_SIZE;
//	pag = list_get(seg->page_table,numero_pagina);
//	int offset = (virtual_address - base) - (numero_pagina * PAGE_SIZE);
//	printf("%d representa el seg %d pag %d offset %d \n",virtual_address,nro_seg,numero_pagina,offset);

	int direccionLogicaMetadata = virtual_address - METADATA_SIZE;

	heap_metadata metadata;
	metadata = buscar_metadata_por_direccion(direccionLogicaMetadata, seg);
	int viejoSize = metadata.size;
	modificar_metadata(direccionLogicaMetadata, seg, viejoSize, 1);

	compactar_espacios_libres(prog);

	prog->using_memory -= viejoSize;

	listar_metadatas(seg->base, seg);

	return 0;
}

segment* ultimo_segmento_programa(program *prog){
	segment* seg;
	int ultimo_elemento = list_size(prog->segment_table)-1;
	seg = list_get(prog->segment_table, ultimo_elemento );
	return seg;
}

int busca_segmento(program* prog, uint32_t direccion){
	segment* segmento_obtenido;

	for(int i=0;i<list_size(prog->segment_table);i++){
		segmento_obtenido = list_get(prog->segment_table, i);
		if((segmento_obtenido->base <= direccion) && (segmento_obtenido->limit >= direccion)){
			return i;
		}
	}
	return -1;
}

void* obtener_data_marco_heap(page* pagina){
    if(!pagina->is_present && number_of_free_frames_upcm() == 0){
    	//TODO: mutex archivo swap ? ADEMAS BLOQUEAR QUE NO ESCRIBA CUANDO LIBERA EL FRAME SWAP HASTA QUE MEMCPY
    	int numFrame = dame_nro_frame_reemplazado();

    	pthread_mutex_lock(&mutex_bitmap_heap);
    	BITMAP[numFrame] = 0;
    	pthread_mutex_unlock(&mutex_bitmap_heap);


    	liberar_frame_swap(pagina->fr);
    	log_debug(debug_logger, "Paso pagina de SWAP a MEMORIA PRINCIPAL -> libero frame de swap. Num frame upcm: %d", numFrame);

        void* buffer_page_swap = malloc(PAGE_SIZE);

        FILE* archivo_swap = fopen(SWAP_PATH,"r+");

        fseek(archivo_swap,(int)pagina->fr * PAGE_SIZE,SEEK_SET);
        fread(buffer_page_swap,PAGE_SIZE,1,archivo_swap);

        pthread_mutex_lock(&mutex_MM);
        log_debug(debug_logger, "Estoy en obtener data y el frame de upcm que me dieron es: %d", numFrame);
    	pagina->fr = (void*) (MAIN_MEMORY + numFrame * PAGE_SIZE);
    	pthread_mutex_unlock(&mutex_MM);
        pagina->is_present = 1;
        pagina->is_used = 1;

        memcpy(pagina->fr,buffer_page_swap,PAGE_SIZE);

        fclose(archivo_swap);
        free(buffer_page_swap);
    }
    else if(!pagina->is_present && number_of_free_frames_upcm() > 0){
        void* buffer_page_swap = malloc(PAGE_SIZE);

        FILE* archivo_swap = fopen(SWAP_PATH,"r+");

        fseek(archivo_swap, (int)pagina->fr * PAGE_SIZE,SEEK_SET);
        fread(buffer_page_swap,PAGE_SIZE,1,archivo_swap);

        page* sacarFrame = page_with_free_size();
        liberar_frame_swap(pagina->fr);
        log_debug(debug_logger, "Paso pagina de SWAP a MEMORIA PRINCIPAL -> libero frame de swap");

        pthread_mutex_lock(&mutex_bitmap_heap);
        BITMAP[(sacarFrame->fr - MAIN_MEMORY)/PAGE_SIZE] = 0;
        pthread_mutex_unlock(&mutex_bitmap_heap);


        pagina->fr = sacarFrame->fr;
        pagina->is_present = 1;
        pagina->is_used = 1;

        memcpy(pagina->fr,buffer_page_swap,PAGE_SIZE);

        free(buffer_page_swap);
        fclose(archivo_swap);
    }

	log_debug(debug_logger, "Estoy en obtener data y repito el frame de upcm que me dieron es: %d",
			(pagina->fr - MAIN_MEMORY) / PAGE_SIZE);
    return pagina->fr;
}

void* obtener_data_marco_mmap(segment* segmento,page* pagina,int nro_pagina){
    if(!pagina->is_present && number_of_free_frames_upcm() == 0){
    	void* buffer_page_mmap = malloc(PAGE_SIZE);

    	int numFrame = dame_nro_frame_reemplazado();

    	pthread_mutex_lock(&mutex_bitmap_mmap);
    	BITMAP[numFrame] = 0;
    	pthread_mutex_unlock(&mutex_bitmap_mmap);

    	log_debug(debug_logger, "Paso pagina de ARCHIVO MAPPED a MEMORIA PRINCIPAL");
    	log_debug(debug_logger, "NUM FRAME: %d",numFrame);
    	pthread_mutex_lock(&mutex_MM);
    	pagina->fr = (void*) (MAIN_MEMORY + numFrame * PAGE_SIZE);
    	pthread_mutex_unlock(&mutex_MM);
        pagina->is_present = 1;
        pagina->is_used = 1;

        if((nro_pagina * PAGE_SIZE) <= segmento->limit){
            int bytes_a_leer = (int)fmin(PAGE_SIZE,((nro_pagina * PAGE_SIZE) + PAGE_SIZE) - segmento->limit);

            memcpy(pagina->fr, segmento->archivo_mapeado->archivo + nro_pagina * PAGE_SIZE, bytes_a_leer);
            if(PAGE_SIZE > bytes_a_leer)
                memset(pagina->fr,'\0',PAGE_SIZE - bytes_a_leer);
        }
        else{
            // el primer byte a leer supera el tamano del archivo
            memset(pagina->fr,'\0',PAGE_SIZE);
        }

        free(buffer_page_mmap);
    }
    else if(!pagina->is_present && number_of_free_frames_upcm() > 0){
        void* buffer_page_mmap = malloc(PAGE_SIZE);

        page* sacarFrame = page_with_free_size();

        pthread_mutex_lock(&mutex_bitmap_heap);
        BITMAP[(sacarFrame->fr - MAIN_MEMORY)/PAGE_SIZE] = 0;
        pthread_mutex_unlock(&mutex_bitmap_heap);

        log_debug(debug_logger, "Paso pagina de MAPPED a MEMORIA PRINCIPAL");
        log_debug(debug_logger, "La dir del frame es: %d", *(int *)sacarFrame->fr);
        log_debug(debug_logger, "Num frame: %d", (sacarFrame->fr - MAIN_MEMORY) /  PAGE_SIZE);
        pagina->fr = sacarFrame->fr;
        pagina->is_present = 1;
        pagina->is_used = 1;


        free(sacarFrame);

        log_debug(debug_logger, "Nro pagina: %d", nro_pagina);
        log_debug(debug_logger, "dir frame de la pagina: %d", *(int *)pagina->fr);
        log_debug(debug_logger, "Tamanio archivo mappeado: %d, path: %s", segmento->tam_archivo_mmap,segmento->archivo_mapeado->pathArchivo);

        memcpy(pagina->fr, segmento->archivo_mapeado->archivo + (nro_pagina * PAGE_SIZE), PAGE_SIZE);

        free(buffer_page_mmap);
    }

    return pagina->fr;
}

//Copia n bytes de LIBMUSE a MUSE
uint32_t memory_cpy(uint32_t dst, void *src, int n, uint32_t pid)
{
	program* prog;
	page* paginaObtenida;
	int numProg = search_program(pid);

	log_info(debug_logger, "Direccion destino: %d, Bytes a copiar: %d, PID: %d", dst, n, pid);

	if((numProg = search_program(pid)) == -1)
	{
		prog = (program *) malloc(sizeof(prog));
		prog->pid = pid;
		prog->segment_table = list_create();
		numProg = list_add(program_list, prog);
		log_debug(debug_logger, "Se creo el prog n°%d de la lista de programas ", numProg);
	}

	prog = list_get(program_list, numProg);
	log_debug(debug_logger, "El programa del pedido es %d", numProg);

	int numSeg = busca_segmento(prog, dst);
	segment* segment = list_get(prog->segment_table, numSeg);

	if(segment == NULL){
		log_error(debug_logger, "Se busco el segmento %d, pero no se encontro", numSeg);
		return -3;
	}else{
		log_debug(debug_logger, "Limite Seg: %d , Base seg: %d", segment->limit, segment->base);
		log_debug(debug_logger, "El segmento a escribir es %d", numSeg);
	}

	if(!segment->is_heap){
		int pidEncontrada;

		if(segment->archivo_mapeado->pathArchivo == NULL){
			log_error(debug_logger, "Se trata de acceder a una direccion unmapped");
			return -1;
		}

		int igualPID(int pid){
		    return pid == prog->pid;
		}

		pidEncontrada = (int)list_find(segment->archivo_mapeado->programas,(void*) igualPID);
		if((void*)pidEncontrada == NULL){
			log_error(debug_logger, "El archivo mmap es privado y el programa no tiene permisos");
			return -2;
		}
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

            // vuelvo a cargar los datos al upcm
            for(int x=0; x<cantidad_paginas_necesarias;x++){
            	paginaObtenida = list_get(segment->page_table,x + numPage);
            	paginaObtenida->is_modify = 1; //esto lo agregue 16/12
                datos = obtener_data_marco_heap(paginaObtenida);
                memcpy(datos,buffer + PAGE_SIZE * x, PAGE_SIZE);
            }
        }
        else{
        	log_error(debug_logger, "Posicion invalida, no se pudo realizar la copia");
            return -1;
        }
	}else{
		log_debug(debug_logger, "Offset: %d",offset);
		log_debug(debug_logger, "Cantidad de paginas necesarias: %d",cantidad_paginas_necesarias);
		
		if((offset + (cantidad_paginas_necesarias * PAGE_SIZE)) >= n){

			memcpy(buffer + offset,src,n);
		    // vuelvo a cargar los datos al upcm
			for(int x=0; x<cantidad_paginas_necesarias;x++){
				paginaObtenida = list_get(segment->page_table,x + numPage);
				paginaObtenida->is_modify = 1; //esto lo agregue 16/12
				datos = obtener_data_marco_mmap(segment,paginaObtenida,x + numPage);
				memcpy(datos,buffer + PAGE_SIZE * x,PAGE_SIZE);
			}
		}else{
		 	// no puedo almacenar los datos pq ingreso a una posicion invalida
			log_error(debug_logger, "Posicion invalida, no se pudo realizar la copia");
			free(buffer);
			return -1;
		 }
	}

	log_debug(debug_logger, "Limite Seg: %d , Base seg: %d", segment->limit, segment->base);

	free(buffer);
	
	return dst;
}

archivoMMAP* buscar_archivo_mmap(char* path){

	bool igualArchivo(archivoMMAP* archivo) {
		return !strcmp(archivo->pathArchivo, path);
	}
	return list_find(lista_archivos_mmap,(void*) igualArchivo);
}

int crear_nuevo_segmento_mmap(size_t length, program* prog){
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

	total_pages_needed = ceil(length / PAGE_SIZE);

	seg->limit = (total_pages_needed * PAGE_SIZE) + seg->base;

	log_debug(debug_logger, "Limite del segmento nuevo ----> %d", seg->limit);
	list_add(prog->segment_table, seg);
	log_debug(debug_logger, "Ahora el programa tiene ----> %d segmentos", list_size(prog->segment_table));

	return seg->base;
}

// Apalancándonos en el mismo mecanismo que permite el swapping de páginas,
// la funcionalidad de memoria compartida que proveerá MUSE (a través de sus 
// funciones de muse_map) se realizará sobre un archivo compartido, en vez 
// del archivo de swap. Esta distinción deberá estar plasmada en la tabla de segmentos.
uint32_t memory_map(char *path, size_t length, int flag, uint32_t pid)
{
	int nro_prog;
	program* prog = NULL;
	segment* segmentoNuevo = NULL;
	int direccionDelSegmento;
	archivoMMAP* archivoMappeado;

	if((nro_prog = search_program(pid)) == -1)
	{
		log_debug(debug_logger, "No lo encontre, lo creo ----> %d", pid);
		prog = (program *) malloc(sizeof(program));
		memset(prog, 0, sizeof(program));
		prog->pid = pid;
		prog->segment_table = list_create();
		nro_prog = list_add(program_list, prog);
		log_debug(debug_logger, "Se creo el prog n°%d de la lista de programas ", nro_prog);
	}else{
		int nro_prog = search_program(pid);
		prog = list_get(program_list, nro_prog);
	}

	log_info(debug_logger, "Path recibido: %s, PID: %d, Length: %d", path, pid, length);

	archivoMappeado = buscar_archivo_mmap(path);

	if(archivoMappeado == NULL){
		archivoMMAP* archivoMappeadoNuevo = (archivoMMAP*)malloc(sizeof(archivoMMAP));
		int fileDescriptor = open(path, O_RDWR, 0);

		if(fileDescriptor == -1){
			return -1;
		}

		off_t fsize;

		fsize = lseek(fileDescriptor, 0, SEEK_END);

		if(fsize < length){
			truncate(path, length);
		}

		void* mmapArchivo = mmap(NULL, length, PROT_READ|PROT_WRITE, flag, fileDescriptor,0);

		int sizePath = strlen(path) + 1;
		archivoMappeadoNuevo->pathArchivo = malloc(sizePath);
		memcpy(archivoMappeadoNuevo->pathArchivo, path, sizePath);

		archivoMappeadoNuevo->archivo = mmapArchivo;
		archivoMappeadoNuevo->programas = list_create();
		archivoMappeadoNuevo->tabla_paginas = list_create();

		int total_pages_needed = ceil(length / PAGE_SIZE); // + ((length % PAGE_SIZE) != 0); //

		//vamos a pedir todas las paginas que necesitemos
		for(int i=0 ; i < total_pages_needed ; i++ )
		{
			page* pag;
			pag = (page *) malloc(sizeof(page));
			pag->is_used = false;
			pag->is_modify = false;
			pag->is_present = false;

			list_add(archivoMappeadoNuevo->tabla_paginas, pag);
		}

		archivoMappeado = archivoMappeadoNuevo;

		list_add(lista_archivos_mmap, archivoMappeado);
	}else{
		int igualPID(int pid){
			return pid == prog->pid;
		}
		int pidEncontrada = (int)list_find(archivoMappeado->programas,(void*) igualPID);

		if(flag == MAP_PRIVATE && (void*)pidEncontrada == NULL){
			log_error(debug_logger, "El archivo ya fue mappeado con la flag MAP_PRIVATE");
			return -2;
		}
	}

	log_debug(debug_logger, "Programa tiene ----> %d segmentos", list_size(prog->segment_table));
	direccionDelSegmento = crear_nuevo_segmento_mmap(length, prog);
	log_debug(debug_logger, "Direccion del segmento ----> %d", direccionDelSegmento);

	int nroSegmentoNuevo = busca_segmento(prog, direccionDelSegmento);

	segmentoNuevo = list_get(prog->segment_table, nroSegmentoNuevo);
	segmentoNuevo->archivo_mapeado = archivoMappeado;
	segmentoNuevo->page_table = archivoMappeado->tabla_paginas;
	segmentoNuevo->tam_archivo_mmap = length;

	if(flag == MAP_SHARED){
		log_info(debug_logger, "La flag de MAP es: MAP_SHARED");
		segmentoNuevo->tipo_map = 1;
		list_add(archivoMappeado->programas, (void*)pid);
	}else if(flag == MAP_PRIVATE){
		log_info(debug_logger, "La flag de MAP es: MAP_PRIVATE");
		segmentoNuevo->tipo_map = 0;
		if(list_size(archivoMappeado->programas) == 0){
			list_add(archivoMappeado->programas, pid);
		}
	}else{
		log_error(debug_logger, "No se reconocio la flag especificada");
		return -1;
	}

	log_debug(debug_logger, "Contenido del segmento mapeado: Base: %d, is_heap: %d, Limit: %d, TamanioArchivo: %d, TipoDeMap: %d",
			segmentoNuevo->base,
			segmentoNuevo->is_heap,
			segmentoNuevo->limit,
			segmentoNuevo->tam_archivo_mmap,
			segmentoNuevo->tipo_map);

	return segmentoNuevo->base;
}

uint32_t memory_sync(uint32_t direccion, size_t length, uint32_t pid)
{
	program* prog;
	segment* segmento_obtenido;
	int nro_prog;

	log_info(debug_logger, "Direccion solicitada: %d, Length: %d", direccion, length);

	//primero chequea que exista el programa
	if((nro_prog = search_program(pid)) == -1)
	{
		log_debug(debug_logger, "No lo encontre, lo creo ----> %d", pid);
		prog = (program *) malloc(sizeof(program));
		prog->pid = pid;
		prog->segment_table = list_create();
		nro_prog = list_add(program_list, prog);
		log_debug(debug_logger, "Se creo el prog n°%d de la lista de programas ", nro_prog);
	}else{
		int nro_prog = search_program(pid);
		prog = list_get(program_list, nro_prog);
	}

	prog = list_get(program_list, nro_prog);
	
	t_list *listaSeg = lista_de_segmentos();

	//busco el segmento que tenga asignado esa direccion
	int numSeg = busca_segmento(prog, direccion);
	segmento_obtenido = list_get(listaSeg, numSeg);

	list_destroy(listaSeg);

	int cantidad_paginas_necesarias = ceil((double)length / (double)PAGE_SIZE);
	log_debug(debug_logger, "Cantidad_paginas_necesarias %d", cantidad_paginas_necesarias);

	//segmentation fault
	if((segmento_obtenido == NULL) || (cantidad_paginas_necesarias > list_size(segmento_obtenido->page_table)) || (segmento_obtenido->base + segmento_obtenido->limit) < (direccion + length)){
		log_error(debug_logger, "Error: Segmentation Fault");
		return -2;
	} 

	if(!segmento_obtenido->is_heap){
		int pidEncontrada;

		int igualPID(int pid){
		    return pid == prog->pid;
		}

		pidEncontrada = (int)list_find(segmento_obtenido->archivo_mapeado->programas,(void*) igualPID);
		if((void*)pidEncontrada == NULL){
			log_error(debug_logger, "El archivo mmap es privado y el programa no tiene permisos");
			return -1;
		}
	}

	//error (returen -1)
	if((segmento_obtenido->is_heap) || (direccion % PAGE_SIZE) != 0){
		log_error(debug_logger, "Error: el segmento obtenido es heap o la direccion no esta al inicio de una pag");
		return -1;
	} 

	//busca el nro de pagina del segmento donde está direccion
	int nro_pagina_obtenida = (direccion - segmento_obtenido->base) / PAGE_SIZE;
	log_debug(debug_logger, "Nro de pag obtenida: %d", nro_pagina_obtenida);

	page* pagina_obtenida;
	void* direccion_datos;
	int offset;
	void* buffer;

	buffer = malloc(cantidad_paginas_necesarias*PAGE_SIZE);

	for(int i=0; i<cantidad_paginas_necesarias; i++)
	{

		pagina_obtenida = list_get(segmento_obtenido->page_table,i + nro_pagina_obtenida);
		direccion_datos = obtener_data_marco_mmap(segmento_obtenido, pagina_obtenida, i + nro_pagina_obtenida);
		memcpy(buffer + (PAGE_SIZE*i), direccion_datos, PAGE_SIZE);
	}

	//el primer byte a escribir no debería superar el tamaño del archivo
	if((nro_pagina_obtenida * PAGE_SIZE) <= segmento_obtenido->tam_archivo_mmap)
	{
		int nro_bytes = (int) fmin(length, segmento_obtenido->tam_archivo_mmap);
		log_debug(debug_logger, "Bytes a escribir: %d MUSE SYNC", nro_bytes);
		memcpy(segmento_obtenido->archivo_mapeado->archivo + nro_pagina_obtenida * PAGE_SIZE, buffer, nro_bytes);
		msync(segmento_obtenido->archivo_mapeado->archivo, segmento_obtenido->tam_archivo_mmap, MS_SYNC);

		if(nro_bytes < segmento_obtenido->tam_archivo_mmap)
			memset(segmento_obtenido->archivo_mapeado->archivo + nro_pagina_obtenida * PAGE_SIZE + nro_bytes + 1,
					0, segmento_obtenido->tam_archivo_mmap - nro_bytes);

		free(buffer);
		log_info(debug_logger, "Fin memory_sync");
		return 0; //unico caso que devuelve que está OK
	}
	free(buffer);

	log_error(debug_logger, "Error: la direccion encontrada es mayor que el tamaño de mmap_file");
	return -1;
}

void liberar_frame(int numeroFrame){
	log_debug(debug_logger, "Libero el frame: %d", numeroFrame);

	pthread_mutex_lock(&mutex_bitmap_heap);
	BITMAP[numeroFrame] = 1;
	pthread_mutex_unlock(&mutex_bitmap_heap);
}

void liberar_frame_swap(int frame){
	log_debug(debug_logger, "Libero el frame swap: %d", frame);

	pthread_mutex_lock(&mutex_bitmap_mmap);
	BITMAP_SWAP_FILE[frame] = 1;
	pthread_mutex_unlock(&mutex_bitmap_mmap);
}

void eliminar_archivo_mmap(archivoMMAP* archivo){

	if(archivo->tabla_paginas != NULL){

		void eliminar_pagina(page* pagina){
			if(pagina->is_present){
				pthread_mutex_lock(&mutex_MM);
				int numeroFrame = (pagina->fr - MAIN_MEMORY)/PAGE_SIZE;
				pthread_mutex_unlock(&mutex_MM);
				log_debug(debug_logger, "Frame a borrar: %d", numeroFrame);
				liberar_frame(numeroFrame);
			}
			free(pagina);
		}

		list_destroy_and_destroy_elements(archivo->tabla_paginas,(void*) eliminar_pagina);
	}
}

int memory_unmap(uint32_t dir, uint32_t pid)
{
	int nro_prog;
	program* prog;
	segment* segmentoBuscado;

	log_info(debug_logger, "Direccion solicitada: %d", dir);

	if((nro_prog = search_program(pid)) == -1)
	{
		prog = (program *) malloc(sizeof(program));
		prog->pid = pid;
		prog->segment_table = list_create();
		nro_prog = list_add(program_list, prog);
		log_debug(debug_logger, "Se creo el prog n°%d de la lista de programas ", nro_prog);
	}else{
		int nro_progg = search_program(pid);
		prog = list_get(program_list, nro_progg);
	}

	int nroSegmento = busca_segmento(prog, dir);

	segmentoBuscado = list_get(prog->segment_table, nroSegmento);

	if(segmentoBuscado == NULL){
	log_error(debug_logger, "Segmentation fault");
		return -1;
	}

	if(!segmentoBuscado->is_heap){
		int pidEncontrada;

		int igualPID(int pid){
		    return pid == prog->pid;
		}

		pidEncontrada = (int)list_find(segmentoBuscado->archivo_mapeado->programas,(void*) igualPID);
		if((void*)pidEncontrada == NULL){
			log_error(debug_logger, "El archivo mmap es privado y el programa no tiene permisos");
			return -1;
		}
	}

	else if((segmentoBuscado->is_heap != 0) || (dir != segmentoBuscado->base)){
	log_error(debug_logger, "No es de MMAP el segmento buscado.");
		return -1;
	}

	archivoMMAP* archivoMapeado = segmentoBuscado->archivo_mapeado;
	log_info(debug_logger, "El archivo mapeado tiene: Path: %s", archivoMapeado->pathArchivo);

	int igualPID(int pid) {
		return pid == prog->pid;
	}
	list_remove_by_condition(archivoMapeado->programas,(void*) igualPID);

	list_remove(prog->segment_table, nroSegmento);

	if(list_size(archivoMapeado->programas) == 0){
		list_destroy(archivoMapeado->programas);
		munmap(segmentoBuscado->archivo_mapeado, segmentoBuscado->tam_archivo_mmap);

		log_debug(debug_logger, "Eliminando archivo MMAP");
		bool igualArchivo(archivoMMAP* archivo) {
				return !strcmp(archivo->pathArchivo, archivoMapeado->pathArchivo);
		}
		list_remove_and_destroy_by_condition(lista_archivos_mmap,(void*) igualArchivo,(void*) eliminar_archivo_mmap);
		log_debug(debug_logger, "Elimine todas las paginas", nro_prog);

		list_destroy(segmentoBuscado->page_table);
		free(archivoMapeado->pathArchivo);
		archivoMapeado->pathArchivo = NULL;
	}

	log_debug(debug_logger, "segmento tiene: base: %d, limite %d, is_heap: %d, tamarchivo: %d",
				segmentoBuscado->base,
				segmentoBuscado->limit,
				segmentoBuscado->is_heap,
				segmentoBuscado->tam_archivo_mmap);

	free(segmentoBuscado);
	segmentoBuscado = NULL;

	return 0;
}

// Copia n bytes de MUSE a LIBMUSE
void* memory_get(void *dst, uint32_t src, size_t numBytes, uint32_t pid)
{
	program* prog = NULL;
	log_info(debug_logger, "Direccion destino: %d", src);
	log_info(debug_logger, "Cantidad de bytes a leer en la direccion: %d", numBytes);

	int numProg = search_program(pid);
	log_debug(debug_logger, "Proceso que hace call a memory_get: %d", numProg);

	if((numProg = search_program(pid)) == -1)
	{
		prog = (program *) malloc(sizeof(program));
		prog->pid = pid;
		prog->segment_table = list_create();
		numProg = list_add(program_list, prog);
		log_debug(debug_logger, "Se creo el prog n°%d de la lista de programas ", numProg);
	}

	prog = list_get(program_list, numProg);

	int numSeg = busca_segmento(prog, src);
	segment* segmento = list_get(prog->segment_table, numSeg);

	log_debug(debug_logger, "Nro de segmento: %d", numSeg);

	if(segmento == NULL){
		log_error(debug_logger, "El segmento no existe");
		return -3;
	}

	if((segmento->base + segmento->limit) < (src + numBytes)){
		log_error(debug_logger, "Estas tratando de leer mas bytes de los que tiene el segmento");
		return -4;
	}

	log_debug(debug_logger, "Base del segmento: %d", segmento->base);

	if(!segmento->is_heap){
		int pidEncontrada;

		if(segmento->archivo_mapeado->pathArchivo == NULL){
			log_error(debug_logger, "Se trata de acceder a una direccion unmapped");
			int respuesta = -1;
			return -1;
		}

		int igualPID(int pid){
		    return pid == prog->pid;
		}

		pidEncontrada = (int)list_find(segmento->archivo_mapeado->programas,(void*) igualPID);
		if((void*)pidEncontrada == NULL){
			log_error(debug_logger, "El archivo mmap es privado y el programa no tiene permisos");
			return -2;
		}
	}

	int numPage = floor((src - segmento->base) / PAGE_SIZE);
	log_debug(debug_logger, "La pagina que quiero leer es: %d", numPage);

	int offset = (src - segmento->base) % PAGE_SIZE;
	log_debug(debug_logger, "El offset dentro de la pagina es: %d", numPage);

	int cant_pag_necesarias = (int)ceil((double)(offset + numBytes)/ PAGE_SIZE);
	log_debug(debug_logger, "La cantidad de paginas a leer es: %d", cant_pag_necesarias);

	void* buffer = malloc(cant_pag_necesarias * PAGE_SIZE);
	page* pagina;
	void* datos;

	//le cargo al buffer todas las paginas
	for(int i=0; i<cant_pag_necesarias; i++){

		pagina = list_get(segmento->page_table,i + numPage);
		if(segmento->is_heap){
			datos = obtener_data_marco_heap(pagina);
	 	}
	 	else{
	 		datos = obtener_data_marco_mmap(segmento, pagina, i + numPage);
		}

	    memcpy(buffer + PAGE_SIZE * i, datos, PAGE_SIZE);
	}

	//obtengo del buffer lo que necesito (usando offset y bytes a leer)
	memcpy(dst, buffer + offset, numBytes);

	free(buffer);

	return dst;
}
