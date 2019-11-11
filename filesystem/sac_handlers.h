#ifndef FILESYSTEM_SAC_HANDLERS_H_
#define FILESYSTEM_SAC_HANDLERS_H_

#include <stdlib.h>
#include <string.h>
#include "sac_servidor.h"

char **splitPath(char *path, int *size);

int esElNodoBuscado(int nodo, char* filename, ptrGBloque nodoPadre);

int buscar_nodo_por_nombre(char *filenameBuscado, ptrGBloque nodoPadre);

ptrGBloque determine_nodo(char *path);


#endif /* FILESYSTEM_SAC_HANDLERS_H_ */
