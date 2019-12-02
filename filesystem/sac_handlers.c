#include "sac_handlers.h"


int tamDataFunction(Function f){
	int tamano = 0;
	tamano+= sizeof(uint8_t);
	tamano+= sizeof(uint8_t);
	for(int y=0; y < f.num_args; y++){
		tamano+= sizeof(uint8_t);
		tamano+= sizeof(uint32_t);
		tamano+= f.args[y].size;
	}
	return tamano;
}

pthread_mutex_t s_bitmap = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t s_tablaDeNodos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t s_tablaDeBloquesDeDatos = PTHREAD_MUTEX_INITIALIZER;

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

int validar_si_ya_existe_otro(char* path, char* fileName){
	char* pathduplicado = strdup(path);
	char* parentDirPath = dirname(pathduplicado);
	int nodoPadreDenuevo = determine_nodo(parentDirPath);
	int bloquePadre = nodoPadreDenuevo + bloqueInicioTablaDeNodos;

	for(int i=0; i<MAX_NUMBER_OF_FILES; i++){
		if(tablaDeNodos[i].parent_dir_block == bloquePadre){
			if(!strcmp(tablaDeNodos[i].fname, fileName)){
				log_error(fuse_logger, "Ya existe un nodo con ese nombre en este directorio.");

				free(pathduplicado);
				return -EEXIST;
			}
		}
	}

	free(pathduplicado);
	return 0;
}

int renombrar_archivo(int nodoBuscadoPosicion, char* path, char* nuevoPath){
	char* pathDuplicado = strdup(nuevoPath);
	char* pathDuplicadoDenuevo = strdup(nuevoPath);
	char* parentDirNuevoPath;
	char* fileName;
	int respuesta = 0;

	parentDirNuevoPath = dirname(pathDuplicado);
	fileName = basename(pathDuplicadoDenuevo);

	int existenciaDeOtro = validar_si_ya_existe_otro(nuevoPath, fileName);

	if(existenciaDeOtro != 0){
		free(pathDuplicadoDenuevo);
		free(pathDuplicado);
		return existenciaDeOtro;
	}

	int nodoNuevoPadre = determine_nodo(parentDirNuevoPath);

	if(nodoNuevoPadre == -1){
		free(pathDuplicadoDenuevo);
		free(pathDuplicado);
		respuesta = -1;
		return respuesta;
	}

	GFile *nodoARenombrar = tablaDeNodos + nodoBuscadoPosicion;

	pthread_mutex_lock(&s_tablaDeNodos);
	memset(nodoARenombrar->fname,0,strlen(nodoARenombrar->fname) + 1);

	strcpy((char*) nodoARenombrar->fname, fileName);

	if(nodoNuevoPadre == 0){
		nodoARenombrar->parent_dir_block = 0;
		pthread_mutex_unlock(&s_tablaDeNodos);
		free(pathDuplicadoDenuevo);
		free(pathDuplicado);
		return respuesta;
	}

	nodoARenombrar->parent_dir_block = nodoNuevoPadre + bloqueInicioTablaDeNodos;

	pthread_mutex_unlock(&s_tablaDeNodos);

	free(pathDuplicadoDenuevo);
	free(pathDuplicado);

	return respuesta;
}

int truncar_archivo(int nodoBuscadoPosicion, uint32_t size){
	int respuesta = 0;

	if(nodoBuscadoPosicion == -1){
		respuesta = -1;
		return respuesta;
	}

	int file_size = tablaDeNodos[nodoBuscadoPosicion].file_size;

	GFile *nodoATruncar = tablaDeNodos + nodoBuscadoPosicion;

	if(size > file_size){
		respuesta = get_new_space(nodoATruncar, (size - file_size));
	}else{
		int pointer_to_delete;
		int data_to_delete;

		setear_posicion(&pointer_to_delete, &data_to_delete, 0, size);

		respuesta = delete_nodes_upto(nodoATruncar, pointer_to_delete, data_to_delete);
	}

	pthread_mutex_lock(&s_tablaDeNodos);
	nodoATruncar->file_size = size;
	pthread_mutex_unlock(&s_tablaDeNodos);

	msync(disco, diskSize, MS_SYNC);

	return respuesta;
}

int setear_posicion (int *pointer_block, int *data_block, size_t size, off_t offset){
	div_t divi;
	divi = div(offset, (BLOQUE_SIZE*PUNTEROS_A_BLOQUES_DATOS));
	*pointer_block = divi.quot;
	*data_block = divi.rem / BLOQUE_SIZE;
	return 0;
}


int delete_nodes_upto (GFile *file_data, int pointer_upto, int data_upto){
	size_t file_size = file_data->file_size;
	int node_to_delete, node_pointer_to_delete, delete_upto;
	punterosBloquesDatos *aux; // Auxiliar utilizado para saber que nodo redireccionar
	int data_pos, pointer_pos;

	// Ubica cual es el ultimo nodo del archivo
	setear_posicion(&pointer_pos, &data_pos, 0, file_size);
	if (file_size%(BLOQUE_SIZE*PUNTEROS_A_BLOQUES_DATOS) == 0) {
		pointer_pos--;
		data_pos = PUNTEROS_A_BLOQUES_DATOS-1;
	}
	else if (file_size%BLOQUE_SIZE == 0) data_pos--;

	// Activa el DELETE_MODE. Este modo NO debe activarse cuando se hacen operaciones que
	// dejen al archivo con un solo nodo. Por ejemplo, truncate -s 0.
	// Deberia estar activo en otro momento.
	if((pointer_upto != 0) | (data_upto != 0)) DISABLE_DELETE_MODE;

	// Borra hasta que los nodos de posicion coincidan con los nodos especificados.
	while( (data_pos != data_upto) | (pointer_pos != pointer_upto) | (DELETE_MODE == 1) ){  // | ((data_pos == 0) & (pointer_pos == 0)) ){
		if ((data_pos < 0) | (pointer_pos < 0)) break;

		// localiza el puntero de datos a borrar.
		node_pointer_to_delete = file_data->indirect_blocks_array[pointer_pos];
		aux = (punterosBloquesDatos *) (disco + node_pointer_to_delete);

		// Indica hasta que nodo debe borrar.
		if (pointer_pos == pointer_upto){
			delete_upto = data_upto;
		} else {
			delete_upto = 0;
		}

		// Borra los nodos de datos que sean necesarios.
		while ((data_pos + DELETE_MODE) != delete_upto){
			node_to_delete = aux->punteros_a_bloques[data_pos];
			pthread_mutex_lock(&s_tablaDeNodos);
			aux->punteros_a_bloques[data_pos] = 0;
			pthread_mutex_unlock(&s_tablaDeNodos);
			pthread_mutex_lock(&s_bitmap);
			bitarray_clean_bit(bitmap, node_to_delete);
			pthread_mutex_unlock(&s_bitmap);
			data_pos--;
		}

		// Si es necesario, borra el nodo de punteros.
		if ((pointer_pos + DELETE_MODE) != pointer_upto){
			pthread_mutex_lock(&s_bitmap);
			bitarray_clean_bit(bitmap, node_pointer_to_delete);
			pthread_mutex_unlock(&s_bitmap);
			pthread_mutex_lock(&s_tablaDeNodos);
			file_data->indirect_blocks_array[pointer_pos] = 0;
			pthread_mutex_unlock(&s_tablaDeNodos);
			pointer_pos--;
		}

	}

	return 0;
}

int get_new_space (GFile *file_data, int size){
	size_t file_size = file_data->file_size, space_in_block = file_size % BLOQUE_SIZE;
	int new_node;
	space_in_block = BLOQUE_SIZE - space_in_block; // Calcula cuanto tamanio le queda para ocupar en el bloque

	// Si no hay suficiente espacio, retorna error.
	if ((cantidad_bloques_libres()*BLOQUE_SIZE) < (size - space_in_block)) return -ENOSPC;

	// Actualiza el file size al tamanio que le corresponde:
	if (space_in_block >= size){
		pthread_mutex_lock(&s_tablaDeNodos);
		file_data->file_size += size;
		pthread_mutex_unlock(&s_tablaDeNodos);
		return 0;
	} else {
		pthread_mutex_lock(&s_tablaDeNodos);
		file_data->file_size += space_in_block;
		pthread_mutex_unlock(&s_tablaDeNodos);
	}

	while ( (space_in_block <= size) ){ // Siempre que lo que haya que escribir sea mas grande que el espacio que quedaba en el bloque
		new_node = get_bloque_vacio();
		agregar_nodo(file_data, new_node);
		size -= BLOQUE_SIZE;
		pthread_mutex_lock(&s_tablaDeNodos);
		file_data->file_size += BLOQUE_SIZE;
		pthread_mutex_unlock(&s_tablaDeNodos);
	}

	pthread_mutex_lock(&s_tablaDeNodos);
	file_data->file_size += size;
	pthread_mutex_unlock(&s_tablaDeNodos);

	return 0;
}

void borrar_contenido(int nodoABorrarPosicion, int tamanio){

	int i = 0;
	int tamanioMaximoDireccionablePorPuntero = (PUNTEROS_A_BLOQUES_DATOS * BLOQUE_SIZE);

	pthread_mutex_lock(&s_bitmap);

	for(i = 0; i < tamanio / tamanioMaximoDireccionablePorPuntero; i++){
		ptrGBloque bloqueDePunterosPosicion = tablaDeNodos[nodoABorrarPosicion].indirect_blocks_array[i];

		punterosBloquesDatos *bloqueDePunterosDatos = (punterosBloquesDatos *) (disco + bloqueDePunterosPosicion);

		for(int j=0; j<PUNTEROS_A_BLOQUES_DATOS; j++){

			ptrGBloque bloqueDeDatosPosicion = bloqueDePunterosDatos->punteros_a_bloques[j];

			bitarray_clean_bit(bitmap, bloqueDeDatosPosicion);

			msync(disco, diskSize, MS_SYNC);
		}

		bitarray_clean_bit(bitmap, bloqueDePunterosPosicion);

		msync(disco, diskSize, MS_SYNC);
	}

	ptrGBloque ultimoBloquePunterosDirectos = tablaDeNodos[nodoABorrarPosicion].indirect_blocks_array[i];

	punterosBloquesDatos *bloqueDePunterosDatosFaltantes = (punterosBloquesDatos *) (disco + ultimoBloquePunterosDirectos);

	for(int j = 0; j < ceil((float) (tamanio % tamanioMaximoDireccionablePorPuntero) / BLOQUE_SIZE); j++){
		ptrGBloque bloque = bloqueDePunterosDatosFaltantes->punteros_a_bloques[j];

		bitarray_clean_bit(bitmap, bloque);

		msync(disco, diskSize, MS_SYNC);
	}

	bitarray_clean_bit(bitmap, ultimoBloquePunterosDirectos);
	pthread_mutex_unlock(&s_bitmap);

	log_info(fuse_logger, "Archivo -> Nodo: %d | Contenido borrado correctamente.", nodoABorrarPosicion);

	msync(disco, diskSize, MS_SYNC);
}

int borrar_archivo(GFile* nodoABorrar, int nodoABorrarPosicion){

	if(nodoABorrarPosicion == -1){
		return -1;
	}

	if(tablaDeNodos[nodoABorrarPosicion].state == 2){
		nodoABorrar->state = 0;
		nodoABorrar->file_size = 0;

		log_info(fuse_logger, "Directorio -> Nodo: %d | borrado correctamente.", nodoABorrarPosicion);
		return 0;
	}

	if(tablaDeNodos[nodoABorrarPosicion].file_size != 0){
		borrar_contenido(nodoABorrarPosicion, tablaDeNodos[nodoABorrarPosicion].file_size);
	}

	nodoABorrar->state = 0;
	nodoABorrar->file_size = 0;

	log_info(fuse_logger, "Archivo -> Nodo: %d | borrado correctamente.", nodoABorrarPosicion);

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

	pthread_mutex_lock(&s_tablaDeNodos);

	while(tablaDeNodos[currNode].state != 0 && currNode < MAX_NUMBER_OF_FILES){
		currNode++;
	}

	if (currNode >= MAX_NUMBER_OF_FILES)
	{
		log_error(fuse_logger, "Tabla de nodos llena, no hay lugar disponible para un nuevo nodo.");

		for(int y=0; y<dimListaSpliteada; y++) // libero cada integrante de la matriz
			free(listaSpliteada[y]);

		free(listaSpliteada);
		return EDQUOT;
	}

	if(dimListaSpliteada > 1){
		char* pathduplicado = strdup(path);
		parentDirPath = dirname(pathduplicado);
		nodoPadre = determine_nodo(parentDirPath);
		free(pathduplicado);
	}

	int respuesta = validar_si_ya_existe_otro(path, fileName);

	if(respuesta != 0){
		free(listaSpliteada);
		return respuesta;
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
	nodoVacio->create_date = time(NULL);
	nodoVacio->modify_date = time(NULL);

	pthread_mutex_unlock(&s_tablaDeNodos);

	if(tipoDeArchivo == 1){
		int nuevo_nodo_vacio = get_bloque_vacio();

		agregar_nodo(nodoVacio, nuevo_nodo_vacio);

		GBlock *bloqueDeDatos = disco + nuevo_nodo_vacio;

		memset(bloqueDeDatos->bytes, 0, BLOQUE_SIZE);
	}

	log_info(fuse_logger, "Nodo creado -> Estado: %d, Nombre: %s, Bloque padre: %d", tipoDeArchivo, fileName, nodoVacio->parent_dir_block);

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
		log_info(fuse_logger, "Nodo raiz encontrado.");
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
		log_error(fuse_logger, "El Path ingresado no corresponde con un nodo existente.");
		arg[0].type = VAR_UINT32;
		arg[0].size = sizeof(uint32_t);
		arg[0].value.val_u32 = -ENOENT;
		fsend.type = -1;
	}else{
		log_info(fuse_logger, "Nodo encontrado -> Posicion: %d.", nodoBuscado);
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
	unsigned int nodoDelArchivo, bloque_punteros, num_bloque_datos;
	unsigned int bloque_a_buscar;
	GFile *node;
	ptrGBloque *pointer_block;
	char *data_block;
	size_t tamanio = size;
	int respuesta;
	void* cursor = buffer;

	nodoDelArchivo = determine_nodo(path);

	node = tablaDeNodos + nodoDelArchivo;

	if(node->file_size <= offset){
		log_error(fuse_logger, "Se intenta leer un offset mayor o igual que el tamanio de archivo. Se retorna size 0. File: %s, Size: %d", path, node->file_size);
		respuesta = 0;
		goto finalizar;
	} else if (node->file_size <= (offset + size)){
		size = node->file_size - offset;
		tamanio = size;
	}

	for (bloque_punteros = 0; bloque_punteros < BLKINDIRECT; bloque_punteros++){

		// Chequea el offset y lo acomoda para leer lo que realmente necesita
		if (offset > BLOQUE_SIZE * 1024){
			offset -= (BLOQUE_SIZE * 1024);
			continue;
		}

		bloque_a_buscar = node->indirect_blocks_array[bloque_punteros];	// Ubica el nodo de punteros a nodos de datos, es relativo al nodo 0: Header.
		punterosBloquesDatos *bloqueDePunterosDatos = (punterosBloquesDatos *) (disco + bloque_a_buscar); // Acomoda el nodo de punteros a nodos de datos, es relativo al bloque de datos.
		ptrGBloque bloqueDeDatos = bloqueDePunterosDatos->punteros_a_bloques[bloque_a_buscar];// Apunta al nodo antes ubicado. Lo utiliza para saber de donde leer los datos.
		// Recorre el bloque de punteros correspondiente.
		for (num_bloque_datos = 0; num_bloque_datos < 1024; num_bloque_datos++){
		// Chequea el offset y lo acomoda para leer lo que realmente necesita
			if (offset >= BLOQUE_SIZE){
				offset -= BLOQUE_SIZE;
				continue;
			}

			bloque_a_buscar = bloqueDePunterosDatos->punteros_a_bloques[num_bloque_datos]; 	// Ubica el nodo de datos correspondiente. Relativo al nodo 0: Header.
			data_block = (char *) (disco + bloque_a_buscar)->bytes; // Acomoda el nodo, haciendolo relativo al bloque de datos.

			// Corre el offset hasta donde sea necesario para poder leer lo que quiere.
			if (offset > 0){
				data_block += offset;
				offset = 0;
			}

			if (tamanio < BLOQUE_SIZE){
				memcpy(cursor, data_block, tamanio);
				cursor += tamanio;
				tamanio = 0;
				break;
			} else {
				memcpy(cursor, data_block, BLOQUE_SIZE);
				tamanio -= BLOQUE_SIZE;
				cursor += BLOQUE_SIZE;
				if (tamanio == 0) break;
			}

		}

		if (tamanio == 0) break;
	}
	respuesta = size;

	finalizar:
	log_trace(fuse_logger, "Terminada lectura.");
	return respuesta;
}

int get_bloque_vacio(){
	pthread_mutex_lock(&s_bitmap);

	int bitActual = bloqueInicioBloquesDeDatos - 1;
	int bitsTotales = bitarray_get_max_bit(bitmap);

	while(bitarray_test_bit(bitmap, bitActual) != 0 && bitActual < bitsTotales){
			bitActual++;
	}

	if(bitActual == bitsTotales){
		log_error(fuse_logger, "No hay bloques de datos disponibles.");
		return -1;
	}

	bitarray_set_bit(bitmap, bitActual);
	pthread_mutex_unlock(&s_bitmap);

	log_debug(fuse_logger, "Bloque vacio disponible en posicion: %d.", bitActual + 1);

	msync(disco, diskSize, MS_SYNC);
	return bitActual;
}

int cantidad_bloques_libres(){
	pthread_mutex_lock(&s_bitmap);
	int contador = 0;
	int bitInicial = bloqueInicioBloquesDeDatos - 1;
	int bitsTotales = bitarray_get_max_bit(bitmap);

	for(int i = bitInicial; i < bitsTotales; i++){
		if(bitarray_test_bit(bitmap, i) == 0){
			contador++;
		}
	}
	pthread_mutex_unlock(&s_bitmap);

	return contador;

}

int agregar_nodo(GFile *file_data, int numeroNodo){
	int node_pointer_number, position;
	size_t tam = file_data->file_size;
	int new_pointer_block;
	punterosBloquesDatos* nodo_punteros;

	pthread_mutex_lock(&s_tablaDeNodos);

	// Ubica el ultimo nodo escrito y se posiciona en el mismo.
	setear_posicion(&node_pointer_number, &position, 0, tam);

	if((node_pointer_number == BLKINDIRECT-1) & (position == PUNTEROS_A_BLOQUES_DATOS-1)) return -ENOSPC;

	// Si es el primer nodo del archivo y esta escrito, debe escribir el segundo.
	// Se sabe que el primer nodo del archivo esta escrito siempre que el primer puntero a bloque punteros del nodo sea distinto de 0 (file_data->blk_indirect[0] != 0)
	// ya que se le otorga esa marca (=0) al escribir el archivo, para indicar que es un archivo nuevo.
	if ((file_data->indirect_blocks_array[node_pointer_number] != 0)){
		if (position == 1024) {
			position = 0;
			node_pointer_number++;
		}
	}
	// Si es el ultimo nodo en el bloque de punteros, pasa al siguiente
	if (position == 0){
		new_pointer_block = get_bloque_vacio();
		if(new_pointer_block < 0) return new_pointer_block; /* Si sucede que sea menor a 0, contendra el codigo de error */

		//pthread_mutex_lock(&s_tablaDeBloquesDeDatos);
		GBlock *bloqueDeDatos = disco + new_pointer_block;

		memset(bloqueDeDatos->bytes, 0, BLOQUE_SIZE);
		//pthread_mutex_unlock(&s_tablaDeBloquesDeDatos);

		//pthread_mutex_lock(&s_tablaDeNodos);
		file_data->indirect_blocks_array[node_pointer_number] = new_pointer_block;
		// Cuando crea un bloque, settea al siguente como 0, dejando una marca.
		file_data->indirect_blocks_array[node_pointer_number + 1] = 0;
		//pthread_mutex_unlock(&s_tablaDeNodos);

	} else {
		new_pointer_block = file_data->indirect_blocks_array[node_pointer_number]; //Se usa como auxiliar para encontrar el numero del bloque de punteros
	}

	// Ubica el nodo de punteros
	nodo_punteros = (punterosBloquesDatos *) (disco + new_pointer_block);


	// Hace que dicho puntero, en la posicion ya obtenida, apunte al nodo indicado.
	nodo_punteros->punteros_a_bloques[position] = numeroNodo;

	pthread_mutex_unlock(&s_tablaDeNodos);

	msync(disco, diskSize, MS_SYNC);

	return 0;

}

int escribir_archivo (char* buffer, char* path, size_t size, uint32_t offset){
	int nodoDelArchivo = determine_nodo(path);
	int nodo_libre_nuevo;
	GFile *node;
	char *data_block;
	size_t tam = size, file_size, space_in_block, offset_in_block = offset % BLOQUE_SIZE;
	off_t off = offset;
	int *n_pointer_block = malloc(sizeof(int)), *n_data_block = malloc(sizeof(int));
	ptrGBloque *pointer_block;
	int respuesta = size;

	unsigned long tamanio_maximo_teorico = (unsigned long) BLKINDIRECT * (unsigned long) PUNTEROS_A_BLOQUES_DATOS * (unsigned long) BLOQUE_SIZE;

	pthread_mutex_t *mutex;

	if(dictionary_has_key(diccionarioDeMutex, path)){
		mutex = dictionary_get(diccionarioDeMutex, path);
	}
	else{
		mutex = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(mutex, NULL);
		dictionary_put(diccionarioDeMutex, path, mutex);
	}

	pthread_mutex_lock(mutex);

	// Ubica el nodo correspondiente al archivo
	node = tablaDeNodos + nodoDelArchivo;
	file_size = node->file_size;

	uint32_t tamanioMaximoTeorico = tamanio_maximo_teorico;

	if ((file_size + size) >= tamanioMaximoTeorico){
		free(n_pointer_block);
		free(n_data_block);
		return -EFBIG;
	}

	// Guarda tantas veces como sea necesario, consigue nodos y actualiza el archivo.
	while (tam != 0){

		// Actualiza los valores de espacio restante en bloque.
		space_in_block = BLOQUE_SIZE - (file_size % BLOQUE_SIZE);
		if (space_in_block == BLOQUE_SIZE) (space_in_block = 0); // Porque significa que el bloque esta lleno.
		if (file_size == 0) space_in_block = BLOQUE_SIZE; /* Significa que el archivo esta recien creado y ya tiene un bloque de datos asignado */

		// Si el offset es mayor que el tamanio del archivo mas el resto del bloque libre, significa que hay que pedir un bloque nuevo
		// file_size == 0 indica que es un archivo que recien se comienza a escribir, por lo que tiene un tratamiento distinto (ya tiene un bloque de datos asignado).
		if ((off >= (file_size + space_in_block)) & (file_size != 0)){

			// Si no hay espacio en el disco, retorna error.
			if (cantidad_bloques_libres() == 0){
				free(n_pointer_block);
				free(n_data_block);
				return -ENOSPC;
			}

			// Obtiene un bloque libre para escribir.
			nodo_libre_nuevo = get_bloque_vacio();
			if (nodo_libre_nuevo < 0) goto finalizar;

			// Agrega el nodo al archivo.
			respuesta = agregar_nodo(node, nodo_libre_nuevo);
			if (respuesta != 0) goto finalizar;

			// Lo relativiza al data block.
			data_block = (char *) (disco + nodo_libre_nuevo)->bytes;

			// Actualiza el espacio libre en bloque.
			space_in_block = BLOQUE_SIZE;

		} else {
			// Ubica a que nodo le corresponderia guardar el dato
			setear_posicion(n_pointer_block, n_data_block, file_size, off);

			//Ubica el nodo a escribir.
			*n_pointer_block = node->indirect_blocks_array[*n_pointer_block];
			punterosBloquesDatos *bloqueDePunterosDatos = (punterosBloquesDatos *) (disco + *n_pointer_block);
			ptrGBloque bloqueDeDatos = bloqueDePunterosDatos->punteros_a_bloques[*n_data_block];
			data_block = (char *) (disco + bloqueDeDatos)->bytes;
		}

		// Escribe en ese bloque de datos.
		if (tam >= BLOQUE_SIZE){
			memcpy(data_block, buffer, BLOQUE_SIZE);
			if ((node->file_size) <= (off)) file_size = node->file_size += BLOQUE_SIZE;
			buffer += BLOQUE_SIZE;
			off += BLOQUE_SIZE;
			tam -= BLOQUE_SIZE;
			offset_in_block = 0;
		} else if (tam <= space_in_block){ /*Hay lugar suficiente en ese bloque para escribir el resto del archivo */
			memcpy(data_block + offset_in_block, buffer, tam);
			if (node->file_size <= off) file_size = node->file_size += tam;
			else if (node->file_size <= (off + tam)) file_size = node->file_size += (off + tam - node->file_size);
			tam = 0;
		} else { /* Como no hay lugar suficiente, llena el bloque y vuelve a buscar uno nuevo */
			memcpy(data_block + offset_in_block, buffer, space_in_block);
			file_size = node->file_size += space_in_block;
			buffer += space_in_block;
			off += space_in_block;
			tam -= space_in_block;
			offset_in_block = 0;
		}

	}

	node->modify_date= time(NULL);

	respuesta = size;

	finalizar:

	pthread_mutex_unlock(mutex);

	msync(disco, diskSize, MS_SYNC);

	free(n_pointer_block);
	free(n_data_block);

	return respuesta;

}
