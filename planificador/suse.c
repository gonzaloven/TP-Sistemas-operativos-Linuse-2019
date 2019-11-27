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
	blocked_queue = list_create();
	exit_queue = list_create(); //todo ver si es necesario

	sem_init(&sem_ULTs_listos, 0, 0);

	pthread_mutex_init(&mutex_new_queue, NULL);
	pthread_mutex_init(&mutex_blocked_queue, NULL);
	pthread_mutex_init(&mutex_exit_queue,NULL);
	pthread_mutex_init(&mutex_multiprog, NULL);
	pthread_mutex_init(&mutex_semaforos,NULL);
	pthread_mutex_init(&mutex_lista_de_process, NULL);

	iniciar_servidor();

	list_destroy(new_queue);
	list_destroy(blocked_queue);
	list_destroy(exit_queue);

	free(mutex_new_queue);
	free(mutex_blocked_queue);
	free(mutex_multiprog);
	free(mutex_semaforos);

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
		log_info(logger, "Soy SUSE y recibi una nueva conexion del socket %d. \n", new_connection);
		free(handshake);
		//Creo un hilo por programa que se conecta
		thread_params = list_add(thread_params, new_connection);
		nuevo_hilo(process_conectado_funcion_thread, thread_params);
	}
}

void* process_conectado_funcion_thread(void* argumentos) {
	un_socket socket_actual = list_get(argumentos, 0);
	list_destroy(argumentos);
	handle_conection_suse(socket_actual);
	pthread_detach(pthread_self()); //todo verificar si esto esta ok por lo de utn.so
	return NULL;
}

void handle_conection_suse(un_socket socket_actual) {

	t_paquete* paquete_recibido = recibir(socket_actual);

	switch(paquete_recibido->codigo_operacion){
		case cop_handshake_hilolay_suse:
			handle_hilolay(socket_actual, paquete_recibido);
		break;
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

	}
	liberar_paquete(paquete_recibido);
}

void handle_suse_join(un_socket socket_actual, t_paquete * paquete_recibido){

	esperar_handshake(socket_actual, paquete_recibido, cop_suse_join);

	log_info(logger, "Realice el primer handshake con hilolay\n");
	sprintf("el socket es", "%d", socket_actual);

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

	log_info(logger, "Realice el primer handshake con hilolay\n");
	sprintf("el socket es", "%d", socket);

	t_paquete* paquete = recibir(socket);

	int desplazamiento = 0;
	int tid = deserializar_int(paquete->data, &desplazamiento);

	int resultado = close_tid(tid,socket);

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;

	serializar_int(buffer, &desp, resultado);
	enviar(socket, cop_wait_sem, tamanio_buffer, buffer);

	free(buffer);
	liberar_paquete(paquete);
}

int join(un_socket socket, int tid){

	bool buscador(t_suse_thread* thread){
		return thread->tid == tid;
	}

	t_process* process = configuracion_suse.process[socket];
	t_suse_thread* thread_executing = list_get(process->ULTS,process->EXEC_THREAD);
	t_suse_thread* thread_joined = list_find(process->ULTS,buscador);

	if(thread_joined == NULL)
	{
		return 0;
	}

	list_add(thread_executing->joinTo,thread_joined);
	list_add(thread_joined->joinedBy,thread_executing);

	ejecucion_a_bloqueado(thread_executing,socket);

	return 0;

}

void handle_hilolay(un_socket socket_actual, t_paquete* paquete_hilolay) {
	esperar_handshake(socket_actual, paquete_hilolay, cop_handshake_hilolay_suse);

	log_info(logger, "Realice el primer handshake con hilolay\n");

	t_paquete* paquete_recibido = recibir(socket_actual);

	int desplazamiento = 0;
	int main_tid = deserializar_int(paquete_recibido->data, &desplazamiento);

	//Genero el process
	t_process * process = generar_process(socket_actual);

	list_add(configuracion_suse.process,process);

	sprintf("el socket es", "%d", process->PROCESS_ID);

	//Creo el main thread
	t_suse_thread * new_thread = malloc(sizeof(t_suse_thread));

	new_thread->tid = main_tid;
	new_thread->procesoId = socket_actual;
	new_thread->estado = E_NEW;

	list_add(process->ULTS,new_thread);
	list_add(new_queue, new_thread);

	pthread_mutex_lock(&mutex_multiprog);

	if(validar_grado_multiprogramacion()){
		nuevo_a_ejecucion(new_thread, socket_actual);
	}
	pthread_mutex_unlock(&mutex_multiprog);

	liberar_paquete(paquete_recibido);
}

void handle_suse_create(un_socket socket_actual, t_paquete* paquete_hilolay){

	//Obtengo el hilo
	esperar_handshake(socket_actual, paquete_hilolay, cop_suse_create);
	log_info(logger, "Esperando hilo de suse_create \n");
	sprintf("el socket es", "%d", socket_actual);

	t_paquete* paquete_recibido = recibir(socket_actual);

	int desplazamiento = 0;
	int new_tid = deserializar_int(paquete_recibido->data, &desplazamiento);

	log_info(logger, "Recibi el hilo", "%d", new_tid);

	bool find_process_by_id(t_process* process) //TODO: Extraer a utils!!!!!!!!!!!!!!!!!!!!!!!!!
	{
		return process->PROCESS_ID == socket_actual;
	}

	t_process* process = list_find(configuracion_suse.process,find_process_by_id);

	t_suse_thread * new_thread = malloc(sizeof(t_suse_thread));

	new_thread->tid = new_tid;
	new_thread->estado = E_NEW;
	new_thread->procesoId = process->PROCESS_ID;
	new_thread->duracionRafaga = 0;
	new_thread->ejecutado_desde_estimacion = false;

	list_add(process->ULTS,new_thread);
	list_add(new_queue,new_thread);

	pthread_mutex_lock(&mutex_multiprog);

	if(validar_grado_multiprogramacion())
	{
		nuevo_a_listo(new_thread,process->PROCESS_ID);
	}

	pthread_mutex_unlock(&mutex_multiprog);

	liberar_paquete(paquete_recibido);
}

int close_tid(int tid, int socket_actual){

	bool find_process_by_id(t_process* process) //TODO: Extraer a utils!!!!!!!!!!!!!!!!!!!!!!!!!
	{
		return process->PROCESS_ID == socket_actual;
	}
	bool find_thread_by_tid(t_suse_thread* thread) //Extraer a utils!!!!!!!!!!!!!!!!!!!!
	{
		return thread->tid == tid;
	}

	t_process* process = list_find(configuracion_suse->process,find_process_by_id);
	t_suse_thread* thread = list_find(process->ULTS,find_thread_by_tid);

	if(thread == NULL)
	{

		 list_destroy_and_destroy_elements(process->ULTS);
		 list_remove_and_destroy_element(configuracion_suse->process,process);
		 int resultado = close(socket_actual);
		 return resultado;
	}

	if(!list_is_empty(thread->joinedBy))
	{
		desjoinear(process,thread);
	}

	switch(thread->estado){
		case E_NEW:

				bool comparador_new(t_suse_thread* element){
					return element->tid == tid && element->procesoId ==socket_actual;
				}

				pthread_mutex_lock(&mutex_new_queue);
				nuevo_a_exit(thread,socket_actual);
				pthread_mutex_unlock(&mutex_new_queue);
		break;

		case E_BLOCKED:
			bool comparador_blocked(t_suse_thread* element)
			{
				return element->tid == tid && element->procesoId ==socket_actual;
			}
			pthread_mutex_lock(&mutex_blocked_queue);
			pthread_mutex_lock(&mutex_multiprog);

			configuracion_suse.MAX_MULTIPROG --;
			bloqueado_a_exit(thread,socket_actual);

			pthread_mutex_unlock(&mutex_multiprog);
			pthread_mutex_unlock(&mutex_blocked_queue);
		break;

		case E_READY:

			bool comparador_ready(t_suse_thread* th){
				return th->tid == thread->tid;
			}

			pthread_mutex_lock(&mutex_multiprog);

			configuracion_suse.MAX_MULTIPROG --;
			listo_a_exit(thread,socket_actual);

			pthread_mutex_unlock(&mutex_multiprog);
		break;

		case E_EXECUTE:
				pthread_mutex_lock(&mutex_multiprog);
				process->EXEC_THREAD = NULL;
				ejecucion_a_exit(thread,socket_actual);
				configuracion_suse.MAX_MULTIPROG --;
				pthread_mutex_unlock(&mutex_multiprog);
		break;
	}

	return 1;
}

void desjoinear(t_process* process, t_suse_thread* thread_joineado)
{

	int size = list_size(thread_joineado->joinedBy);

	for(int i = 0; i < size; i++)
	{
		desjoinear_hilo(thread_joineado,thread_joineado->joinedBy[i]);
	}

}

void desjoinear_hilo(t_suse_thread* thread_joineado, t_suse_thread* thread_joiner)
{
	bool comparador(t_suse_thread* th){
		return th->tid == thread_joineado->tid;
	}

	list_remove(thread_joiner->joinTo,comparador);

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
			configuracion_suse.ACTUAL_MULTIPROG ++;
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


t_process * generar_process(un_socket socket) {
	t_process * process = malloc(sizeof(t_process));
	process->PROCESS_ID = (int)socket; //todo ver si el socket solo es un int o hay que castear
	return process;
}

//t_process* process_por_id(int process_id){
//	bool encontrar_process(void* process){
//		return ((t_process*)process)->PROCESS_ID == process_id;
//	}
//	pthread_mutex_lock(&mutex_lista_de_process);
//	t_process* result = list_find(lista_de_process, encontrar_process);
//	pthread_mutex_unlock(&mutex_lista_de_process);
//	return result;
//}

void handle_next_tid(un_socket socket_actual, paquete_next_tid){
	esperar_handshake(socket_actual, paquete_next_tid, cop_next_tid);
	t_paquete* paquete_recibido = recibir(socket_actual); // Recibo hilo principal
	int desplazamiento = 0;
	int msg = deserializar_int(paquete_recibido->data, &desplazamiento);
	liberar_paquete(paquete_recibido);

	bool comparador(t_process* p)
	{
		return p->PROCESS_ID == socket_actual;
	}

	t_process* process = list_find(lista_de_process,comparador);

	log_info(logger, "Iniciando planificacion...\n");
	int next_tid = obtener_proximo_ejecutar(process); //Inicia planificacion

	// Envio el next tid
	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;
	serializar_int(buffer, &desp, next_tid);
	enviar(socket_actual, cop_next_tid, tamanio_buffer, buffer);
	free(buffer);
}

int obtener_proximo_ejecutar(t_process* process){

	ordenar_cola_listos(process->READY_LIST);

	t_suse_thread next_ULT = list_get(process->READY_LIST, 0); // TODO: Si no funciona hacemos una bool que haga return thread = null; va a retornar el primero q haya
	int next_tid = next_ULT.tid;

	listo_a_ejecucion(next_ULT, process->PROCESS_ID);

	actualizarRafaga(next_ULT);
	process->LAST_EXEC = next_ULT;
	return next_tid;
}

void ordenar_cola_listos(t_list* ready_list) {
	estimar_ULTs_listos(ready_list);
	ordenar_por_sjf(ready_list);
}

void estimar_ULTs_listos(t_list* ready_list) {
	void estimar(t_suse_thread* ULT) { //todo espero un int para encontrar el hilo
		if (ULT->ejecutado_desde_estimacion) {
			ULT->ejecutado_desde_estimacion = false;
			estimar_rafaga(ULT);
		}
	}
	list_iterate(ready_list, estimar);
}

void estimar_rafaga(t_suse_thread * ULT){
	int rafaga_anterior = ULT->duracionRafaga; //Duracion de la rafaga anterior
	float estimacion_anterior = ULT->estimacionUltimaRafaga; // Estimacion anterior
	//todo ver si el tipo de dato esta ok
	float estimacion = configuracion_suse.ALPHA_SJF * rafaga_anterior + (1 - configuracion_suse.ALPHA_SJF) * estimacion_anterior;
	ULT->estimacionUltimaRafaga = estimacion;
}

void ordenar_por_sjf(t_list* ready_list){
	list_sort(ready_list, funcion_SJF);
}

bool funcion_SJF(t_suse_thread* ULT1, t_suse_thread* ULT2) {

	int process_id = ULT1->procesoId;

	bool comparador(t_process* p)
	{
		return p->PROCESS_ID == process_id;
	}

	t_process* process = list_find(lista_de_process,comparador);

	if (ULT1->estimacionUltimaRafaga == ULT2->estimacionUltimaRafaga) {
		return list_get(process->ULTS, 0); //todo ver como obtener el primer elemento no vacio de la lista // Si no funciona hacemos una bool que haga return thread = null; va a retornar el primero q haya
	}
	return ULT1->estimacionUltimaRafaga < ULT2->estimacionUltimaRafaga;
}

void actualizarRafaga(t_suse_thread * ULT) {

	bool comparador(t_process* program)
	{
		return program->PROCESS_ID == ULT->procesoId;
	}

	t_process* process = list_find(lista_de_process,comparador);

	if(ULT == process->LAST_EXEC)// TODO: Chequear si esa igualacion entre dos punteros funciona o hacemos una funcion de igualar los dos tid;
	{
		ULT->duracionRafaga++;
	}
	else
	{
		ULT->duracionRafaga = 1; //todo esto quizas deberia ser 0
	}
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
		resultado = desbloquear_hilos_semaforo(sem); //todo esto no serian hilos en vez de procesos?
	}
	pthread_mutex_unlock(&mutex_semaforos);

	int tamanio_buffer = sizeof(int);
	void * buffer = malloc(tamanio_buffer);
	int desp = 0;
	serializar_int(buffer, &desp, resultado);
	enviar(socket_actual, cop_wait_sem, tamanio_buffer, buffer);
	free(buffer);

}

int desbloquear_hilos_semaforo(sem){

	bool comparador(t_suse_semaforos* semaforo){
		return semaforo->NAME == sem;
	}
	t_suse_semaforos* semaforo = list_find(configuracion_suse.semaforos, comparador);

	if(semaforo->BLOCKED_LIST[0] == NULL){
		return 0;
	}

	t_suse_thread* thread = semaforo->BLOCKED_LIST[0];
	t_process * process = configuracion_suse.process[thread->procesoId];

	bloqueado_a_listo(thread,process);

	list_remove(semaforo->BLOCKED_LIST,0);

	return 0;
}

int incrementar_semaforo(uint32_t tid, char* sem){

	bool comparador(t_suse_semaforos* semaforo){
		return semaforo->NAME == sem;
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
		return semaforo->NAME == sem_name;
	}

	t_suse_semaforos* semaforo = list_find(configuracion_suse.semaforos, comparador);

	if(semaforo == NULL){
			log_info(logger,"El semaforo no existe\n");
			return -1;
		}
	if(semaforo->VALUE <= 0)
	{

		ejecucion_a_bloqueado_por_semaforo(tid,socket_actual,semaforo);

	}

	semaforo->VALUE --;

	return 0;
}
