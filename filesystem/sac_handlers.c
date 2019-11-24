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

int determine_nodo(char *path, int inicioTablaDeNodos){
	ptrGBloque nodoUltimoPadre = inicioTablaDeNodos;
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
			nodoUltimoPadre = buscar_nodo_por_nombre(filenameBuscado, 0) + inicioTablaDeNodos;
			continue;
		}

		filenameBuscado = listaSpliteada[i];

		nodoUltimoPadre = buscar_nodo_por_nombre(filenameBuscado, nodoUltimoPadre);

		if(nodoUltimoPadre == -1){
			return -1;
		}else{
			nodoUltimoPadre += inicioTablaDeNodos;
		}
	}

	free(listaSpliteada);
	return nodoUltimoPadre - inicioTablaDeNodos;
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
