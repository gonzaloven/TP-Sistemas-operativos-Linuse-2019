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
	int resultado = -1;

	for(int i=0; i< MAX_NUMBER_OF_FILES; i++){
		if(!esElNodoBuscado(i, filenameBuscado, nodoPadre)){
			i++;
		}else{
			resultado = i;
			break;
		}
	}

	return resultado;
}

int determine_nodo(char *path, int inicioTablaDeNodos){
	ptrGBloque nodoUltimoPadre = inicioTablaDeNodos;
	char *filenameBuscado;

	int dimListaSpliteada, i;
	char **listaSpliteada;

	listaSpliteada = splitPath(path, &dimListaSpliteada);

	char* directorio = malloc(strlen(path) + 1);
	char* pathOriginal = malloc(strlen(path) + 1);
	char* filename = malloc(strlen(path) + 1);

	memcpy(pathOriginal, path, strlen(path) + 1);
	memcpy(directorio, dirname(path), strlen(path) + 1);
	memcpy(filename, basename(pathOriginal), strlen(basename(pathOriginal)) + 1);

	if(!strcmp(directorio, "/")){
		filenameBuscado = filename;
		nodoUltimoPadre = buscar_nodo_por_nombre(filenameBuscado, 0);
		return nodoUltimoPadre;
	}

	for(i=0 ; i<dimListaSpliteada; i++){

		filenameBuscado = listaSpliteada[i];

		nodoUltimoPadre = buscar_nodo_por_nombre(filenameBuscado, nodoUltimoPadre);

		if(nodoUltimoPadre == -1){
			return -1;
		}
	}

	free(directorio);
	free(pathOriginal);
	free(filename);
	free(listaSpliteada);
	return nodoUltimoPadre;
}

int tamano_malloc_list(t_list* lista){
	int tamano = 0;
	for(int i=0; i < list_size(lista); i++){
		tamano+= strlen((char*)list_get(lista, i));
	}

	return tamano + 1; // +1 por el \0
}

void lista_a_string(t_list* lista, char** string){
	*string = malloc(tamano_malloc_list(lista));

	if(list_size(lista) > 0)
		strcpy(*string, (char*)list_get(lista, 0));

	for(int i=1; i < list_size(lista); i++){
		strcat(*string, "/");
		strcat(*string, (char*)list_get(lista, i));
	}
}
