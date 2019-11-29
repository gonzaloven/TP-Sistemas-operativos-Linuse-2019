#ifndef FILESYSTEM_SAC_HANDLERS_H_
#define FILESYSTEM_SAC_HANDLERS_H_

#include <stdlib.h>
#include <string.h>
#include "sac_servidor.h"
#include <commons/string.h>
#include <math.h>

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

int escribir_archivo (char* buffer, char *path, size_t size, uint32_t offset);

int agregar_nodo(GFile *file_data, int numeroNodo);

int setear_posicion (int *pointer_block, int *data_block, size_t size, off_t offset);

int cantidad_bloques_libres();

int get_bloque_vacio();


#endif /* FILESYSTEM_SAC_HANDLERS_H_ */

/*int asignar_en_disponible(GFile* archivo, ptrGBloque punteroIndirecto, ptrGBloque bloqueNuevo){
	int posicion = 0;
	ptrGBloque bloqueDePunteros = archivo->indirect_blocks_array[punteroIndirecto];

	punterosBloquesDatos *bloqueDePunterosDatos = (punterosBloquesDatos *) (disco + bloqueDePunteros);

	while(bloqueDePunterosDatos->punteros_a_bloques[posicion] != 0 && posicion < PUNTEROS_A_BLOQUES_DATOS){
		posicion++;
	}

	ptrGBloque bloqueVacioParaDatos = get_bloque_vacio();

	bloqueDePunterosDatos->punteros_a_bloques[0] = bloqueVacioParaDatos;
	ocuparBloque(bloqueVacioParaDatos);
}

int esta_lleno(ptrGBloque bloqueDePunteros){
	punterosBloquesDatos *bloqueDePunterosDatos = (punterosBloquesDatos *) (disco + bloqueDePunteros);
	int posicion = 0;

	while(bloqueDePunterosDatos->punteros_a_bloques[posicion] != 0 && posicion < PUNTEROS_A_BLOQUES_DATOS){
		posicion++;
	}

	if(posicion == PUNTEROS_A_BLOQUES_DATOS){
		return 1;
	}else{
		return 0;
	}
}

int agregar_bloque(GFile *archivoAAgregar, int numeroBloqueNuevo){
	int posicionActual = 0;
	int posicionActualPunteros = 0;

	while(archivoAAgregar->indirect_blocks_array[posicionActual] != 0 && posicionActual < BLKINDIRECT){
		ptrGBloque bloqueDePunterosDatosActual = archivoAAgregar->indirect_blocks_array[posicionActual];

		if(esta_lleno(bloqueDePunterosDatosActual)){
			posicionActual++;
		}else{
			asignar_en_disponible(archivoAAgregar, posicionActual, numeroBloqueNuevo);
			return 0;
		}
	}

	if(posicionActual == BLKINDIRECT){
		return -1;
	}

	archivoAAgregar->indirect_blocks_array[posicionActual] = numeroBloqueNuevo;
	ocuparBloque(numeroBloqueNuevo);

	GBlock *bloqueDePunterosDatosNuevo = disco + numeroBloqueNuevo;
	punterosBloquesDatos *bloqueDePunterosDatos = (punterosBloquesDatos *) (disco + numeroBloqueNuevo);

	memset(bloqueDePunterosDatosNuevo, 0, BLOQUE_SIZE);

	ptrGBloque bloqueVacioParaDatos = get_bloque_vacio();

	bloqueDePunterosDatos->punteros_a_bloques[0] = bloqueVacioParaDatos;
	ocuparBloque(bloqueVacioParaDatos);

	return 0;
}*/
