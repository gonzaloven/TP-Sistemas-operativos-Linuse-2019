#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stddef.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#define MUSE_LOG_PATH "../logs/muse.log"

/* Estructura ppal de cada programa: nos dice para cada pid su numero de tabla de segmento*/
typedef struct program_s
{
	uint16_t pid;
	t_list *segment_table;
}program;

/* Estructura de Metadata, una por cada segmento */
typedef struct HeapMetadata 
{
	uint32_t size;
	bool is_free;
}heap_metadata;

typedef struct frame_s
{
	heap_metadata metadata;
	void *data;
}frame;

/*Estructura de una pagina: sabemos si está presente, su numero y a que frame apunta */
typedef struct page_s
{
	bool is_present; //sirve para páginas compartidas
	uint16_t page_num;
	frame *fr; 
}page;

/*
 * Estructura del segmento, sabemos su espacio libre
 * un puntero a su base, uno a su limite y uno a su tabla de páginas
 */
typedef struct segment_s
{
	//segment_lock;
	uint16_t free_size;
	uint32_t *base;
	uint32_t *limit;
	t_list *page_table;
}segment;

/**
 * @param Salen del archivo config
 */
void muse_main_memory_init(int memory_size, int page_size);

void muse_main_memory_stop();

/**
 * Busca el process_id en program_list
 * @return -1 if no está, else devuelve el nro de programa en program_list
 */
int search_program(uint32_t pid);

/**
 * @return -1 if no hay espacio libre, else devuelve la base para el tamaño pedido
 */
int has_segment_with_free_space(program *prog, int size);

/**
 * @return NULL if error, page if OK 
 */
page *find_free_page();

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
uint32_t memory_get(void *dst, uint32_t src, size_t n, uint32_t pid);

uint32_t memory_cpy(uint32_t dst, void *src, int n, uint32_t pid);

uint32_t memory_map(char *path, size_t length, int flags, uint32_t pid);

uint32_t memory_sync(uint32_t addr, size_t len, uint32_t pid);

int memory_unmap(uint32_t dir, uint32_t pid);

uint32_t muse_add_segment_to_program(program *p, int segm_size, uint32_t pid);

#endif
