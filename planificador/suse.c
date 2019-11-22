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
	sprintf("Iniciando suse");
	char* log_path = "../logs/suse_logs.txt";

	logger = log_create(log_path, "SUSE Logs", 1, 1);
	log_info(logger, "Inicializando SUSE. \n");

	configuracion_suse = get_configuracion();
	log_info(logger, "Archivo de configuracion levantado. \n");
	
	new_queue = list_create();
	ready_queue = list_create(); //Esta va a pasar a ser una por programa que se conecte
	blocked_queue = list_create(); //Esta va a pasar a ser una por programa que se conecte

	pthread_mutex_init(&mutex_new_queue, NULL);
	pthread_mutex_init(&mutex_ready_queue, NULL);
	pthread_mutex_init(&mutex_blocked_queue, NULL);
	pthread_mutex_init(&mutex_multiprog, NULL);

	iniciar_servidor();

	return EXIT_SUCCESS;
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


void handle_conection_suse(int socket_actual) { //Aca supongo que en cada case habria que crear un hilo de atencion para atender pedidos concurrentes, un hilo por programa que se conecte
	t_paquete* received_packet = recibir(socket_actual);
	switch(received_packet->codigo_operacion){
		case cop_handshake_hilolay_suse:
			handle_hilolay(socket_actual, received_packet);
		break;

		case cop_next_tid:
			handle_next_tid(socket_actual, received_packet);
		break;
		case cop_close_tid:
			handle_close_tid(socket_actual,received_packet);
		break;

	}
}

void handle_hilolay(un_socket socket_actual, t_paquete* paquete_hilolay) {

	esperar_handshake(socket_actual, paquete_hilolay, cop_handshake_hilolay_suse);
	log_info(logger, "Realice handshake con hilolay\n");
	sprintf("el socket es", "%d", socket_actual);

	t_paquete* paquete_recibido = recibir(socket_actual); // Recibo hilo principal
	int desplazamiento = 0;
	int master_tid = deserializar_int(paquete_recibido->data, &desplazamiento);

	t_program * program = generar_programa(socket_actual);
	sprintf("el socket es", "%d", program->PROGRAM_ID); //El programa solo hay que generarlo cuando es el primer handshake. podria ser algo como lo siguiente.

/*	t_program * program =list_find(configuracion_suse.programs, x => x.PROGRAM_ID == socket_actual); //no se como pasarle una condicion booleana como parametro.
 *
	if(program == NULL)
	{
		t_program * program = generar_programa(socket_actual);
		sprintf("el socket es", "%d", program->PROGRAM_ID);
	}
	else
	{
		t_suse_thread * new_thread = malloc(sizeof(t_suse_thread));
		new_thread->tid = master_tid;
		new_thread->estado = NEW;
		list_add(program->ULTS,new_thread);
		list_add(new_queue,new_thread->tid);
	}

	free(program);
*/
	list_add(new_queue, master_tid); //todo ver si en la lista agrego programas o hilos
	liberar_paquete(paquete_recibido);

}

void handle_close_tid(un_socket socket_actual, t_paquete* received_package){

	esperar_handshake(socket_actual, received_package, cop_close_tid);
	log_info(logger, "Realice handshake con hilolay\n");
	sprintf("el socket es", "%d", socket_actual);

	t_paquete* paquete_recibido = recibir(socket_actual); // Recibo hilo a finalizar
	int desplazamiento = 0;
	int tid = deserializar_int(paquete_recibido->data, &desplazamiento);
	close_tid(tid);
	liberar_paquete(paquete_recibido);
}

t_program * generar_programa(un_socket socket) {
	t_program * program = malloc(sizeof(t_ESI));
	program->PROGRAM_ID = (int)socket; //todo ver si el socket solo es un int o hay que castear
	return program;
}

void handle_next_tid(un_socket socket_actual, received_packet){
	esperar_handshake(socket_actual, received_packet, cop_next_tid);
	t_paquete* paquete_recibido = recibir(socket_actual); // Recibo hilo principal
	int desplazamiento = 0;
	int msg = deserializar_int(paquete_recibido->data, &desplazamiento);
	liberar_paquete(received_packet);

	int next_tid = get_next_tid();

	// Envio el next tid
	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;
	enviar(socket_actual, cop_next_tid, tamanio_buffer, buffer);
	free(buffer);
}

int get_next_tid(){

	//todo planificar

}
void close_tid(int tid){

		t_program* program = list_find(configuracion_suse->programs, true /*x => x.PROGRAM_ID == socket_actual*/); //pasarle bien la condicion

		t_suse_thread * thread = list_find(program->ULTS,true /*x => x.tid = tid*/); //no se si no falta malloc de thread.

		//IF THREAD IS NULL CREO Q SE TENIAN Q LIBERAR RECURSOS.. Supongo q es desconectar al programa (chau socket) ṕero como no estoy seguro ni se bien como hacerlo no lo hago

		switch(thread->estado){

		case NEW:
				pthread_mutex_lock(&mutex_new_queue);
				list_remove(new_queue,true/*x => x.tid == tid*/);
				pthread_mutex_unlock(&mutex_new_queue);

				pthread_mutex_lock(&mutex_multiprog);
				configuracion_suse.MAX_MULTIPROG --;
				pthread_mutex_unlock(&mutex_multiprog);

				//thread->estado = EXIT; O SINO list_remove_and_destroy_by_condition(program->ULTS, x.tid = tid);
				//creo q no nos hace falta manejar un estado exit, pero bueno pongo las opciones
				/*LO QUE PODRIAMOS PENSAR PARA NO ANDAR PONIENDO CONDICIONES, ES QUE SI LOS TID SON ENTEROS...
				 *USARLOS COMO INDICES DENTRO DE LA LISTA, ENTONCES EL THREAD CON TID 5 VA A ESTAR EN LA POSICION 5 DE LA LISTA
				 *USARLOS Y NO VAN A HACER FALTA ESAS CONDICIOPNES. AVISAME Y LO HAGO*/
		break;

		//Supongo que la estructura t_program se le va a agregar las dos listas de ready y exec, una vez hecho eso se removeria el thread de ahi.
		}

		free(program);
		free(thread);
}
