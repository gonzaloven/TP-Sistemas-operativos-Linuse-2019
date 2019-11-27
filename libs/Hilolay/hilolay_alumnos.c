#include "hilolay_alumnos.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <utils/network.h>
#include <utils/message.h>

//todo no se si cada vez que esperamos una respuesta hay que fijarse de que cop viene por las dudas
static struct hilolay_operations hiloops = {
		.suse_create = &suse_create,
		.suse_schedule_next = &suse_schedule_next,
		//.suse_join = &suse_join, //todo hacer el join desde hilolay
		.suse_close = &suse_close,
		.suse_wait = &suse_wait,
		.suse_signal = &suse_signal

};

int socket_suse = 0; //todo verificar si hay que conectarse aca en la global o basta con hacerlo en la funcion
int master_thread = 0;

void hilolay_init(void){
	log_info(logger, "Ejecutando hilolay_init... \n");
	init_internal(&hiloops);
	conectar_con_suse();
}

void conectar_con_suse() {
	socket_suse = conectar_a(configuracion_hilolay.SUSE_IP ,configuracion_hilolay.SUSE_PORT);
	log_info(logger, "Me conecte con SUSE por el socket %d. \n", socket_suse);

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;

	serializar_int(buffer, &desp, 0);
	enviar(socket_suse, cop_handshake_hilolay_suse, tamanio_buffer, buffer);
	log_info(logger, "Envie un mensaje (0) a SUSE. \n");

}

int suse_create(int master_thread){
	log_info(logger, "Ejecutando suse_create... \n");
	bool result = realizar_handshake(socket_suse, cop_suse_create);

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;

	serializar_int(buffer, &desp, master_thread);
	enviar(socket_suse, cop_generico, tamanio_buffer, buffer); //todo esto no se si diferencia el main thread
	free(buffer);
	log_info(logger, "Envie el tid %d a suse. \n");
	log_info(logger, "La respuesta de suse_create es %d \n", (int)result);

	return (int)result; //todo responder correctamente

}

int suse_schedule_next(){ //todo testear
	log_info(logger, "Ejecutando suse_schedule_next %d... \n");
	int next;
	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;
	int msg = 1; //este mensaje es solo para preguntarle que thread se va a ejecutar

	serializar_int(buffer, &desp, msg);
	enviar(socket_suse, cop_next_tid, tamanio_buffer, buffer);
	log_info(logger, "Envie un mensaje (0) a SUSE. \n");
	free(buffer);

	t_paquete* received_packet = recibir(socket_suse);
	int desplazamiento = 0;

	if(received_packet->codigo_operacion == cop_next_tid){
		next = deserializar_int(received_packet->data, &desplazamiento);
		log_info(logger, "Recibi el next_tid %d. \n", next);
	}
	else{
		next = -1;
		log_info("El codigo de operacion es incorrecto, deberia ser cop_next_tid y es %d", received_packet->codigo_operacion)
	}

	liberar_paquete(received_packet);

	return next;
}

int suse_close(int tid){
	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desplazamiento = 0;
	serializar_int(buffer, &desplazamiento, tid);
	enviar(socket_suse, cop_close_tid, tamanio_buffer, buffer);
	free(buffer);
	log_info(logger, "Enviando a cerrar el thread %d. \n", tid);

	//SUSE me devuelve una respuesta indicando si logro cerrarlo o no
	t_paquete* received_packet = recibir(socket_suse);
	int desp = 0;
	int result = deserializar_int(received_packet->data, &desp);
	liberar_paquete(received_packet);

	return result;
}

int suse_join(int tid)
{
	int tamanio_buffer = sizeof(int);
		void * buffer = malloc(tamanio_buffer);
		int desplazamiento = 0;
		serializar_int(buffer, &desplazamiento, tid);
		enviar(socket_suse, cop_suse_join, tamanio_buffer, buffer);
		free(buffer);

		t_paquete* received_packet = recibir(socket_suse);
		int desp = 0;
		int result = deserializar_int(received_packet->data, &desp);
		liberar_paquete(received_packet);

		return result;
}


int suse_wait(int tid, char *sem_name){ //todo desde suse en la estructur agregar hilo + semaforos del mismo

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desplazamiento = 0;
	serializar_int(buffer, &desplazamiento, tid);
	serializar_valor(sem_name); //todo ver si el sem_name va con & o no
	enviar(socket_suse, cop_wait_sem, tamanio_buffer, buffer);¿
	free(buffer);
	log_info(logger, "Enviando a hacer un wait del sem %s del thread %d. \n", sem_name, tid);

	//repuesta de suse que decremento el semaforo ok
	t_paquete* received_packet = recibir(socket_suse);
	int desp = 0;
	int result = deserializar_int(received_packet->data, &desp);
	liberar_paquete(received_packet);
	return result;
}


int suse_signal(int tid, char *sem_name){

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desplazamiento = 0;

	serializar_int(buffer, &desplazamiento, tid);
	serializar_valor(sem_name); //todo ver si el sem_name va con & o no
	enviar(socket_suse, cop_signal_sem, tamanio_buffer, buffer);
	free(buffer);
	log_info(logger, "Enviando a hacer un signal del sem %s del thread %d. \n", sem_name, tid);

	t_paquete* received_packet = recibir(socket_suse);

	int desp = 0;
	int result = deserializar_int(received_packet->data, &desp);

	liberar_paquete(received_packet);

	return result;
}


hilolay_alumnos_configuracion get_configuracion_hilolay() {
	log_info(logger,"Levantando archivo de configuracion de Hilolay alumnos \n");
	hilolay_alumnos_configuracion configuracion_hilolay;
	t_config*  archivo_configuracion = config_create(HILOLAY_ALUMNO_CONFIG_PATH);
	configuracion_hilolay.SUSE_IP = copy_string(get_campo_config_string(archivo_configuracion, "SUSE_IP"));
	configuracion_hilolay.SUSE_PORT = get_campo_config_int(archivo_configuracion, "SUSE_PORT");

	config_destroy(archivo_configuracion);
	return configuracion_hilolay;
}


