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


#endif /* FILESYSTEM_SAC_HANDLERS_H_ */

/*
	//t_Getatrr* p_param = getatrr_decode(payload);
	log_info(logger, "Reading: Path: %s - Size: %d - Offset %d",
	//		p_param->path, p_param->size, p_param->offset);
	(void) fi;
	unsigned int node_n, bloque_punteros, num_bloque_datos;
	unsigned int bloque_a_buscar; // Estructura auxiliar para no dejar choclos
	struct sac_server_gfile *node;
	ptrGBloque *pointer_block;
	char *data_block;
	size_t tam = p_param->size;
	int res;

	//node_n = determine_node(p_param->path);

	if (node_n == -1) return -ENOENT; // contemplo que exista el nodo

	node = node_table_start;

	// Ubica el nodo correspondiente al archivo
	node = &(node[node_n]);

	pthread_rwlock_rdlock(&rwlock); //Toma un lock de lectura.
	log_lock_trace(logger, "Read: Toma lock lectura. Cantidad de lectores: %d", rwlock.__data.__nr_readers);

	if(node->file_size <= p_param->offset){
		log_error(logger, "Se intenta leer un offset mayor o igual que el tamanio de archivo. Se retorna size 0. File: %s, Size: %d", p_param->path, node->file_size);
		// TODO enviar error al cliente
		res = 0;
		goto finalizar;
	} else if (node->file_size <= (p_param->offset + size)){
		tam = size = ((node->file_size)-(offset));
		// TODO enviar error al cliente
		log_error(logger, "Se intenta leer una posicion mayor o igual que el tamanio de archivo. Se retornaran %d bytes. File: %s, Size: %d", p_param->size, p_param->path, node->file_size);
	}
	// Recorre todos los punteros en el bloque de la tabla de nodos
	for (bloque_punteros = 0; bloque_punteros < BLKINDIRECT; bloque_punteros++){

		// Chequea el offset y lo acomoda para leer lo que realmente necesita
		if (offset > BLOCKSIZE * 1024){
			offset -= (BLOCKSIZE * 1024);
			continue;
		}

		bloque_a_buscar = (node->blk_indirect)[bloque_punteros];	// Ubica el nodo de punteros a nodos de datos, es relativo al nodo 0: Header.
		bloque_a_buscar -= (GFILEBYBLOCK + BITMAP_BLOCK_SIZE + NODE_TABLE_SIZE);	// Acomoda el nodo de punteros a nodos de datos, es relativo al bloque de datos.
		pointer_block =(ptrGBloque *) &(data_block_start[bloque_a_buscar]);		// Apunta al nodo antes ubicado. Lo utiliza para saber de donde leer los datos.
		// Recorre el bloque de punteros correspondiente.
		for (num_bloque_datos = 0; num_bloque_datos < 1024; num_bloque_datos++){
		// Chequea el offset y lo acomoda para leer lo que realmente necesita
			if (offset >= BLOCKSIZE){
				offset -= BLOCKSIZE;
				continue;
			}

			bloque_a_buscar = pointer_block[num_bloque_datos]; 	// Ubica el nodo de datos correspondiente. Relativo al nodo 0: Header.
			bloque_a_buscar -= (GFILEBYBLOCK + BITMAP_BLOCK_SIZE + NODE_TABLE_SIZE);	// Acomoda el nodo, haciendolo relativo al bloque de datos.
			data_block = (char *) &(data_block_start[bloque_a_buscar]);

			// Corre el offset hasta donde sea necesario para poder leer lo que quiere.
			if (offset > 0){
				data_block += offset;
				offset = 0;
			}

			if (tam < BLOCKSIZE){
				memcpy(buf, data_block, tam);
				buf = &(buf[tam]);
				tam = 0;
				break;
			} else {
				memcpy(buf, data_block, BLOCKSIZE);
				tam -= BLOCKSIZE;
				buf = &(buf[BLOCKSIZE]);
				if (tam == 0) break;
			}

		}

		if (tam == 0) break;
	}
	res = size;

	finalizar:
	pthread_rwlock_unlock(&rwlock); //Devuelve el lock de lectura.
	log_lock_trace(logger, "Read: Libera lock lectura. Cantidad de lectores: %d", rwlock.__data.__nr_readers);
	log_trace(logger, "Terminada lectura.");
	return res;
	*/
