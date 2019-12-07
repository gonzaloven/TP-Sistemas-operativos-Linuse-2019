#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stddef.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

//#define MUSE_LOG_PATH "/home/utnso/git/tp-2019-2c-Los-Trapitos/logs/muse.log"

#define MUSE_LOG_PATH "/home/utnso/tp-2019-2c-Los-Trapitos/logs/muse.log"

/**
 * @struct program_s   
 * @brief Estructura ppal de cada programa 
 * tiene como variables su pid y su lista de tabla de segmentos
 */
typedef struct program_s
{
	uint16_t pid;
	t_list *segment_table;	
}program;

/**
 * @struct HeapMetadata
 * @brief Estructura de Metadata, una por cada segmento y una por cada frame
 * tiene como variables uint32_t size y bool is_free
 */
typedef struct HeapMetadata 
{
	uint32_t size; 
	bool is_free;
}__attribute__((packed)) heap_metadata;


/*
//Un frame puede tener varios bloques, un mismo bloque puede ser compartido por varios frames
typedef struct bloque_s
{
	heap_metadata metadata;
	void *data;
}bloque;
*/

/*Estructura de una pagina: sabemos si está presente, su numero y a que frame apunta */
typedef struct page_s
{
	bool is_modify;
	bool is_used;
	bool is_present; //1 si está en memoria principal, 0 en disco
	//uint16_t page_num;
	void *fr; //si is_present = 1, indica el frame de MP. Caso contrario indicará la posición de Swap
}page;

/*
 * Estructura del segmento, sabemos su espacio libre
 * un puntero a su base, uno a su limite y uno a su tabla de páginas
 */

typedef struct{
	char* pathArchivo;
	void* archivo;
	t_list* tabla_paginas;
	t_list* programas;
}__attribute__((packed)) archivoMMAP;

typedef struct segment_s
{
	bool is_heap; //(1 is common_segment, 0 is mmap)
	uint32_t base; //base lógica
	uint32_t limit;
	t_list *page_table;
	archivoMMAP* archivo_mapeado;
	int tipo_map; //0 privado, 1 compartido
	int tam_archivo_mmap;

}segment;

/**
 * @param memory_size y @param page_size salen del archivo config
 */
void muse_main_memory_init(int memory_size, int page_size, int swap_size);

void muse_main_memory_stop();

int number_of_free_frames();

void metricas_por_socket_conectado(uint32_t pid);

int busca_segmento(program *prog,uint32_t va);

void* obtener_data_marco_heap(page* pagina);

void* obtener_data_marco_mmap(segment* segmento,page* pagina,int nro_pagina);

/**
 * Busca el process_id en program_list
 * @return -1 if no está, else devuelve el nro de programa en program_list
 */
int search_program(uint32_t pid);

/**
 * @return -1 si no hay espacio libre, sino devuelve la base para el tamaño pedido
 */
int segment_with_free_space(program *prog, int size);

/**
 * Se busca un frame libre, y devuelve la página que apunta a ese frame
 * @return NULL if nos quedamos sin frames, page if OK
 */
page *find_free_frame();

/** Allocates memory in UCM
 * @param size size of the memory to allocate 
 * @return virtual address
 */
uint32_t memory_malloc(int size, uint32_t pid);

/**
 * Frees a memory allocation
 * @param virtual_address: the address of memory to free
 * @return -1 if SEGFAULT, 0 if ok
 */
uint8_t memory_free(uint32_t virtual_address, uint32_t pid);

/**
* Copia una cantidad 'n' de bytes desde una posición de memoria de MUSE a una 'dst' local.
* @param dst Posición de memoria local con tamaño suficiente para almacenar 'n' bytes.
* @param src Posición de memoria de MUSE de donde leer los 'n' bytes.
* @param n Cantidad de bytes a copiar.
* @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
*/
void* memory_get(void *dst, uint32_t src, size_t numBytes, uint32_t pid);

/**
* Copia una cantidad `n` de bytes desde una posición de memoria local a una `dst` en MUSE.
* @param dst Posición de memoria de MUSE con tamaño suficiente para almacenar `n` bytes.
* @param src Posición de memoria local de donde leer los `n` bytes.
* @param n Cantidad de bytes a copiar.
* @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
*/
uint32_t memory_cpy(uint32_t dst, void *src, int n, uint32_t pid);

/**
* Devuelve un puntero a una posición mappeada de páginas por una cantidad `length` de bytes el archivo del `path` dado.
* @param path Path a un archivo en el FileSystem de MUSE a mappear.
* @param length Cantidad de bytes de memoria a usar para mappear el archivo.
* @param flags
*          MAP_PRIVATE     Solo un proceso/hilo puede mappear el archivo.
*          MAP_SHARED      El segmento asociado al archivo es compartido.
* @return Retorna la posición de memoria de MUSE mappeada. Si hay error, retorna el valor
* MAP_FAILED (that is, (void *) -1). 
* @note: Si `length` sobrepasa el tamaño del archivo, toda extensión deberá estar llena de "\0".
* @note: muse_free no libera la memoria mappeada. @see muse_unmap
*/
uint32_t memory_map(char *path, size_t length, int flags, uint32_t pid);

/**
* Descarga una cantidad `len` de bytes y lo escribe en el archivo en el FileSystem.
* @param addr Dirección a memoria mappeada.
* @param len Cantidad de bytes a escribir.
* @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
* @note Si `len` es menor que el tamaño de la página en la que se encuentre, se deberá escribir la página completa.
*/
uint32_t memory_sync(uint32_t addr, size_t len, uint32_t pid);

/**
* Borra el mappeo a un archivo hecho por muse_map.
* @param dir Dirección a memoria mappeada.
* @note Esto implicará que todas las futuras utilizaciones de direcciones basadas en `dir` serán accesos inválidos.
* @note Solo se deberá cerrar el archivo mappeado una vez que todos los hilos hayan liberado la misma cantidad de muse_unmap que muse_map.
* @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
*/
int memory_unmap(uint32_t dir, uint32_t pid);

int proxima_metadata_libre(int dirLogica, segment* segmentoActual);

segment* ultimo_segmento_programa(program *prog);

int dame_nro_frame_reemplazado();

int se_hace_la_vistima(page* pag, int nro_de_pag, int nro_de_segmento);

int mandar_al_archivo_swap_toda_la_pagina_que_esta_en(int nro_frame);

int frame_swap_libre();

void liberar_frame_swap(void* frame);

void destroy_program_list_elements(program* prog);

void destroy_segment_table_elements(segment* seg);

void destroy_page_table_elements(page* pag);

#endif
