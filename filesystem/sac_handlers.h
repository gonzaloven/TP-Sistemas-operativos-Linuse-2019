#ifndef FILESYSTEM_SAC_HANDLERS_H_
#define FILESYSTEM_SAC_HANDLERS_H_

#include <stdlib.h>
#include <string.h>
#include "sac_servidor.h"
#include <commons/string.h>

char **splitPath(char *path, int *size);

int esElNodoBuscado(int nodo, char* filename, ptrGBloque nodoPadre);

int buscar_nodo_por_nombre(char *filenameBuscado, ptrGBloque nodoPadre);

int determine_nodo(char *path);

int tamano_malloc_list(t_list*);

void lista_a_string(t_list* lista, char** string);

int largoListaString(char** lista);

int borrar_directorio (GFile* nodoABorrar, int nodoABorrarPosicion);

int borrar_archivo(GFile* nodoABorrar, int nodoABorrarPosicion);

void borrar_contenido(int nodoABorrarPosicion, int tamanio);

Function enviar_respuesta_basica(int respuesta, FuncType tipoFuncion);

int crear_nuevo_nodo (char* path, int tipoDeArchivo);

void completar_lista_con_fileNames(t_list* listaDeArchivos, ptrGBloque nodoPadre);

Function validarSiExiste(char* path, FuncType tipoFuncion);

Function retornar_error(Function funcionAEnviar);

int leer_archivo(char* buffer, char *path, size_t size, uint32_t offset);


#endif /* FILESYSTEM_SAC_HANDLERS_H_ */
