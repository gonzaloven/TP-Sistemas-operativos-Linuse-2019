#ifndef FILESYSTEM_SAC_HANDLERS_H_
#define FILESYSTEM_SAC_HANDLERS_H_

#include <stdlib.h>
#include <string.h>
#include "sac_servidor.h"

char **splitPath(char *path, int *size);

int esElNodoBuscado(int nodo, char* filename, ptrGBloque nodoPadre);

int buscar_nodo_por_nombre(char *filenameBuscado, ptrGBloque nodoPadre);

int determine_nodo(char *path);

int tamano_malloc_list(t_list*);

void lista_a_string(t_list* lista, char** string);


#endif /* FILESYSTEM_SAC_HANDLERS_H_ */
