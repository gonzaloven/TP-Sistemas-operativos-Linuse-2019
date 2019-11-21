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
	ready_queue = list_create();
	blocked_queue = list_create();

	pthread_mutex_init(&mutex_new_queue, NULL);
	pthread_mutex_init(&mutex_ready_queue, NULL);
	pthread_mutex_init(&mutex_blocked_queue, NULL);

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
	t_paquete* received_packet = recibir(socket_actual);
	switch(received_packet->codigo_operacion){
		case cop_handshake_hilolay_suse:
			handle_hilolay(socket_actual, received_packet);
		break;

		case cop_next_tid:
			handle_next_tid(socket_actual, received_packet);

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
	sprintf("el socket es", "%d", program->PROGRAM_ID);
	list_add(new_queue, master_tid); //todo ver si en la lista agrego programas o hilos
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
