#ifndef LIBRARIES_H_
#define LIBRARIES_H_

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <commons/string.h>
#include <commons/error.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/txt.h>
#include <commons/log.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <ifaddrs.h>
#include <stdarg.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#define MAX_LEN 128


enum codigos_de_operacion {
	codigo_error = -1,
	codigo_healthcheck = 2,
	cop_generico = 0,
	cop_archivo = 1,

	cop_handshake_hilolay_suse = 10,
	cop_handshake_suse_hilolay = 11,
	cop_next_tid = 12,
	cop_close_tid = 13
};

typedef int un_socket;

typedef struct {
	int codigo_operacion;
	int tamanio;
	void * data;
} t_paquete;

enum estados {
	READY = 0,
	EXECUTE = 1,
	BLOCKED = 2,
	EXIT = 3,
	NEW = 4 //todo ver si es necesario
};

typedef struct {
	int tid;
	int estado;
} t_suse_thread;

/**	@NAME: conectar_a
 * 	@DESC: Intenta conectarse.
 * 	@RETURN: Devuelve el socket o te avisa si hubo un error al conectarse.
 *
 */

un_socket conectar_a(char *IP, char* Port);

/**	@NAME: socket_escucha
 * 	@DESC: Configura un socket que solo le falta hacer listen.
 * 	@RETURN: Devuelve el socket listo para escuchar o te avisa si hubo un error al conectarse.
 *
 */

un_socket socket_escucha(char* IP, char* Port);

/**	@NAME: enviar
 * 	@DESC: Hace el envio de la data que le pasamos. No hay que hacer más nada.
 *
 */

void enviar(un_socket socket_envio, int codigo_operacion, int tamanio,
		void * data);

/**	@NAME: recibir
 * 	@DESC: devuelve un paquete que está en el socket pedido
 *
 */
t_paquete* recibir(un_socket socket_para_recibir);

/**	@NAME: aceptar_conexion
 * 	@DESC: devuelve el socket nuevo que se quería conectar
 *
 */
un_socket aceptar_conexion(un_socket socket_escuchador);

/**	@NAME: liberar_paquete
 * 	@DESC: libera el paquete y su data.
 *
 */
void liberar_paquete(t_paquete * paquete);

/**	@NAME: realizar_handshake
 *
 */
bool realizar_handshake(un_socket socket_del_servidor, int cop_handshake);

/**	@NAME: esperar_handshake
 *
 */
bool esperar_handshake(un_socket socket_del_cliente,
		t_paquete* inicio_del_handshake, int cop_handshake);

char get_campo_config_char(t_config* archivo_configuracion, char* nombre_campo);

int get_campo_config_int(t_config* archivo_configuracion, char* nombre_campo);

char** get_campo_config_array(t_config* archivo_configuracion,
		char* nombre_campo);

char* get_campo_config_string(t_config* archivo_configuracion,
		char* nombre_campo);

void enviar_archivo(un_socket socket, char* path);

bool comprobar_archivo(char* path);

char* obtener_mi_ip();

void print_image(FILE *fptr);

void imprimir(char*);

char** str_split(char* a_str, const char a_delim);

char *str_replace(char *orig, char *rep, char *with);

int countOccurrences(char * str, char * toSearch);

char *randstring(size_t length);

unsigned long int lineCountFile(const char *filename);

void crear_subcarpeta(char* nombre);

char* generarDirectorioTemporal(char* carpeta);

void *get_in_addr(struct sockaddr *sa);

int size_of_string(char* string);

char* string_concat(int cant_strings, ...);

void log_and_free(t_log* logger, char* mensaje);

void log_error_and_free(t_log* logger, char* mensaje);

void free_array(char** array, int array_size);

char* copy_string(char* value);

void enviar_listado_de_strings(un_socket socket, t_list * listado_strings);

t_list * recibir_listado_de_strings(un_socket socket);

int cantidad_entradas_necesarias(char* valor, int tamanio_entrada);

void serializar_int(void * buffer, int * desplazamiento, int valor);

int deserializar_int(void * buffer, int * desplazamiento);

void serializar_string(void * buffer, int * desplazamiento, char* valor);

char* deserializar_string(void * buffer, int * desplazamiento);

void serializar_lista_strings(void * buffer, int * desplazamiento, t_list * lista);

t_list * deserializar_lista_strings(void * buffer, int * desplazamiento);

int size_of_strings(int cant_strings, ...);

int size_of_list_of_strings_to_serialize(t_list * list);

void destruir_lista_strings(t_list * lista);

bool strings_equal(char* string1, char* string2);

void detectar_deadlock();

t_list * copy_list(t_list * lista);

int array_of_strings_length(char** array);

t_list * list_remove_all_by_condition(t_list * lista, bool(*condicion)(void*)); // Remueve todos los elementos de una determinada condicion y los devuelve

#endif /* LIBRARIES_H_ */
