#include "hilolay_alumnos.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <utils/network.h>
#include <utils/message.h>

static struct hilolay_operations hiloops = {
		.suse_create = &suse_create,
		/*
		.suse_schedule_next = &suse_schedule_next,
		.suse_join = &suse_join,
		.suse_close = &suse_close,
		.suse_wait = &suse_wait,
		.suse_signal = &suse_signal
		*/
};

int socket_suse = 0;
int master_thread = 0;

void hilolay_init(void){
	init_internal(&hiloops);
	conectar_con_suse();
	//todo el orden importa?
}

void conectar_con_suse() {
	socket_suse = conectar_a(configuracion_hilolay.SUSE_IP ,configuracion_hilolay.SUSE_PORT);
	log_info(logger, "Me conecte con SUSE. \n");
	//TODO ver si con esto basta o tengo que mandar un mensaje
}

int suse_create(int master_thread){ //todo testear que los mensajes se manden of
	bool result = realizar_handshake(socket_suse, cop_handshake_hilolay_suse);

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;

	serializar_int(buffer, &desp, master_thread);
	enviar(socket_suse, cop_generico, tamanio_buffer, buffer); //todo el cop generico estara ok?
	free(buffer);
	log_info(logger, "Envie el tid a suse. \n");

	return (int)result; //todo responder correctamente o castear la funcion a void

}

int suse_schedule_next(){ //todo testear
	int next;
	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;
	int msg = 1; //este mensaje es solo para preguntarle que thread se va a ejecutar

	serializar_int(buffer, &desp, msg);
	enviar(socket_suse, cop_next_tid, tamanio_buffer, buffer);
	free(buffer);

	t_paquete* received_packet = recibir(socket_suse);
	int desplazamiento = 0;

	if(received_packet->codigo_operacion == cop_next_tid){
		next = deserializar_int(received_packet->data, &desplazamiento);
	}
	else{
		next = -1;
		log_info("El codigo de operacion es incorrecto, deberia ser cop_next_tid y es %d", cop_next_tid) //Aca no deberia ser received_packet->codigo_operacion??
	}

	return next;
}

void handle_conection_hilolay() {
	t_paquete* received_packet = recibir(socket_suse);
	switch(received_packet->codigo_operacion){
		case cop_next_tid:
			received_packet->data
	}
}


hilolay_alumnos_configuracion get_configuracion_hilolay() {
	log_info(logger,"Levantando archivo de configuracion de Hilolay \n");
	hilolay_alumnos_configuracion configuracion_hilolay;
	t_config*  archivo_configuracion = config_create(HILOLAY_ALUMNO_CONFIG_PATH);
	configuracion_hilolay.SUSE_IP = copy_string(get_campo_config_string(archivo_configuracion, "SUSE_IP"));
	configuracion_hilolay.SUSE_PORT = get_campo_config_int(archivo_configuracion, "SUSE_PORT");

	config_destroy(archivo_configuracion);
	return configuracion_hilolay;
}


