#ifndef SUSE_H
#define SUSE_H

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <commons/string.h>
#include <libraries.h>
#include <stdbool.h>

typedef struct suse_configuration
{
	char* LISTEN_PORT;
	int METRICS_TIMER;
	char** SEM_IDS;
	char** SEM_INIT;
	char** SEM_MAX;
	double ALPHA_SJF;
	int MAX_MULTIPROG;
	t_list * process;
	t_list * semaforos;
	int ACTUAL_MULTIPROG;
}suse_configuration;

suse_configuration configuracion_suse;
suse_configuration get_configuracion();

pthread_mutex_t mutex_new_queue;
t_list* new_queue;

pthread_mutex_t mutex_blocked_queue;
t_list* blocked_queue;

pthread_mutex_t mutex_exit_queue;
pthread_mutex_t mutex_semaforos;

pthread_mutex_t mutex_multiprog;

pthread_mutex_t mutex_process_list;

pthread_mutex_t mutex_ready_queue;

sem_t sem_ULTs_listos;

t_list* exit_queue;

t_process * generar_programa(int socket_hilolay);

void handle_suse_create(un_socket socket_actual, t_paquete* paquete_hilolay);

int close_tid(int tid, int socket_actual);

void handle_close_tid(un_socket socket_actual, t_paquete* received_packet);

void handle_wait_sem(un_socket socket_actual, t_paquete* paquete_wait_sem);

void obtener_ULT_ready_FIFO();

int i_thread = 0;
pthread_t threads[20];

pthread_t nuevo_hilo(void *(* funcion ) (void *), t_list * parametros);

void* process_conectado_funcion_thread(void* argumentos);

int obtener_proximo_ejecutar(t_process* process);

void ordenar_cola_listos(t_list* ready_list);

void estimar_ULTs_listos(t_list* ready_list);

void estimar_rafaga(t_suse_thread * ULT);

void init_semaforos();

bool funcion_SJF(t_suse_thread* ULT1, t_suse_thread* ULT2);

void ordenar_por_sjf(t_list* ready_list);

void nuevo_a_exit(t_suse_thread* thread, un_socket socket_actual);

void bloqueado_a_exit(t_suse_thread* thread, un_socket socket);

void listo_a_exit(t_suse_thread* thread, un_socket socket);

t_process * generar_process(un_socket socket);

void handle_next_tid(un_socket socket_actual, t_paquete * paquete_next_tid);

void handle_signal_sem(un_socket socket_actual, t_paquete* paquete_signal_sem);

int incrementar_semaforo(uint32_t tid, char* sem);

int decrementar_semaforo(int socket_actual,int tid, char* sem_name);

void ejecucion_a_exit(t_suse_thread* thread, un_socket socket);

void handle_ULT_create(t_process* process, int tid);

void handle_suse_join(un_socket socket_actual, t_paquete * paquete_recibido);

int join(un_socket socket, int tid);

void desjoinear(t_process* process, t_suse_thread* thread_joineado);

void desjoinear_hilo(t_suse_thread* thread_joineado, t_suse_thread* thread_joiner);

void listo_a_ejecucion(t_suse_thread* thread, un_socket socket);

int desbloquear_hilos_semaforo(char* sem);

bool validar_grado_multiprogramacion();

void handle_main_thread_create(un_socket socket_actual, int tid);

t_suse_thread* ULT_create(t_process* process, int tid);

double get_time_today();
/*
--------------------------------------------------------
----------------- Variables para el Servidor -----------
--------------------------------------------------------
*/
void iniciar_servidor();

void handle_conection_suse(int socketActual);

int listener;     // listening socket descriptors
struct sockaddr_storage remoteaddr; // client address
socklen_t addrlen;
char buf[256];    // buffer for client data
int nbytes;
char remoteIP[INET6_ADDRSTRLEN];
int yes=1;        // for setsockopt() SO_REUSEADDR, below
int i, j, rv;
struct addrinfo hints, *ai, *p;
/*
--------------------------------------------------------
----------------- FIN Variables para el Servidor -------
--------------------------------------------------------
*/


#endif 

