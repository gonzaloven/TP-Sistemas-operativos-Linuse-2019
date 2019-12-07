#include "suse.h"

#include <commons/collections/node.h>
#include <commons/collections/queue.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "libraries.h"

//hols
t_log* logger;
t_log* logger_metrics;
char * suse_config_path = "/home/utnso/workspace/tp-2019-2c-Los-Trapitos/configs/planificador.config";

int main(void){
	char* log_file;
	log_file = "/home/utnso/workspace/tp-2019-2c-Los-Trapitos/logs/planificador_logs.txt";
	logger = log_create(log_file, "SUSE logs", 1, 1);
	log_info(logger, "Inicializando SUSE. \n");

	configuracion_suse = get_configuracion();
	log_info(logger, "Archivo de configuracion levantado. \n");
	
	init_semaforos();

	new_queue = list_create();
	blocked_queue = list_create();
	exit_queue = list_create();

	pthread_mutex_init(&mutex_new_queue, NULL);
	pthread_mutex_init(&mutex_blocked_queue, NULL);
	pthread_mutex_init(&mutex_exit_queue,NULL);
	pthread_mutex_init(&mutex_multiprog, NULL);
	pthread_mutex_init(&mutex_semaforos,NULL);
	pthread_mutex_init(&mutex_process_list, NULL);


	iniciar_metricas();
	log_info(logger, "Iniciando metricas.. \n");


	log_info(logger, "Iniciando servidor... \n");
	iniciar_servidor();

	list_destroy(new_queue);
	list_destroy(blocked_queue);
	list_destroy(exit_queue);

	pthread_mutex_destroy(&mutex_new_queue);
	pthread_mutex_destroy(&mutex_blocked_queue);
	pthread_mutex_destroy(&mutex_multiprog);
	pthread_mutex_destroy(&mutex_semaforos);

	return EXIT_SUCCESS;
}

void iniciar_metricas(){
	t_list* params = list_create();
	nuevo_hilo(metricas, params);
}

void* metricas(void* params){

	char* metrics_logs;
	metrics_logs = "/home/utnso/workspace/tp-2019-2c-Los-Trapitos/logs/METRICAS_SUSE.txt";
	logger_metrics = log_create(metrics_logs, "Metrics logs", 1, 1);
	while(1)
	{
	sleep(configuracion_suse.METRICS_TIMER);

	metricas_sistema();
	metricas_por_programa();
	metricas_por_hilo();
	}
	pthread_detach(pthread_self());
	return NULL;
}

void metricas_por_programa()
{
	log_info(logger_metrics, "Calculando metricas por programa...\n");
	int cantidadProgramas = list_size(configuracion_suse.process);

	for(int i = 0; i < cantidadProgramas; i++)
	{
		t_process* process = list_get(configuracion_suse.process,i);

		log_info(logger_metrics, "Metricas del programa %d:...\n",process->PROCESS_ID);

		int news = 0;
		int readys = 0;
		int executing = 0;
		int blocked = 0;
		int exit = 0;

		int cantidadHilos = list_size(process->ULTS);
		int j = 0;
		while(j<cantidadHilos)
		{

			t_suse_thread* thread = list_get(process->ULTS,j);

			switch(thread->estado)
			{

			case 1:
				readys ++;
			break;
			case 2:
				executing = 1;
			break;
			case 3:
				blocked ++;
			break;
			case 5:
				news ++;
			break;

			}
			j++;
		}

		bool buscadorNew(t_suse_thread* thread)
		{
			return thread->procesoId == process->PROCESS_ID;
		}

		t_list* nuevaLista = list_filter(exit_queue,buscadorNew);
		exit = list_size(nuevaLista);

		log_info(logger_metrics,"Cantidad de hilos por cola:\n");
		log_info(logger_metrics,"En cola de new: %d...\n",news);
		log_info(logger_metrics,"En cola de ready: %d...\n",readys);
		log_info(logger_metrics,"En cola de execute: %d...\n",executing);
		log_info(logger_metrics,"En cola de blocked: %d...\n",blocked);
		log_info(logger_metrics,"En cola de exit: %d...\n",exit);
		log_info(logger_metrics,"---------------------------------------\n");

	}

}


void metricas_por_hilo()
{
	double tiempo = get_time_today();
	int cantidadProcesos = list_size(configuracion_suse.process);

	for(int i = 0; i < cantidadProcesos; i++)
	{
		t_process* process = list_get(configuracion_suse.process,i);
		log_info(logger_metrics,"Metricas para el proceso %d: \n",process->PROCESS_ID);
		int cantidadHilos = list_size(process->ULTS);

		for(int j = 0; j < cantidadHilos; j++)
		{
			t_suse_thread* thread = list_get(process->ULTS,j);
			double ejecucion = get_tiempo_ejecucion(thread,tiempo);
			double espera = thread->tiempoDeEspera;
			double cpu = thread->tiempoDeCpu;
			double ejecucionTotal = get_ejecucion_total(process);
			double porcentajeEjecucion = (ejecucionTotal / ejecucion)*100 ;

			log_info(logger_metrics,"Tiempo de ejecucion para hilo %d: %f\n",thread->tid,ejecucion);
			log_info(logger_metrics,"Tiempo de espera para hilo %d: %f\n",thread->tid,espera);
			log_info(logger_metrics,"Tiempo de uso de CPU para hilo %d: %f\n",thread->tid,cpu);
			log_info(logger_metrics,"Porcentaje de ejecucion para hilo %d: %f\n",thread->tid,porcentajeEjecucion);

		}
	}

}

double get_tiempo_ejecucion(t_suse_thread* thread, double tiempo)
{
	return tiempo - thread->tiempoDeEjecucion;
}


double get_ejecucion_total(t_process* process)
{
	int cantidadHilos = list_size(process->ULTS);
	double contador = 0;
	for(int i = 0; i < cantidadHilos; i++)
	{
		t_suse_thread* thread = list_get(process->ULTS,i);
		contador += thread->tiempoDeEjecucion;
	}
	return contador;
}


void metricas_sistema(){
	log_info(logger_metrics, "Calculando metricas del sistema...\n");
	log_info(logger_metrics, "Semaforos actuales: \n");

	int cantidad_semaforos = list_size(configuracion_suse.semaforos);

	pthread_mutex_lock(&mutex_semaforos);
	for(int i = 0; i < cantidad_semaforos; i++)
	{
		t_suse_semaforos* semaforo = list_get(configuracion_suse.semaforos,i);
		log_info(logger_metrics,"Valor actual del semaforo %s:  %d...\n",semaforo->NAME,semaforo->VALUE);
	}
	pthread_mutex_unlock(&mutex_semaforos);

	pthread_mutex_lock(&mutex_multiprog);
	log_info(logger_metrics, "Grado actual de multiprogramacion actual: %d", configuracion_suse.ACTUAL_MULTIPROG);
	pthread_mutex_unlock(&mutex_multiprog);
}

void init_semaforos(){

	log_info(logger, "Inicializando semaforos \n");
	int i = 0;
	while(configuracion_suse.SEM_IDS[i] != NULL)
	{
		t_suse_semaforos* semaforo = malloc(sizeof(t_suse_semaforos));
				semaforo->NAME = (char*)configuracion_suse.SEM_IDS[i];
				semaforo->INIT = atoi((char*)configuracion_suse.SEM_INIT[i]);
				semaforo->MAX = atoi((char*)configuracion_suse.SEM_MAX[i]);
				semaforo->VALUE = atoi((char*)configuracion_suse.SEM_INIT[i]);
				semaforo->BLOCKED_LIST = list_create();
				list_add(configuracion_suse.semaforos, semaforo);
				i++;
	}

}

suse_configuration get_configuracion() {

	log_info(logger, "Levantando archivo de configuracion de SUSE \n");

	suse_configuration configuracion_suse;
	t_config* archivo_configuracion;
	archivo_configuracion = config_create(suse_config_path);
	t_list* process = list_create();
	t_list* semaforos;
	
	configuracion_suse.LISTEN_PORT = copy_string(get_campo_config_string(archivo_configuracion, "LISTEN_PORT"));
	configuracion_suse.METRICS_TIMER = get_campo_config_int(archivo_configuracion, "METRICS_TIMER"); //SE LLAMA METRICS TIMER
	configuracion_suse.SEM_IDS = get_campo_config_array(archivo_configuracion, "SEM_IDS");
	configuracion_suse.SEM_INIT = get_campo_config_array(archivo_configuracion, "SEM_INIT");
	configuracion_suse.SEM_MAX = get_campo_config_array(archivo_configuracion, "SEM_MAX");
	configuracion_suse.ALPHA_SJF = get_campo_config_double(archivo_configuracion, "ALPHA_SJF");
	configuracion_suse.MAX_MULTIPROG = get_campo_config_int(archivo_configuracion,"MAX_MULTIPROG");
	configuracion_suse.ACTUAL_MULTIPROG = 0;
	configuracion_suse.process = process;
	configuracion_suse.semaforos = list_create();

	config_destroy(archivo_configuracion);

	return configuracion_suse;
}

void iniciar_servidor() {
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, configuracion_suse.LISTEN_PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1); //decia salir
	}

	for(p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) {
			continue;
		}
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
		break;
	}
	// if we got here, it means we didn't get bound
	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	freeaddrinfo(ai); // all done with this
	// listen
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(3);
	}

/*
--------------------------------------------------------
----------------- Conexiones entrantes -----------------
--------------------------------------------------------
*/
	while(1){
		log_info(logger,"SUSE esperando conexiones...\n");

		un_socket new_connection = aceptar_conexion(listener);

		t_paquete* handshake = recibir(listener);

		log_info(logger, "Recibi una nueva conexion del socket %d. \n", new_connection);

		free(handshake);

		//Creo un hilo por programa que se conecta
		log_info(logger, "Creando hilo para atender al proceso %d. \n", new_connection);

		t_list* thread_params;
		thread_params = list_create();
		list_add(thread_params, new_connection);
		nuevo_hilo(process_conectado_funcion_thread, thread_params);
		log_info(logger, "Cree el hilo para el proceso %d", new_connection);
	}
}

void* process_conectado_funcion_thread(void* argumentos) {
	un_socket socket_actual = list_get(argumentos, 0);
	list_destroy(argumentos);
	handle_conection_suse(socket_actual);
	pthread_detach(pthread_self());
	return NULL;
}

void handle_conection_suse(un_socket socket_actual)
{
	bool atender = true;

	while(atender)
	{

		t_paquete* paquete_recibido = recibir(socket_actual);

		switch(paquete_recibido->codigo_operacion)
		{
			case cop_suse_create:
				handle_suse_create(socket_actual, paquete_recibido);
				break;
			case cop_next_tid:
				handle_next_tid(socket_actual, paquete_recibido);
				break;
			case cop_close_tid:
				handle_close_tid(socket_actual, paquete_recibido);
				break;
			case cop_wait_sem:
				handle_wait_sem(socket_actual, paquete_recibido);
				break;
			case cop_signal_sem:
				handle_signal_sem(socket_actual,paquete_recibido);
				break;
			case cop_suse_join:
				handle_suse_join(socket_actual,paquete_recibido);
				break;
			default:
				atender = false;

			}


	}
}

void handle_suse_join(un_socket socket_actual, t_paquete * paquete_recibido){

	log_info(logger,"Inicio Join");
	esperar_handshake(socket_actual, paquete_recibido, cop_suse_join);

	log_info(logger, "Realice el primer handshake con hilolay\n");
	log_info(logger,"el socket es", "%d", socket_actual);

	t_paquete* paquete = recibir(socket_actual);

	int desplazamiento = 0;
	int tid = deserializar_int(paquete->data, &desplazamiento);

	int resultado = join(socket_actual,tid);

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;

	serializar_int(buffer, &desp, resultado);
	enviar(socket_actual, cop_wait_sem, tamanio_buffer, buffer);

	free(buffer);
	liberar_paquete(paquete);
}

void handle_close_tid(un_socket socket, t_paquete* paquete_recibido)
{
	esperar_handshake(socket, paquete_recibido, cop_close_tid);

	log_info(logger, "Realice el handshake de close_tid\n");

	t_paquete* paquete = recibir(socket);

	int desplazamiento = 0;
	int tid = deserializar_int(paquete->data, &desplazamiento);
	liberar_paquete(paquete);
	log_info(logger, "Recibi un close para el ULT %d\n", tid);

	int resultado = close_tid(tid,socket);

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;

	serializar_int(buffer, &desp, resultado);
	enviar(socket, cop_close_tid, tamanio_buffer, buffer);

	free(buffer);

}

int join(un_socket socket, int tid){

	bool buscador_thread_id(t_suse_thread* thread){
		return thread->tid == tid;
	}

	bool buscador_process_id(t_process* p)
	{
		return p->PROCESS_ID == socket;
	}

	t_process* process = list_find(configuracion_suse.process, buscador_process_id);
	t_suse_thread* thread_executing = process->EXEC_THREAD;
	t_suse_thread* thread_joined = list_find(process->ULTS, buscador_thread_id);

	if(thread_joined == 0 || thread_executing == 0)
	{
		return 0;
	}

	list_add(thread_executing->joinTo,thread_joined);
	list_add(thread_joined->joinedBy,thread_executing);

	ejecucion_a_bloqueado(thread_executing,socket);

	return 0;

}

void handle_suse_create(un_socket socket_actual, t_paquete* paquete_hilolay) {
	log_info(logger, "Esperando handshake en suse_create...\n");
	esperar_handshake(socket_actual, paquete_hilolay, cop_suse_create);

	t_paquete* paquete_recibido = recibir(socket_actual);

	bool find_process_by_id(void* process) {
		return ((t_process*)process)->PROCESS_ID == (int)socket_actual;
	}

	t_process* process = list_find(configuracion_suse.process, find_process_by_id);

	int desplazamiento = 0;
	int tid = deserializar_int(paquete_recibido->data, &desplazamiento);

	log_info(logger, "Recibi el ULT %d del proceso %d \n", tid, socket_actual);
	log_info(logger, "Validando si es el main thread o ULT comun \n");

	if(list_size(configuracion_suse.process) != 0  && process != 0){
		log_info(logger, "El proceso %d ya tiene un main_thread \n", socket_actual);
		handle_ULT_create(process, tid);
	}
	else{
		log_info(logger, "El proceso %d es nuevo \n", socket_actual);
		handle_main_thread_create(socket_actual, tid);
	}

	liberar_paquete(paquete_recibido);
}

void handle_ULT_create(t_process* process, int tid){

	t_suse_thread * new_thread;
	new_thread  = ULT_create(process, tid);

	log_info(logger, "El grado de multiprogramacion es %d \n", configuracion_suse.ACTUAL_MULTIPROG);

	pthread_mutex_lock(&mutex_multiprog);
	if(validar_grado_multiprogramacion())
	{
		nuevo_a_listo(new_thread,process->PROCESS_ID);
		log_info(logger, "El ULT %d del proceso %d esta en ready \n", new_thread->tid, process->PROCESS_ID);
	}
	else{
		log_info(logger, "No puedo agregar el ULT %d del proceso %d a la cola de ready por grado de multiprogramacion \n", tid);
	}

	pthread_mutex_unlock(&mutex_multiprog);

}

void handle_main_thread_create(un_socket socket_actual, int tid) {

	t_process* new_process = generar_process(socket_actual);
	log_info(logger, "Proceso creado con id %d \n", new_process->PROCESS_ID);

	list_add(configuracion_suse.process, new_process);

	t_suse_thread* main_thread = ULT_create(new_process, tid);

	log_info(logger, "Validando si puedo poner a ejecutar el main thread... \n", configuracion_suse.ACTUAL_MULTIPROG);

	pthread_mutex_lock(&mutex_multiprog);
	if(validar_grado_multiprogramacion()){
		nuevo_a_ejecucion(main_thread, new_process->PROCESS_ID);
		log_info(logger, "El thread %d del proceso %d esta ejecutando \n", main_thread->tid, new_process->PROCESS_ID);
	}

	pthread_mutex_unlock(&mutex_multiprog);

}

t_suse_thread* ULT_create(t_process* process, int tid){
	log_info(logger, "Creando ULT %d", tid);
	double tiempo = get_time_today();

	t_suse_thread* new_thread = malloc(sizeof(t_suse_thread));
	new_thread->tid = tid;
	new_thread->procesoId = process->PROCESS_ID;
	new_thread->estado = E_NEW;
	new_thread->duracionRafaga = 0;
	new_thread->ejecutado_desde_estimacion = false;
	new_thread->joinTo = list_create();
	new_thread->joinedBy = list_create();
	new_thread->estimacionUltimaRafaga = 0;
	new_thread->tiempoDeCpu = 0;
	new_thread->tiempoDeEspera = 0;
	new_thread->tiempoDeEjecucion = tiempo;

	list_add(process->ULTS, new_thread);
	list_add(new_queue, new_thread);
	log_info(logger, "ULT creado con id %d \n", new_thread->tid);

	return new_thread;
}


int close_tid(int tid, int socket_actual){
	bool find_process_by_id(t_process* process)
	{
		return process->PROCESS_ID == socket_actual;
	}

	bool find_thread_by_tid(t_suse_thread* thread)
	{
		return thread->tid == tid;
	}

	t_process* process = list_find(configuracion_suse.process,find_process_by_id);
	t_suse_thread* thread = list_find(process->ULTS,find_thread_by_tid);

	if(thread->tid == 0)
	{
		log_info(logger, "Recibi un close para el ULT %d\n", tid);
		list_destroy_and_destroy_elements(process->ULTS, free);
		bool destructor(t_process* p)
		{
			return p->PROCESS_ID == socket_actual;
		}

		list_remove_and_destroy_by_condition(configuracion_suse.process, destructor, free);
		int resultado = close(socket_actual);
		return resultado;
	}

	if(!list_is_empty(thread->joinedBy))
	{
		desjoinear(process,thread);
	}

	switch(thread->estado){
		case E_NEW:
				pthread_mutex_lock(&mutex_new_queue);
				nuevo_a_exit(thread,socket_actual);
				pthread_mutex_unlock(&mutex_new_queue);
		break;

		case E_BLOCKED:

			pthread_mutex_lock(&mutex_blocked_queue);
			pthread_mutex_lock(&mutex_multiprog);

			configuracion_suse.ACTUAL_MULTIPROG --;
			bloqueado_a_exit(thread,socket_actual);
			if(validar_grado_multiprogramacion())
			{
				obtener_ULT_ready_FIFO();
			}

			pthread_mutex_unlock(&mutex_multiprog);
			pthread_mutex_unlock(&mutex_blocked_queue);
		break;

		case E_READY:

			pthread_mutex_lock(&mutex_multiprog);

			configuracion_suse.ACTUAL_MULTIPROG --;
			listo_a_exit(thread,socket_actual);
			if(validar_grado_multiprogramacion())
			{
				obtener_ULT_ready_FIFO();
			}

			pthread_mutex_unlock(&mutex_multiprog);
		break;

		case E_EXECUTE:
				pthread_mutex_lock(&mutex_multiprog);
				process->EXEC_THREAD = 0;
				ejecucion_a_exit(thread,socket_actual);
				configuracion_suse.ACTUAL_MULTIPROG --;
				if(validar_grado_multiprogramacion())
				{
					obtener_ULT_ready_FIFO();
				}

				pthread_mutex_unlock(&mutex_multiprog);
		break;
	}

	metricas_sistema();
	metricas_por_programa();
	metricas_por_hilo();

	return 1;
}

void desjoinear(t_process* process, t_suse_thread* thread_joineado)
{

	int size = list_size(thread_joineado->joinedBy);

	for(int i = 0; i < size; i++)
	{
		t_suse_thread* thread= list_get(thread_joineado->joinedBy,i);
		desjoinear_hilo(thread_joineado,thread);
	}

}

void desjoinear_hilo(t_suse_thread* thread_joineado, t_suse_thread* thread_joiner)
{
	bool comparador(t_suse_thread* th){
		return th->tid == thread_joineado->tid;
	}

	list_remove_by_condition(thread_joiner->joinTo,comparador);

	if(list_is_empty(thread_joiner->joinTo))
	{
		pthread_mutex_lock(&mutex_multiprog);

		if(validar_grado_multiprogramacion())
		{
			int pid = thread_joiner->procesoId;

			bool buscador(t_process* p)
			{
				return p->PROCESS_ID == pid;
			}

			t_process* process = list_find(configuracion_suse.process,buscador);

			bloqueado_a_listo(thread_joiner,process);
		}
		else
		{
			thread_joiner->estado = E_NEW;

			pthread_mutex_lock(&mutex_new_queue);
			list_add(new_queue,thread_joiner);
			pthread_mutex_unlock(&mutex_new_queue);
		}

		pthread_mutex_unlock(&mutex_multiprog);
	}

}

void obtener_ULT_ready_FIFO()
{
	t_suse_thread* thread = list_get(new_queue,0);
	bool buscador(t_process* p)
	{
		return p->PROCESS_ID == thread->procesoId;
	}
	if(thread != 0)
	{
		t_process* process = list_find(configuracion_suse.process,buscador);
		nuevo_a_listo(thread, process->PROCESS_ID);
	}
}



bool validar_grado_multiprogramacion()
{
	log_info(logger, "Validando grado de multiprogramacion...\n");
	return configuracion_suse.MAX_MULTIPROG >= configuracion_suse.ACTUAL_MULTIPROG + 1;
	log_info(logger, "El grado de multiprogramacion es %d\n", configuracion_suse.ACTUAL_MULTIPROG);
}

t_process * generar_process(int process_id) {
	t_process * process = malloc(sizeof(t_process));
	process->PROCESS_ID = process_id; //todo ver tipo de dato
	process->ULTS = list_create();
	process->READY_LIST = list_create();
	process->EXEC_THREAD = 0;
	sem_init(&(process->semaforoReady),0,0);

	return process;
}


void handle_next_tid(un_socket socket_actual, t_paquete * paquete_next_tid){
	log_info(logger, "Inicio schedule_next...\n");
	bool comparador(t_process* p)
	{
		return p->PROCESS_ID == socket_actual;
	}

	t_process* process = list_find(configuracion_suse.process, comparador);
	sem_t sem_ULTs_listos = process->semaforoReady;
	int value;
	sem_getvalue(&sem_ULTs_listos,&value);
	log_info(logger, "El valor del sem_ULTs_listos es %d \n", value);
	esperar_handshake(socket_actual, paquete_next_tid, cop_next_tid);

	t_paquete* paquete_recibido = recibir(socket_actual);
	int desplazamiento = 0;
	int msg = deserializar_int(paquete_recibido->data, &desplazamiento);

	log_info(logger, "Recibi una peticion de suse_schedule_next \n");
	log_info(logger, "Iniciando planificacion...\n");

	int next_tid = obtener_proximo_ejecutar(process);

	// Envio el next tid
	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;
	serializar_int(buffer, &desp, next_tid);
	enviar(socket_actual, cop_next_tid, tamanio_buffer, buffer);
	log_info(logger, "Envie el next_tid %d a SUSE", next_tid);
	free(buffer);
	liberar_paquete(paquete_recibido);
}

double get_time_today(){
	struct timeval current_time;
	gettimeofday(&current_time,NULL);
	return (double)(current_time.tv_sec + (double)current_time.tv_usec/1000000);
}

int obtener_proximo_ejecutar(t_process* process){
	double tiempo_actual;
	tiempo_actual = get_time_today();
	t_suse_thread* exec_actual = process->EXEC_THREAD;

	if(exec_actual != 0)
	{
		double rafaga_actual = exec_actual->duracionRafaga;

		//Calculo  la rafaga del hilo actual
		if(rafaga_actual != 0)
		{
			exec_actual->duracionRafaga = tiempo_actual - rafaga_actual;
		}
		ejecucion_a_listo(exec_actual,process->PROCESS_ID);

	}
	sem_wait(&(process->semaforoReady));
	ordenar_cola_listos(process->READY_LIST);

	int count = list_size(process->READY_LIST);

	for(int i = 0 ; i<count; i++)
	{
		t_suse_thread* th = list_get(process->READY_LIST,i);
		log_info(logger,"El hilo %d tiene rafaga %f...\n",th->tid,th->estimacionUltimaRafaga);
	}

	t_suse_thread *next_ULT = list_get(process->READY_LIST, 0); // TODO: Si no funciona hacemos una bool que haga return thread != null; va a retornar el primero q haya
	if(next_ULT == 0)
	{
		log_info(logger,"No hay procesos para ejecutar \n"); //Esto se soluciona con el semaforo
		return -1;
	}
	int next_tid = next_ULT->tid;
	log_info(logger, "El proximo ULT a ejecutar es %d", next_tid);

	listo_a_ejecucion(next_ULT, process->PROCESS_ID);
	//Seteo la rafaga del hilo actual
	next_ULT->duracionRafaga = tiempo_actual;
	return next_tid;
}

void ordenar_cola_listos(t_list* ready_list) {
	estimar_ULTs_listos(ready_list);
	ordenar_por_sjf(ready_list);
}

void estimar_ULTs_listos(t_list* ready_list) {
	//TODO ver sino es void*
	log_info(logger, "Estimando ULTs...\n");
	void estimar(t_suse_thread* ULT) {
		if (ULT->ejecutado_desde_estimacion) {
			ULT->ejecutado_desde_estimacion = false;
			estimar_rafaga(ULT);
		}
	}
	list_iterate(ready_list, estimar);
}

void estimar_rafaga(t_suse_thread * ULT){
	double rafaga_anterior = ULT->duracionRafaga; //Duracion de la rafaga anterior
	double estimacion_anterior = ULT->estimacionUltimaRafaga; // Estimacion anterior
	double estimacion = configuracion_suse.ALPHA_SJF * rafaga_anterior + (1 - configuracion_suse.ALPHA_SJF) * estimacion_anterior;
	ULT->estimacionUltimaRafaga = estimacion;
}

void ordenar_por_sjf(t_list* ready_list){
	log_info(logger, "Ordenando por SJF...");
	list_sort(ready_list, funcion_SJF);
}

bool funcion_SJF(t_suse_thread* ULT1, t_suse_thread* ULT2) {

	int process_id = ULT1->procesoId;

	bool comparador(t_process* p)
	{
		return p->PROCESS_ID == process_id;
	}

	pthread_mutex_lock(&mutex_process_list);
	t_process* process = list_find(configuracion_suse.process, comparador);
	pthread_mutex_unlock(&mutex_process_list);


	if (ULT1->estimacionUltimaRafaga == ULT2->estimacionUltimaRafaga) {
		return list_get(process->ULTS, 0); //todo ver como obtener el primer elemento no vacio de la lista // Si no funciona hacemos una bool que haga return thread = null; va a retornar el primero q haya
	}
	return ULT1->estimacionUltimaRafaga < ULT2->estimacionUltimaRafaga;
}


void handle_wait_sem(un_socket socket_actual, t_paquete* paquete_wait_sem){
	//Recibo el semaforo a decremetnar
	esperar_handshake(socket_actual, paquete_wait_sem, cop_wait_sem);
	log_info(logger, "Recibiendo el semaforo para incrementar \n");

	t_paquete* paquete_recibido = recibir(socket_actual);
	int desplazamiento = 0;
	int tid = deserializar_int(paquete_recibido->data, &desplazamiento);
	char* sem = deserializar_string(paquete_recibido->data, &desplazamiento);
	liberar_paquete(paquete_recibido);

	pthread_mutex_lock(&mutex_semaforos);
	int resultado = decrementar_semaforo(socket_actual,tid, sem);
	pthread_mutex_unlock(&mutex_semaforos);

	//Enviar la confirmacion del semaforo decrementado
	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;
	serializar_int(buffer, &desp, resultado);
	enviar(socket_actual, cop_wait_sem, tamanio_buffer, buffer);
	free(buffer);
}

void handle_signal_sem(un_socket socket_actual, t_paquete* paquete_signal_sem){

	esperar_handshake(socket_actual, paquete_signal_sem, cop_signal_sem);
	log_info(logger, "Recibiendo el semaforo para incrementar...\n");

	t_paquete* paquete_recibido = recibir(socket_actual);
	int desplazamiento = 0;
	int tid = deserializar_int(paquete_recibido->data, &desplazamiento);
	char* sem = deserializar_string(paquete_recibido->data, &desplazamiento);
	liberar_paquete(paquete_recibido);

	log_info(logger,"Recibi el semaforo %s para incrementar...\n",sem);

	pthread_mutex_lock(&mutex_semaforos);
	int resultado = incrementar_semaforo(tid, sem);
	if(resultado != -1){
		resultado = desbloquear_hilos_semaforo(sem);
	}
	pthread_mutex_unlock(&mutex_semaforos);

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;
	serializar_int(buffer, &desp, resultado);
	enviar(socket_actual, cop_wait_sem, tamanio_buffer, buffer);
	free(buffer);
}

//todo definir que le pasas, si un char* o un t_sem*
int desbloquear_hilos_semaforo(char* sem){

	bool buscador_sem_name(t_suse_semaforos* semaforo){
		return strings_equal(semaforo->NAME,sem);
	}

	t_suse_semaforos* semaforo = list_find(configuracion_suse.semaforos, buscador_sem_name);

	if(list_get(semaforo->BLOCKED_LIST, 0) == NULL){
		return 0;
	}

	//todo encontrar solucion  con listas

	t_suse_thread* thread = list_get(semaforo->BLOCKED_LIST,0);

	bool buscador_process_id(t_process* p){
		return p->PROCESS_ID == thread->procesoId;
	}

	t_process * process = list_find(configuracion_suse.process, buscador_process_id);

	bloqueado_a_listo(thread,process);

	list_remove(semaforo->BLOCKED_LIST,0);


	return 0;
}

int incrementar_semaforo(uint32_t tid, char* sem){

	bool comparador(t_suse_semaforos* semaforo){
		return strings_equal(semaforo->NAME,sem);
	}

	t_suse_semaforos* semaforo = list_find(configuracion_suse.semaforos, comparador);

	if(semaforo == NULL)
	{
		log_info(logger,"El semaforo no existe\n");
		return -1;
	}

	if(semaforo->VALUE == semaforo->MAX)
	{
		log_info(logger,"Semaforo al maximo"); //Esto no puede pasar pero bueno, hay que chequearlo
		return -1;
	}

	semaforo->VALUE ++;

	return 0;

}

int decrementar_semaforo(int socket_actual,int tid, char* sem_name){

	bool comparador(t_suse_semaforos* semaforo){
		return strings_equal(semaforo->NAME,sem_name);
	}

	t_suse_semaforos* semaforo = list_find(configuracion_suse.semaforos, comparador);

	if(semaforo == NULL)
	{
		log_info(logger,"El semaforo no existe\n");
		return -1;
	}

	log_info(logger,"El valor actual del semaforo es %d",semaforo->VALUE);

	if(semaforo->VALUE <= 0)
	{

		ejecucion_a_bloqueado_por_semaforo(tid,socket_actual,semaforo);

	}

	semaforo->VALUE --;

	return 0;
}

//////Funciones transicion de estados/////
void ejecucion_a_exit(t_suse_thread* thread, un_socket socket)
{

	bool buscador(t_process* program)
		{
			return program->PROCESS_ID == socket;
		}
	bool buscadorThread(t_suse_thread* th)
		{
			return th->tid == thread->tid;
		}
	pthread_mutex_lock(&mutex_process_list);
	t_process* process = list_find(configuracion_suse.process, buscador);
	pthread_mutex_unlock(&mutex_process_list);

	double tiempo = get_time_today();
	thread->tiempoDeCpu += (tiempo - thread->tiempoInicialEnExec);

	remover_ULT_exec(process);
	list_remove_by_condition(process->ULTS,buscadorThread);
	thread->estado = E_EXIT;
	pthread_mutex_lock(&mutex_exit_queue);
	list_add(exit_queue,thread);
	pthread_mutex_unlock(&mutex_exit_queue);

	log_info(logger, "El tid %d paso de execute a exit\n", thread->tid);
}

void bloqueado_a_exit(t_suse_thread* thread,un_socket socket)
{
	bool buscador(t_process* program)
			{
				return program->PROCESS_ID == socket;
			}
	bool buscadorThread(t_suse_thread* th)
		{
			return th->tid == thread->tid;
		}
	pthread_mutex_lock(&mutex_process_list);
	t_process* process = list_find(configuracion_suse.process, buscador);
	pthread_mutex_unlock(&mutex_process_list);

	eliminar_ULT_cola_actual(thread,process);
	list_remove_by_condition(process->ULTS,buscadorThread);
	thread->estado = E_EXIT;
	pthread_mutex_lock(&mutex_exit_queue);
	list_add(exit_queue,thread);
	pthread_mutex_unlock(&mutex_exit_queue);

	log_info(logger, "El tid %d paso de bloqueado a exit\n", thread->tid);
}

void listo_a_exit(t_suse_thread* thread,un_socket socket)
{
	bool buscador(t_process* program){
		return program->PROCESS_ID == socket;
	}
	bool buscadorThread(t_suse_thread* th)
	{
		return th->tid == thread->tid;
	}
	pthread_mutex_lock(&mutex_process_list);
	t_process* process = list_find(configuracion_suse.process, buscador);
	pthread_mutex_unlock(&mutex_process_list);

	double tiempo = get_time_today();
	thread->tiempoDeEspera += (tiempo - thread->tiempoInicialEnReady);

	eliminar_ULT_cola_actual(thread,process);
	list_remove_by_condition(process->ULTS,buscadorThread); //todo esto esta duplicado
	thread->estado = E_EXIT;
	pthread_mutex_lock(&mutex_exit_queue);
	list_add(exit_queue,thread);
	pthread_mutex_unlock(&mutex_exit_queue);

	sem_wait(&(process->semaforoReady));

	log_info(logger, "El tid %d paso de ready a exit\n", thread->tid);
}


void nuevo_a_exit(t_suse_thread* thread,un_socket socket_actual)
{
	bool buscador(t_process* program)
		{
			return program->PROCESS_ID == socket_actual;
		}
	bool buscadorThread(t_suse_thread* th)
	{
		return th->tid == thread->tid;
	}

	pthread_mutex_lock(&mutex_process_list);
	t_process* process = list_find(configuracion_suse.process, buscador);
	pthread_mutex_unlock(&mutex_process_list);

	eliminar_ULT_cola_actual(thread,process);
	list_remove_by_condition(process->ULTS,buscadorThread);
	thread->estado = E_EXIT;
	pthread_mutex_lock(&mutex_exit_queue);
	list_add(exit_queue,thread);
	pthread_mutex_unlock(&mutex_exit_queue);
	log_info(logger, "El tid %d paso de new a exit\n", thread->tid);

}

void listo_a_ejecucion(t_suse_thread* thread, un_socket socket){

	bool buscador(t_process* program)
	{
		return program->PROCESS_ID == socket;
	}

	pthread_mutex_lock(&mutex_process_list);
	t_process* process = list_find(configuracion_suse.process, buscador);
	pthread_mutex_unlock(&mutex_process_list);

	double tiempo = get_time_today();
	thread->tiempoInicialEnExec = tiempo;

	thread->tiempoDeEspera += (tiempo - thread->tiempoInicialEnReady);

	eliminar_ULT_cola_actual(thread,process);
	thread->estado = E_EXECUTE;
	thread->ejecutado_desde_estimacion = true;

	process->EXEC_THREAD = thread;
	log_info(logger, "El tid %d paso de ready a execute\n", thread->tid);
}

void nuevo_a_ejecucion(t_suse_thread* thread, un_socket socket)
{
	struct timeval aux;
	gettimeofday(&aux,NULL);

	bool buscador(t_process* process){
		return process->PROCESS_ID == socket;
	}

	t_process* process = list_find(configuracion_suse.process, buscador);

	eliminar_ULT_cola_actual(thread, process);

	double tiempo = get_time_today();
	thread->tiempoInicialEnExec = tiempo;

	thread->duracionRafaga = (double)(aux.tv_sec + (double)aux.tv_usec/1000000);
	thread->estado = E_EXECUTE;
	thread->ejecutado_desde_estimacion = true;
	process->EXEC_THREAD = thread;
	configuracion_suse.ACTUAL_MULTIPROG ++;

	log_info(logger, "El thread %d paso de new a execute \n", thread->tid);
}

void ejecucion_a_listo(t_suse_thread* thread, un_socket socket)
{
	bool buscador_process_id(t_process* p){
			return p->PROCESS_ID == socket;
	}

	t_process* process = list_find(configuracion_suse.process, buscador_process_id);

	double tiempo = get_time_today();
	thread->tiempoDeCpu += (tiempo - thread->tiempoInicialEnExec);

	thread->tiempoInicialEnReady = tiempo;
	remover_ULT_exec(process);
	thread->estado = E_READY;
	list_add(process->READY_LIST,thread);
	sem_post(&(process->semaforoReady));
	log_info(logger, "El thread %d paso de execute a ready \n", thread->tid);

}

void bloqueado_a_listo(t_suse_thread* thread,t_process* program)
{
	double tiempo = get_time_today();
	thread->tiempoInicialEnReady = tiempo;
	eliminar_ULT_cola_actual(thread,program);
	thread->estado = E_READY;
	list_add(program->READY_LIST,thread);
	sem_post(&(program->semaforoReady));
	log_info(logger, "El thread %d paso de blocked a ready \n", thread->tid);
}


void ejecucion_a_bloqueado(t_suse_thread* thread,un_socket socket)
{
	bool buscador_process_id(t_process* p){
			return p->PROCESS_ID == socket;
	}

	t_process* process = list_find(configuracion_suse.process, buscador_process_id);
	//t_suse_thread* th = process->EXEC_THREAD;

	double tiempo = get_time_today();
	thread->tiempoDeCpu += (tiempo - thread->tiempoInicialEnExec);

	double aux2 = thread->duracionRafaga;
	if(aux2 > 0)
	{
		struct timeval aux;
		gettimeofday(&aux,NULL);
		double tiempo = (double)(aux.tv_sec + (double)aux.tv_usec/1000000);
		thread->duracionRafaga = tiempo - aux2;
	}


	remover_ULT_exec(process);
	thread->estado = E_BLOCKED;
	pthread_mutex_lock(&mutex_blocked_queue);
	list_add(blocked_queue,thread);
	pthread_mutex_unlock(&mutex_blocked_queue);
	log_info(logger, "El thread %d paso de execute a blocked \n", thread->tid);
}

void ejecucion_a_bloqueado_por_semaforo(int tid, un_socket socket, t_suse_semaforos* semaforo)
{
	bool comparador(t_process* process)
	{
		return process->PROCESS_ID == socket;
	}

	bool comparadorThread(t_suse_thread* th)
	{
		return th->tid == tid;
	}

	t_process* program = list_find(configuracion_suse.process,comparador);

	t_suse_thread* thread = list_find(program->ULTS,comparadorThread);

	double tiempo = get_time_today();
	thread->tiempoDeCpu += (tiempo - thread->tiempoInicialEnExec);


	double aux2 = thread->duracionRafaga;
	if(aux2 > 0)
	{
		struct timeval aux;
		gettimeofday(&aux,NULL);
		double tiempo = (double)(aux.tv_sec + (double)aux.tv_usec/1000000);
		thread->duracionRafaga = tiempo - aux2;
	}

	remover_ULT_exec(program);

	thread->estado = E_BLOCKED;

	list_add(semaforo->BLOCKED_LIST,thread);

	pthread_mutex_lock(&mutex_blocked_queue);
	list_add(blocked_queue,thread);
	pthread_mutex_unlock(&mutex_blocked_queue);

	log_info(logger, "El thread %d paso de execute a blocked por semaforo %s \n", thread->tid, semaforo->NAME);
}

void eliminar_ULT_cola_actual(t_suse_thread *ULT, t_process* process)
{

	switch(ULT->estado)
	{

	case E_READY:
		remover_ULT_listo(ULT,process);
	break;
	case E_NEW:
		remover_ULT_nuevo(ULT);
	break;
	case E_EXECUTE:
		remover_ULT_exec(process);
	break;
	case E_BLOCKED:
		remover_ULT_bloqueado(ULT);
	break;

	}
}

void remover_ULT_nuevo(t_suse_thread* ULT)
{
	bool comparador(t_suse_thread* thread){
		return thread->tid == ULT->tid && thread->procesoId == ULT->procesoId;
	}
	pthread_mutex_lock(&mutex_new_queue);
	list_remove_by_condition(new_queue, comparador);
	pthread_mutex_unlock(&mutex_new_queue);
}

void remover_ULT_bloqueado(t_suse_thread* thread)
{
	bool comparador(t_suse_thread* th){
		return th->tid == thread->tid && th->procesoId == thread->procesoId;
	}

	pthread_mutex_lock(&mutex_blocked_queue);
	list_remove_by_condition(blocked_queue,comparador);
	pthread_mutex_unlock(&mutex_blocked_queue);

}
void remover_ULT_exec(t_process* process)
{
	process->EXEC_THREAD = 0;

}

void nuevo_a_listo(t_suse_thread* ULT, int process_id)
{
	bool comparador(t_process* process)
	{
		return process->PROCESS_ID == process_id;
	}

	t_process* program = list_find(configuracion_suse.process, comparador);

	double tiempo = get_time_today();
	ULT->tiempoInicialEnReady = tiempo;

	list_add(program->READY_LIST, ULT);

	eliminar_ULT_cola_actual(ULT, program);
	ULT->estado = E_READY;
	configuracion_suse.ACTUAL_MULTIPROG ++;
	sem_post(&(program->semaforoReady));
}

void remover_ULT_listo(t_suse_thread* thread,t_process* process)
{

	bool comparador(t_suse_thread* th){
		return th->tid == thread->tid;
	}

	list_remove_by_condition(process->READY_LIST,comparador); //Remuevo por tid.

}

pthread_t nuevo_hilo(void *(* funcion ) (void *), t_list * parametros)
{
	pthread_t thread = threads[i_thread];
	int thread_creacion = pthread_create(&thread, NULL, funcion, (void*) parametros);
	if (thread_creacion != 0) {
		perror("pthread_create");
	} else {
		i_thread++;
	}
	return thread;
}


/////Fin funciones transicion de estados /////
