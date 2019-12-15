#ifndef LIBMUSE_H_
#define LIBMUSE_H_

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "network.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <commons/config.h>
#include <sys/mman.h> //for mmap() & munmap()
#include <signal.h>

#include <stdlib.h>

#define LIBMUSE_CONFIG_PATH "/home/utnso/workspace/tp-2019-2c-Los-Trapitos/configs/libmuse.config"

t_config *config;

/**
* Inicializa la biblioteca de MUSE.
* @param id El Process (o Thread) ID para identificar el caller en MUSE.
* @return Si pasa un error, retorna -1. Si se inicializó correctamente, retorna 0.
* @see Para obtener el id de un proceso pueden usar getpid() de la lib POSIX (unistd.h)
* @note Debido a la naturaleza centralizada de MUSE, esta función deberá 
* definir el ID del proceso/hilo según "IP-ID".
*/
int muse_init(int id, char* ip, int puerto);

/**
* Cierra la biblioteca de MUSE.
*/
void muse_close();

/**
* Reserva una porción de memoria contígua de tamaño `tam`.
* @param tam La cantidad de bytes a reservar.
* @return La dirección de la memoria reservada.
*/
uint32_t muse_alloc(uint32_t tam);

/**
* Libera una porción de memoria reservada.
* @param dir La dirección de la memoria a reservar.
*/
void muse_free(uint32_t dir);

/**
* Copia una cantidad `n` de bytes desde una posición de memoria de MUSE a una `dst` local.
* @param dst Posición de memoria local con tamaño suficiente para almacenar `n` bytes.
* @param src Posición de memoria de MUSE de donde leer los `n` bytes.
* @param n Cantidad de bytes a copiar.
* @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
*/
int muse_get(void* dst, uint32_t src, size_t n);

/**
* Copia una cantidad `n` de bytes desde una posición de memoria local a una `dst` en MUSE.
* @param dst Posición de memoria de MUSE con tamaño suficiente para almacenar `n` bytes.
* @param src Posición de memoria local de donde leer los `n` bytes.
* @param n Cantidad de bytes a copiar.
* @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
*/
int muse_cpy(uint32_t dst, void* src, int n);

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
* @example	
	uint32_t map = muse_map("hola.txt", filesize, MAP_PRIVATE);		
	opens the "hola.txt" file in RDONLY mode
	map is a position _mapped_ of pages of a given 'filesize' of the "hola.txt" file  
	muse_map basically is just putting the hola.txt file in memory 
*/
uint32_t muse_map(char *path, size_t length, int flags);

/**
* Descarga una cantidad `len` de bytes y lo escribe en el archivo en el FileSystem.
* @param addr Dirección a memoria mappeada.
* @param len Cantidad de bytes a escribir.
* @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
* @note Si `len` es menor que el tamaño de la página en la que se encuentre, se deberá escribir la página completa.
* @example	
	remember we got uint32_t map = muse_map("hola.txt", filesize, MAP_PRIVATE);		
	so let's imagine we do the following:
		muse_sync(map, 200)
	so we're basicly just writing 200 bytes of "nothing" to map	
*/
int muse_sync(uint32_t addr, size_t len);

/**
* Borra el mappeo a un archivo hecho por muse_map.
* @param dir Dirección a memoria mappeada.
* @note Esto implicará que todas las futuras utilizaciones de direcciones basadas en `dir` serán accesos inválidos.
* @note Solo se deberá cerrar el archivo mappeado una vez que todos los hilos hayan liberado la misma cantidad de muse_unmap que muse_map.
* @return Si pasa un error, retorna -1. Si la operación se realizó correctamente, retorna 0.
*/
int muse_unmap(uint32_t dir);

/* Funciones agregadas x nosotros */

/** 
* call es el serializador, hay varios funciones prehechas que se pueden usar
* @param function la funcion a llamar (dah)
*/ 
int call(Function *function);

#endif
