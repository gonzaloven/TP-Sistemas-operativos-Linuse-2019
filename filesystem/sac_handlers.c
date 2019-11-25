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
			return -1;
		}else{
			nodoUltimoPadre += bloqueInicioTablaDeNodos;
		}
	}

	free(listaSpliteada);
	return nodoUltimoPadre - bloqueInicioTablaDeNodos;
}

int tamano_malloc_list(t_list* lista){
	int tamano = 0;
	char* nombre;
	for(int i=0; i < list_size(lista); i++){
		nombre = (char*)list_get(lista, i);
		tamano+= strlen((char*)list_get(lista, i)) + 1;
	}

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
		return EDQUOT;
	}

	time_t tiempoAhora;
	uint64_t timestamp;
	tiempoAhora = time(0);
	timestamp = (uint64_t) tiempoAhora;

	if(dimListaSpliteada > 1){
		parentDirPath = dirname(strdup(path));
		nodoPadre = determine_nodo(parentDirPath);
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
