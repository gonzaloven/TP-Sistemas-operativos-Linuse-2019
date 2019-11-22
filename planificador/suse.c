#include "suse.h"
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/node.h>
#include <commons/collections/queue.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

int main(void){
	char* log_path = "../logs/suse_logs.txt";

	logger = log_create(log_path, "SUSE Logs", 1, 1);
	log_info(logger, "Inicializando SUSE. \n");

	configuracion_suse = get_configuracion();
	log_info(logger, "Archivo de configuracion levantado. \n");
	
	init_semaforos();


	new_queue = list_create();

	pthread_mutex_init(&mutex_new_queue, NULL);
	pthread_mutex_init(&mutex_ready_queue, NULL);
	pthread_mutex_init(&mutex_blocked_queue, NULL);
	pthread_mutex_init(&mutex_multiprog, NULL);
	pthread_mutex_init(&mutex_semaforos,NULL);

	iniciar_servidor();

	return EXIT_SUCCESS;
}

void init_semaforos(){

	uint32_t cantidad_semaforos = sizeof(configuracion_suse.SEM_ID) / sizeof(configuracion_suse.SEM_ID[0]);

	for(uint32_t i = 0; i < cantidad_semaforos; i++){

			t_suse_semaforos* semaforo = malloc (sizeof(t_suse_semaforos));
			semaforo->NAME = configuracion_suse.SEM_ID[i];
			semaforo->INIT = configuracion_suse.SEM_INIT[i];
			semaforo->MAX = configuracion_suse.SEM_MAX[i];
			semaforo->VALUE = configuracion_suse.SEM_INIT[i];

			list_add(configuracion_suse.semaforos,semaforo);
			free(semaforo);
	}

}

suse_configuration get_configuracion() {

	log_info(logger, "Levantando archivo de configuracion de SUSE \n");

	suse_configuration configuracion_suse;
	t_config* archivo_configuracion = config_create(SUSE_CONFIG_PATH);
	
	configuracion_suse.LISTEN_PORT = copy_string(get_campo_config_string(archivo_configuracion, "LISTEN_PORT"));
	configuracion_suse.METRICS_TIMER = get_campo_config_int(archivo_configuracion, "ESTIMACION_INICIAL");
	configuracion_suse.SEM_ID = get_campo_config_array(archivo_configuracion, "SEM_ID");
	configuracion_suse.SEM_INIT = get_campo_config_array(archivo_configuracion, "SEM_INIT");
	configuracion_suse.SEM_MAX = get_campo_config_array(archivo_configuracion, "SEM_MAX");
	configuracion_suse.ALPHA_SJF = get_campo_config_int(archivo_configuracion, "ALPHA_SJF");
	configuracion_suse.MAX_MULTIPROG = get_campo_config_int(archivo_configuracion,"MAX_MULTIPROG");
	configuracion_suse.ACTUAL_MULTIPROG = 0;

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
		salir(1);
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
		int new_connection = aceptar_conexion(listener);
		t_paquete* handshake = recibir(listener);
		log_info(logger, "Soy SUSE y recibi una nueva conexion . \n");
		free(handshake);
		handle_conection_suse(new_connection);
	}
}


void handle_conection_suse(int socket_actual) {
	//Aca supongo que en cada case habria que crear un hilo de atencion
	//para atender pedidos concurrentes, un hilo por programa que se conecte
	t_paquete* paquete_recibido = recibir(socket_actual);
	switch(paquete_recibido->codigo_operacion){
	//todo  hay que generar un case para:
	//1) primer handshake hilolay-suse cuando envia el hilo main
	//2) cuando nos pasan un hilo nuevo
		case cop_handshake_hilolay_suse:
			handle_hilolay(socket_actual, paquete_recibido);
		break;
		case cop_next_tid:
			handle_next_tid(socket_actual, paquete_recibido);
		break;
		case cop_close_tid:
			handle_close_tid(socket_actual,paquete_recibido);
		break;
		case cop_wait_sem:
			handle_wait_sem(socket_actual, paquete_recibido);
		break;
		case cop_signal_sem:
			handle_signal_sem(socket_actual,paquete_recibido);
		break;

	}
	liberar_paquete(paquete_recibido);
}

void handle_hilolay(un_socket socket_actual, t_paquete* paquete_hilolay) {

	esperar_handshake(socket_actual, paquete_hilolay, cop_handshake_hilolay_suse);

	log_info(logger, "Realice el primer handshake con hilolay\n");
	sprintf("el socket es", "%d", socket_actual);

	t_paquete* paquete_recibido = recibir(socket_actual);

	int desplazamiento = 0;
	int master_tid = deserializar_int(paquete_recibido->data, &desplazamiento);

	//El programa solo hay que generarlo cuando es el primer handshake. podria ser algo como lo siguiente.

	t_program * program =list_get(configuracion_suse.programs, socket_actual);

	if(program == NULL)
	{
		t_program * program = generar_programa(socket_actual);

		list_add_in_index(configuracion_suse.programs,program,program->PROGRAM_ID);

		sprintf("el socket es", "%d", program->PROGRAM_ID);
	}
	else
	{
		t_suse_thread * new_thread = malloc(sizeof(t_suse_thread));

		new_thread->tid = master_tid;
		new_thread->estado = NEW;
		new_thread->procesoId = socket_actual;

		list_add_in_index(program->ULTS,new_thread,new_thread->tid);
		list_add(new_queue,new_thread->tid);
		free(new_thread);
	}

	free(program);

	liberar_paquete(paquete_recibido);
}

void handle_close_tid(un_socket socket_actual, t_paquete* paquete_close_tid){
	//Recibo el hilo a cerrar
	esperar_handshake(socket_actual, paquete_close_tid, cop_close_tid);
	log_info(logger, "Realice handshake con hilolay\n");
	sprintf("el socket es", "%d", socket_actual);

	t_paquete* paquete_recibido = recibir(socket_actual); // Recibo hilo a finalizar
	int desplazamiento = 0;
	int tid = deserializar_int(paquete_recibido->data, &desplazamiento);

	int resultado= close_tid(tid,socket_actual);
	liberar_paquete(paquete_recibido);

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;

	serializar_int(buffer, &desp, resultado);
	enviar(socket_suse, cop_close_tid, tamanio_buffer, buffer);
	free(buffer);

}

t_program * generar_programa(un_socket socket) {
	t_program * program = malloc(sizeof(t_program));
	program->PROGRAM_ID = (int)socket; //todo ver si el socket solo es un int o hay que castear
	return program;
}

void handle_next_tid(un_socket socket_actual, paquete_next_tid){
	esperar_handshake(socket_actual, paquete_next_tid, cop_next_tid);
	t_paquete* paquete_recibido = recibir(socket_actual); // Recibo hilo principal
	int desplazamiento = 0;
	int msg = deserializar_int(paquete_recibido->data, &desplazamiento);
	liberar_paquete(paquete_recibido);

	int next_tid = get_next_tid();

	// Envio el next tid
	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;
	serializar_int(buffer, &desp, next_tid);
	enviar(socket_actual, cop_next_tid, tamanio_buffer, buffer);
	free(buffer);
}

int get_next_tid(){

	//todo planificar

}
int close_tid(int tid, int socket_actual){

	     t_program* program = list_get(configuracion_suse->programs, socket_actual);
	     t_suse_thread* thread = list_get(program->ULTS,tid);

	   /*  //IF THREAD IS NULL CREO Q SE TENIAN Q LIBERAR RECURSOS.. Supongo q es desconectar al programa (chau socket) ṕero como no estoy seguro ni se bien como hacerlo no lo hago
	     if(thread == NULL){
	    	 int resultado = close(socket_actual);
	    	 return resultado;
	     }*/

		switch(thread->estado){

		case NEW:
				pthread_mutex_lock(&mutex_new_queue);
				list_remove(new_queue,true/*x => x.tid == tid*/);
				pthread_mutex_unlock(&mutex_new_queue);

				pthread_mutex_lock(&mutex_multiprog);
				configuracion_suse.MAX_MULTIPROG --;
				pthread_mutex_unlock(&mutex_multiprog);

				list_remove_and_destroy_element(program->ULTS, tid);

		break;

		case 1: //NO ME RECONOCE READY

				list_remove(program->READY_LIST,tid);

				pthread_mutex_lock(&mutex_multiprog);
				configuracion_suse.MAX_MULTIPROG --;
				pthread_mutex_unlock(&mutex_multiprog);

				list_remove_and_destroy_element(program->ULTS, tid);

		break;

		case EXECUTE:

				list_remove(program->EXEC_LIST,tid);

				pthread_mutex_lock(&mutex_multiprog);
				configuracion_suse.MAX_MULTIPROG --;
				pthread_mutex_unlock(&mutex_multiprog);

				list_remove_and_destroy_element(program->ULTS, tid);

		break;

		}

		free(program);
		free(thread);

		return 1;
}

void handle_wait_sem(socket_actual, paquete_wait_sem){
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

void handle_signal_sem(socket_actual, paquete_signal_sem){

	esperar_handshake(socket_actual, paquete_signal_sem, cop_signal_sem);
	log_info(logger, "Recibiendo el semaforo para decrementar \n");

	t_paquete* paquete_recibido = recibir(socket_actual);
	int desplazamiento = 0;
	int tid = deserializar_int(paquete_recibido->data, &desplazamiento);
	char* sem = deserializar_string(paquete_recibido->data, &desplazamiento);
	liberar_paquete(paquete_recibido);

	pthread_mutex_lock(&mutex_semaforos);
	int resultado = incrementar_semaforo(tid, sem);
	if(resultado != -1){
		resultado = desbloquear_proceso_semaforo(sem);
	}
	pthread_mutex_unlock(&mutex_semaforos);

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;
	serializar_int(buffer, &desp, resultado);
	enviar(socket_actual, cop_wait_sem, tamanio_buffer, buffer);
	free(buffer);

}

int desbloquear_proceso_semaforo(sem){

	t_suse_semaforos* semaforo = list_find(configuracion_suse.semaforos, semaforo->NAME = sem);

	if(semaforo->BLOCKED_LIST[0] == NULL){
		return 0; //no hay procesos para desbloquear.
	}

	t_suse_thread* thread = semaforo->BLOCKED_LIST[0];
	t_program * program = configuracion_suse.programs[thread->procesoId];
	program->bloqueado = false;

	pthread_mutex_lock(&mutex_multiprog);

	if(configuracion_suse.ACTUAL_MULTIPROG < configuracion_suse.MAX_MULTIPROG){

		thread->estado = 1; //No me reconoce READY. ESTA DUPLICADO EN HILOLAY_INTERNAL. TODO: CAMBIAR LOS ENUMS.
		list_add(program->READY_LIST,thread);
		configuracion_suse.ACTUAL_MULTIPROG ++;
	}
	pthread_mutex_unlock(&mutex_multiprog);

	thread->estado = NEW;

	pthread_mutex_lock(&mutex_new_queue);
	list_add(new_queue,thread);
	pthread_mutex_unlock(&mutex_new_queue);

	list_remove(semaforo->BLOCKED_LIST,0);

	return 0;

}

int incrementar_semaforo(uint32_t tid, char* sem){

	t_suse_semaforos* semaforo = list_find(configuracion_suse.semaforos, semaforo->NAME = sem);

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

//todo definir esta funcion
int decrementar_semaforo(int socket_actual,int tid, char* sem){

	t_suse_semaforos* semaforo = list_find(configuracion_suse.semaforos, semaforo->NAME = sem);

	if(semaforo == NULL){
			log_info(logger,"El semaforo no existe\n");
			return -1;
		}
	if(semaforo->VALUE <= 0)
	{

		t_program* program = configuracion_suse.programs[socket_actual];
		program->bloqueado = true;
		t_suse_thread* thread = program->ULTS[tid];
		thread->estado = BLOCKED;
		list_add(semaforo->BLOCKED_LIST,thread);

	}

	semaforo->VALUE --;

	return 0;
}
