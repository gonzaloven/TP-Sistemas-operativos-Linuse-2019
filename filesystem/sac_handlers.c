#include "sac_handlers.h"

char **splitPath(char *path, int *size){
    char *tmp;
    char **splitted = NULL;
    int i, length;

    if (!path){
    	*size = 0;
    	return NULL;
    }

    tmp = strdup(path);
    length = strlen(tmp);

    *size = 1;
    for (i = 0; i < length; i++) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';
            (*size)++;
        }
    }

    splitted = (char **)malloc(*size * sizeof(*splitted));
    if (!splitted) {
        free(tmp);
        *size = 0;
        return NULL;
    }

    for (i = 0; i < *size; i++) {
        splitted[i] = strdup(tmp);
        tmp += strlen(splitted[i]) + 1;
    }
    return splitted;
}

int esElNodoBuscado(int nodo, char* filename, ptrGBloque nodoPadre){

	return tablaDeNodos[nodo].state != 0
			&& tablaDeNodos[nodo].parent_dir_block == nodoPadre
			&& !strcmp(tablaDeNodos[nodo].fname, filename);
}

int buscar_nodo_por_nombre(char *filenameBuscado, ptrGBloque nodoPadre){
	int i;

	for(i=0; i< MAX_NUMBER_OF_FILES && !esElNodoBuscado(i, filenameBuscado, nodoPadre); i++);

	if(i == MAX_NUMBER_OF_FILES){
		return -1;
	}

	return i;
}

int largoListaString(char** lista){
	int largo = 0;
	int i = 0;
	for(i=0; lista[i] != NULL; i++){
		largo++;
	}

	return largo;
}

int determine_nodo(char *path){
	ptrGBloque nodoUltimoPadre = bloqueInicioTablaDeNodos;
	char *filenameBuscado;

	int dimListaSpliteada, i;
	char **listaSpliteada;

	listaSpliteada = string_split(path, "/");
	dimListaSpliteada = largoListaString(listaSpliteada);

	if(dimListaSpliteada == 1){
		filenameBuscado = listaSpliteada[0];
		nodoUltimoPadre = buscar_nodo_por_nombre(filenameBuscado, 0);
		free(listaSpliteada[0]);
		listaSpliteada[0] = NULL;
		free(listaSpliteada);
		listaSpliteada = NULL;
		return nodoUltimoPadre;
	}

	for(i=0 ; i<dimListaSpliteada; i++){

		if(i == 0){
			filenameBuscado = listaSpliteada[0];
			nodoUltimoPadre = buscar_nodo_por_nombre(filenameBuscado, 0) + bloqueInicioTablaDeNodos;
			continue;
		}

		filenameBuscado = listaSpliteada[i];

		nodoUltimoPadre = buscar_nodo_por_nombre(filenameBuscado, nodoUltimoPadre);

		if(nodoUltimoPadre == -1){
			for(int y=0; y <= i; y++){
				free(listaSpliteada[y]);
				listaSpliteada[y] = NULL;
			}

			free(listaSpliteada);
			listaSpliteada = NULL;
			return -1;
		}else{
			nodoUltimoPadre += bloqueInicioTablaDeNodos;
		}
	}

	for(int y=0; y<dimListaSpliteada; y++){
		free(listaSpliteada[y]); // libero cada integrante de la matriz
		listaSpliteada[y] = NULL;
	}


	free(listaSpliteada);
	listaSpliteada = NULL;
	return nodoUltimoPadre - bloqueInicioTablaDeNodos;
}

int tamano_malloc_list(t_list* lista){
	int tamano = 0;
	char* nombre;
	for(int i=0; i < list_size(lista); i++){
		nombre = (char*)list_get(lista, i);
		tamano+= strlen((char*)list_get(lista, i)) + 1;
	}
 //Aca tambien hay memory leaks
	return tamano;
}

void lista_a_string(t_list* lista, char** string){
	*string = malloc(tamano_malloc_list(lista));

	if(list_size(lista) > 0)
		strcpy(*string, (char*)list_get(lista, 0));

	for(int i=1; i < list_size(lista); i++){
		strcat(*string, "/");
		strcat(*string, (char*)list_get(lista, i));
	}

	int jamon = 1;
}

void borrar_contenido(int nodoABorrarPosicion, int tamanio){

	int tamanioMaximoDireccionablePorPuntero = (PUNTEROS_A_BLOQUES_DATOS * BLOQUE_SIZE);

	for(int i = 0; i < tamanio / tamanioMaximoDireccionablePorPuntero; i++){
		ptrGBloque bloqueDePunterosPosicion = tablaDeNodos[nodoABorrarPosicion].indirect_blocks_array[i];

		punterosBloquesDatos *bloqueDePunterosDatos = (punterosBloquesDatos *) (disco + bloqueDePunterosPosicion);

		for(int j=0; i<PUNTEROS_A_BLOQUES_DATOS; j++){

			ptrGBloque bloqueDeDatosPosicion = bloqueDePunterosDatos->punteros_a_bloques[j];

			bitarray_clean_bit(bitmap, bloqueDeDatosPosicion);
		}

		bitarray_clean_bit(bitmap, bloqueDePunterosPosicion);
	}
	//////////////////////////////////Esto de abajo creo que esta mal, podria ponerle el valor que le quedo a i y listo en la posicion de ind. blocks array
	ptrGBloque ultimoBloquePunterosDirectos = tablaDeNodos[nodoABorrarPosicion].indirect_blocks_array[(tamanio / tamanioMaximoDireccionablePorPuntero) + 1];

	punterosBloquesDatos *bloqueDePunterosDatosFaltantes = (punterosBloquesDatos *) (disco + ultimoBloquePunterosDirectos);

	for(int i = 0; i < (tamanio % tamanioMaximoDireccionablePorPuntero) / BLOQUE_SIZE; i++){
		ptrGBloque bloqueDeDatosPosicion = bloqueDePunterosDatosFaltantes->punteros_a_bloques[i];

		bitarray_clean_bit(bitmap, bloqueDeDatosPosicion);
	}
	msync(disco, diskSize, MS_SYNC);
}

int borrar_archivo(GFile* nodoABorrar, int nodoABorrarPosicion){

	if(nodoABorrarPosicion == -1){
		return -1;
	}

	if(tablaDeNodos[nodoABorrarPosicion].state == 2){
		nodoABorrar->state = 0;
		nodoABorrar->file_size = 0;
		return 0;
	}

	if(tablaDeNodos[nodoABorrarPosicion].file_size != 0){
		borrar_contenido(nodoABorrarPosicion, tablaDeNodos[nodoABorrarPosicion].file_size);
	}

	nodoABorrar->state = 0;
	nodoABorrar->file_size = 0;

	msync(disco, diskSize, MS_SYNC);
	return 0;
}

int borrar_directorio (GFile* nodoABorrar, int nodoABorrarPosicion){
	int currNode = 0;

	borrar_archivo(nodoABorrar, nodoABorrarPosicion);

	while(currNode < MAX_NUMBER_OF_FILES){
		if(tablaDeNodos[currNode].state != 0)
		{
			if(tablaDeNodos[currNode].parent_dir_block == (nodoABorrarPosicion + 2))
			{
				GFile *nodoABorrar = tablaDeNodos + currNode;
				if(tablaDeNodos[currNode].state == 1){
					borrar_archivo(nodoABorrar, currNode);
				}else{
					borrar_directorio(nodoABorrar, currNode);
				}
			}
		}
		currNode++;
	}
	return 0;
}

Function retornar_error(Function funcionAEnviar){
	Arg argerror[1];

	argerror[0].type = VAR_UINT32;
	argerror[0].size = sizeof(uint32_t);
	argerror[0].value.val_u32 = -1;

	funcionAEnviar.type = -1;
	funcionAEnviar.num_args = 1;
	funcionAEnviar.args[0] = argerror[0];

	return funcionAEnviar;
}

Function enviar_respuesta_basica(int respuesta, FuncType tipoFuncion){
	Message msg;
	Function fsend;
	Arg arg[1];

	if(respuesta == 0){
		arg[0].type = VAR_UINT32;
		arg[0].size = sizeof(uint32_t);
		arg[0].value.val_u32 = 0;
		fsend.type = tipoFuncion;
	}else{
		return retornar_error(fsend);
	}

	fsend.num_args = 1;
	fsend.args[0] = arg[0];

	return fsend;
}

int crear_nuevo_nodo (char* path, int tipoDeArchivo){
	int currNode = 0;
	char *parentDirPath;
	char *fileName;
	int nodoPadre;
	int dimListaSpliteada;
	char **listaSpliteada;

	///////////////////////////////////////FALTA VALIDAR QUE NO PUEDA HABER MAS DE UN NODO CON EL MISMO NOMBRE Y CON EL MISMO PADRE A LA VEZ

	listaSpliteada = string_split(path, "/");
	dimListaSpliteada = largoListaString(listaSpliteada);
	fileName = listaSpliteada[dimListaSpliteada - 1];

	while(tablaDeNodos[currNode].state != 0 && currNode < MAX_NUMBER_OF_FILES){
		currNode++;
	}

	if (currNode >= MAX_NUMBER_OF_FILES)
	{
		for(int y=0; y<dimListaSpliteada; y++) // libero cada integrante de la matriz
			free(listaSpliteada[y]);

		free(listaSpliteada);
		return EDQUOT;
	}

	time_t tiempoAhora;
	uint64_t timestamp;
	tiempoAhora = time(0);
	timestamp = (uint64_t) tiempoAhora;

	if(dimListaSpliteada > 1){
		char* pathduplicado = strdup(path);
		parentDirPath = dirname(pathduplicado);
		nodoPadre = determine_nodo(parentDirPath);
		free(pathduplicado);
	}

	GFile *nodoVacio = tablaDeNodos + currNode;

	strcpy((char*) nodoVacio->fname, fileName);
	nodoVacio->file_size = 0;

	if(dimListaSpliteada > 1){
		nodoVacio->parent_dir_block = nodoPadre + bloqueInicioTablaDeNodos;
	}else{
		nodoVacio->parent_dir_block = 0;
	}

	nodoVacio->state = tipoDeArchivo;
	nodoVacio->create_date = timestamp;
	nodoVacio->modify_date = timestamp;

	msync(disco, diskSize, MS_SYNC);

	for(int y=0; y<dimListaSpliteada; y++) // libero cada integrante de la matriz
		free(listaSpliteada[y]);

	free(listaSpliteada);
	return 0;
}

void completar_lista_con_fileNames(t_list* listaDeArchivos, ptrGBloque nodoPadre){
	for(int i = 0; i < MAX_NUMBER_OF_FILES; i++){
		if(tablaDeNodos[i].state != 0)
		{
			if(tablaDeNodos[i].parent_dir_block == nodoPadre)
			{
				char * name = malloc(strlen(tablaDeNodos[i].fname) + 1);
				strcpy(name, tablaDeNodos[i].fname);
				list_add(listaDeArchivos, tablaDeNodos[i].fname);
				free(name);
			}
		}
	}
}

Function validarSiExiste(char* path, FuncType tipoFuncion){
	Message msg;
	Function fsend;
	Arg arg[1];

	if(!strcmp(path, "/")){
		arg[0].type = VAR_UINT32;
		arg[0].size = sizeof(uint32_t);
		arg[0].value.val_u32 = 0;
		fsend.type = tipoFuncion;
		fsend.num_args = 1;
		fsend.args[0] = arg[0];
		return fsend;

	}

	int nodoBuscado = determine_nodo(path);

	if(nodoBuscado == -1){
		arg[0].type = VAR_UINT32;
		arg[0].size = sizeof(uint32_t);
		arg[0].value.val_u32 = -ENOENT;
		fsend.type = -1;
	}else{
		arg[0].type = VAR_UINT32;
		arg[0].size = sizeof(uint32_t);
		arg[0].value.val_u32 = 0;
		fsend.type = tipoFuncion;
	}

	fsend.num_args = 1;
	fsend.args[0] = arg[0];

	return fsend;
}

int leer_archivo(char* buffer, char *path, size_t size, uint32_t offset){
	//log_info(logger, "Reading: Path: %s - Size: %d - Offset %d",path, size, offset);
	unsigned int nodoDelArchivo, bloque_punteros, num_bloque_datos;
	unsigned int bloque_a_buscar;
	struct sac_server_gfile *node;
	ptrGBloque *pointer_block;
	char *data_block;
	size_t tamanio = size;
	int respuesta;

	nodoDelArchivo = determine_nodo(path);

	node = tablaDeNodos + nodoDelArchivo;

	//pthread_rwlock_rdlock(&rwlock); //Toma un lock de lectura.
	//log_lock_trace(logger, "Read: Toma lock lectura. Cantidad de lectores: %d", rwlock.__data.__nr_readers);

	if(node->file_size <= offset){
		//log_error(logger, "Se intenta leer un offset mayor o igual que el tamanio de archivo. Se retorna size 0. File: %s, Size: %d", path, node->file_size);
		// TODO enviar error al cliente
		respuesta = 0;
		goto finalizar;
	} else if (node->file_size <= (offset + size)){
		//tamanio = size = ((node->file_size) - (offset));
		// TODO enviar error al cliente
		//log_error(logger, "Se intenta leer una posicion mayor o igual que el tamanio de archivo. Se retornaran %d bytes. File: %s, Size: %d", size, path, node->file_size);
		respuesta = 0;
		goto finalizar;
	}

	for (bloque_punteros = 0; bloque_punteros < BLKINDIRECT; bloque_punteros++){

		// Chequea el offset y lo acomoda para leer lo que realmente necesita
		if (offset > BLOQUE_SIZE * 1024){
			offset -= (BLOQUE_SIZE * 1024);
			continue;
		}

		bloque_a_buscar = (node->indirect_blocks_array)[bloque_punteros];	// Ubica el nodo de punteros a nodos de datos, es relativo al nodo 0: Header.
		bloque_a_buscar -= (1 + (bloqueInicioTablaDeNodos - 1) + MAX_NUMBER_OF_FILES);	// Acomoda el nodo de punteros a nodos de datos, es relativo al bloque de datos.
		pointer_block =(ptrGBloque *) &(data_block_start[bloque_a_buscar]);		// Apunta al nodo antes ubicado. Lo utiliza para saber de donde leer los datos.
		// Recorre el bloque de punteros correspondiente.
		for (num_bloque_datos = 0; num_bloque_datos < 1024; num_bloque_datos++){
		// Chequea el offset y lo acomoda para leer lo que realmente necesita
			if (offset >= BLOQUE_SIZE){
				offset -= BLOQUE_SIZE;
				continue;
			}

			bloque_a_buscar = pointer_block[num_bloque_datos]; 	// Ubica el nodo de datos correspondiente. Relativo al nodo 0: Header.
			bloque_a_buscar -= (1 + (bloqueInicioTablaDeNodos - 1) + MAX_NUMBER_OF_FILES);	// Acomoda el nodo, haciendolo relativo al bloque de datos.
			data_block = (char *) &(data_block_start[bloque_a_buscar]);

			// Corre el offset hasta donde sea necesario para poder leer lo que quiere.
			if (offset > 0){
				data_block += offset;
				offset = 0;
			}

			if (tamanio < BLOQUE_SIZE){
				memcpy(buffer, data_block, tamanio);
				buffer = &(buffer[tamanio]);
				tamanio = 0;
				break;
			} else {
				memcpy(buffer, data_block, BLOQUE_SIZE);
				tamanio -= BLOQUE_SIZE;
				buffer = &(buffer[BLOQUE_SIZE]);
				if (tamanio == 0) break;
			}

		}

		if (tamanio == 0) break;
	}
	respuesta = size;

	finalizar:
	//pthread_rwlock_unlock(&rwlock); //Devuelve el lock de lectura.
	//log_lock_trace(logger, "Read: Libera lock lectura. Cantidad de lectores: %d", rwlock.__data.__nr_readers);
	//log_trace(logger, "Terminada lectura.");
	return respuesta;
}

/*int escribir_archivo (char* buffer, char *path, size_t size, uint32_t offset){
	//log_trace(logger, "Writing: Path: %s - Size: %d - Offset %d", path, size, offset);

	int nodoDelArchivo = determine_nodo(path);
	int nodo_libre_nuevo;
	struct grasa_file_t *node;
	char *data_block;
	size_t tam = size, file_size, space_in_block, offset_in_block = offset % BLOCKSIZE;
	off_t off = offset;
	int *n_pointer_block = malloc(sizeof(int)), *n_data_block = malloc(sizeof(int));
	ptrGBloque *pointer_block;
	int res = size;

	// Ubica el nodo correspondiente al archivo
	node = tablaDeNodos + nodoDelArchivo;

	if ((file_size + size) >= THELARGESTFILE) return -EFBIG;

	// Toma un lock de escritura.
			log_lock_trace(logger, "Write: Pide lock escritura. Escribiendo: %d. En cola: %d.", rwlock.__data.__writer, rwlock.__data.__nr_writers_queued);
	pthread_rwlock_wrlock(&rwlock);
			log_lock_trace(logger, "Write: Recibe lock escritura.");

	// Guarda tantas veces como sea necesario, consigue nodos y actualiza el archivo.
	while (tam != 0){

		// Actualiza los valores de espacio restante en bloque.
		space_in_block = BLOCKSIZE - (file_size % BLOCKSIZE);
		if (space_in_block == BLOCKSIZE) (space_in_block = 0); // Porque significa que el bloque esta lleno.
		if (file_size == 0) space_in_block = BLOCKSIZE;  Significa que el archivo esta recien creado y ya tiene un bloque de datos asignado

		// Si el offset es mayor que el tamanio del archivo mas el resto del bloque libre, significa que hay que pedir un bloque nuevo
		// file_size == 0 indica que es un archivo que recien se comienza a escribir, por lo que tiene un tratamiento distinto (ya tiene un bloque de datos asignado).
		if ((off >= (file_size + space_in_block)) & (file_size != 0)){

			// Si no hay espacio en el disco, retorna error.
			if (bitmap_free_blocks == 0) return -ENOSPC;

			// Obtiene un bloque libre para escribir.
			new_free_node = get_node();
			if (new_free_node < 0) goto finalizar;

			// Agrega el nodo al archivo.
			res = add_node(node, new_free_node);
			if (res != 0) goto finalizar;

			// Lo relativiza al data block.
			new_free_node -= (GHEADERBLOCKS + NODE_TABLE_SIZE + BITMAP_BLOCK_SIZE);
			data_block = (char*) &(data_block_start[new_free_node]);

			// Actualiza el espacio libre en bloque.
			space_in_block = BLOCKSIZE;

		} else {
			// Ubica a que nodo le corresponderia guardar el dato
			set_position(n_pointer_block, n_data_block, file_size, off);

			//Ubica el nodo a escribir.
			*n_pointer_block = node->blk_indirect[*n_pointer_block];
			*n_pointer_block -= (GHEADERBLOCKS + NODE_TABLE_SIZE + BITMAP_BLOCK_SIZE);
			pointer_block = (ptrGBloque*) &(data_block_start[*n_pointer_block]);
			*n_data_block = pointer_block[*n_data_block];
			*n_data_block -= (GHEADERBLOCKS + NODE_TABLE_SIZE + BITMAP_BLOCK_SIZE);
			data_block = (char*) &(data_block_start[*n_data_block]);
		}

		// Escribe en ese bloque de datos.
		if (tam >= BLOCKSIZE){
			memcpy(data_block, buf, BLOCKSIZE);
			if ((node->file_size) <= (off)) file_size = node->file_size += BLOCKSIZE;
			buf += BLOCKSIZE;
			off += BLOCKSIZE;
			tam -= BLOCKSIZE;
			offset_in_block = 0;
		} else if (tam <= space_in_block){ Hay lugar suficiente en ese bloque para escribir el resto del archivo
			memcpy(data_block + offset_in_block, buf, tam);
			if (node->file_size <= off) file_size = node->file_size += tam;
			else if (node->file_size <= (off + tam)) file_size = node->file_size += (off + tam - node->file_size);
			tam = 0;
		} else {  Como no hay lugar suficiente, llena el bloque y vuelve a buscar uno nuevo
			memcpy(data_block + offset_in_block, buf, space_in_block);
			file_size = node->file_size += space_in_block;
			buf += space_in_block;
			off += space_in_block;
			tam -= space_in_block;
			offset_in_block = 0;
		}

	}

	node->m_date= time(NULL);

	res = size;

	finalizar:
	// Devuelve el lock de escritura.
	pthread_rwlock_unlock(&rwlock);
			log_lock_trace(logger, "Write: Devuelve lock escritura. En cola: %d", rwlock.__data.__nr_writers_queued);
			log_trace(logger, "Terminada escritura.");
	return res;

}*/
