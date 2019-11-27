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
	exit_queue = list_create();

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
	log_info(logger, "Inicializando semaforos \n");

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
		log_info(logger, "Creando process con id... %d. \n", new_connection);
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
	log_info(logger, "Esperando primer handshake de hilolay...\n");
	esperar_handshake(socket_actual, paquete_hilolay, cop_handshake_hilolay_suse);

	log_info(logger, "Realice el primer handshake con hilolay.\n");

	t_paquete* paquete_recibido = recibir(socket_actual);

	int desplazamiento = 0;
	int main_tid = deserializar_int(paquete_recibido->data, &desplazamiento);


	t_process * process = generar_process(socket_actual);
	log_info(logger, "El  programa que se conecto es el de ID %d \n", process->PROCESS_ID);

	list_add(configuracion_suse.process,process);

	//Creo el main thread
	t_suse_thread * new_thread = malloc(sizeof(t_suse_thread));

	new_thread->tid = main_tid;
	new_thread->procesoId = socket_actual;
	new_thread->estado = E_NEW;

	list_add(process->ULTS,new_thread);
	list_add(new_queue, new_thread);

	log_info(logger, "El main thread tiene id %d \n", new_thread->tid);

	pthread_mutex_lock(&mutex_multiprog);

	log_info(logger, "El grado de multiprogramacion es %d, validando...", configuracion_suse.ACTUAL_MULTIPROG);
	if(configuracion_suse.ACTUAL_MULTIPROG + 1 <= configuracion_suse.MAX_MULTIPROG){
		nuevo_a_ejecucion(new_thread, socket_actual);
	}
	pthread_mutex_unlock(&mutex_multiprog);

	liberar_paquete(paquete_recibido);
}

void handle_suse_create(un_socket socket_actual, t_paquete* paquete_hilolay){
	log_info(logger, "Esperando handshake de suse_create...\n");

	//Obtengo el hilo
	esperar_handshake(socket_actual, paquete_hilolay, cop_suse_create);
	log_info(logger, "Realice handshake en suse_create \n");

	t_paquete* paquete_recibido = recibir(socket_actual);
	log_info(logger, "El programa que se conecto es el de ID %d \n", socket_actual);

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

	log_info(logger, "El grado de multiprogramacion es %d, validando...", configuracion_suse.ACTUAL_MULTIPROG);
	pthread_mutex_lock(&mutex_multiprog);
	if(validar_grado_multiprogramacion())
	{
		nuevo_a_listo(new_thread,process->PROCESS_ID);
		log_info(logger, "El thread %d del programa %d esta en ready", new_thread->tid, process->PROCESS_ID);
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
	log_info(logger, "Esperando handshake de suse_schedule_next...\n");
	esperar_handshake(socket_actual, paquete_next_tid, cop_next_tid);
	log_info(logger, "Realice el handhsake de suse_schedule_next");

	t_paquete* paquete_recibido = recibir(socket_actual); // Recibo hilo principal
	int desplazamiento = 0;
	int msg = deserializar_int(paquete_recibido->data, &desplazamiento);
	liberar_paquete(paquete_recibido);

	bool comparador(t_process* p)
	{
		return p->PROCESS_ID == socket_actual;
	}

	t_process* process = list_find(lista_de_process, comparador);

	log_info(logger, "Iniciando planificacion de programa %d...\n",process->PROCESS_ID);
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
	log_info(logger, "Ordenando cola de listos...");

	clock_t aux = clock(); //pasado a float, segundos. Es el momento en que termina de ejcutar

	t_suse_thread* exec_actual = process->EXEC_THREAD;

	float aux2 = exec_actual->duracionRafaga;

	exec_actual->duracionRafaga = aux2 - aux; //La diferencia entre cuando empezo y termino es lo q duro la rafaga.

	ordenar_cola_listos(process->READY_LIST);

	t_suse_thread *next_ULT = list_get(process->READY_LIST, 0); // TODO: Si no funciona hacemos una bool que haga return thread != null; va a retornar el primero q haya
	int next_tid = next_ULT->tid;
	log_info(logger, "El proximo a ejecutar es %d", next_tid);

	listo_a_ejecucion(next_ULT, process->PROCESS_ID);

	//actualizarRafaga(next_ULT);

	process->LAST_EXEC = next_ULT; //Esto para que sirve??

	next_ULT->duracionRafaga = aux; //Le pongo el momento de inicio de la rafaga.

	return next_tid;
}

void ordenar_cola_listos(t_list* ready_list) {
	estimar_ULTs_listos(ready_list);
	ordenar_por_sjf(ready_list);
}

void estimar_ULTs_listos(t_list* ready_list) {
	//TODO ver sino es void*
	log_info(logger, "Estimando ULTs...");
	void estimar(t_suse_thread* ULT) {
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
	log_info(logger, "Ordenando por SJF...");
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

//void actualizarRafaga(t_suse_thread * ULT) {
//
//	bool comparador(t_process* program)
//	{
//		return program->PROCESS_ID == ULT->procesoId;
//	}
//
//	t_process* process = list_find(lista_de_process,comparador);
//
//	if(ULT == process->LAST_EXEC)// TODO: Chequear si esa igualacion entre dos punteros funciona o hacemos una funcion de igualar los dos tid;
//	{
//		ULT->duracionRafaga++;
//	}
//	else
//	{
//		ULT->duracionRafaga = 1; //todo esto quizas deberia ser 0
//	}
//}


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

//////Funciones transicion de estados/////
void ejecucion_a_exit(t_suse_thread* thread, un_socket socket)
{

	bool buscador(t_process* program)
		{
			return program->PROCESS_ID == socket;
		}

	t_process* process = list_find(lista_de_process,buscador);

	remover_ULT_exec(process);
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

	t_process* process = list_find(lista_de_process,buscador);

	eliminar_ULT_cola_actual(thread,process);
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

	t_process* process = list_find(lista_de_process,buscador);

	eliminar_ULT_cola_actual(thread,process);
	thread->estado = E_EXIT;
	pthread_mutex_lock(&mutex_exit_queue);
	list_add(exit_queue,thread);
	pthread_mutex_unlock(&mutex_exit_queue);

	log_info(logger, "El tid %d paso de ready a exit\n", thread->tid);
}


void nuevo_a_exit(t_suse_thread* thread,un_socket socket_actual)
{
	bool buscador(t_process* program)
		{
			return program->PROCESS_ID == socket;
		}

	t_process* process = list_find(lista_de_process,buscador);

	eliminar_ULT_cola_actual(thread,process);

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

	t_process* process = list_find(lista_de_process,buscador);

	eliminar_ULT_cola_actual(thread,process);
	thread->estado = E_EXECUTE;
	thread->ejecutado_desde_estimacion = true;

	eliminar_ULT_cola_actual(process->EXEC_THREAD);
	ejecucion_a_listo(process->EXEC_THREAD,socket);

	process->EXEC_THREAD = thread;
	log_info(logger, "El tid %d paso de ready a execute\n", thread->tid);
}

void nuevo_a_ejecucion(t_suse_thread* thread, un_socket socket)
{
	bool buscador(t_process* process)
	{
		return process->PROCESS_ID == socket;
	}

	t_process* program = list_get(configuracion_suse.process,buscador);

	eliminar_ULT_cola_actual(thread,program);

	thread->duracionRafaga = clock(); //Pasado a int, segundos.

	thread->estado = E_EXECUTE;
	program->EXEC_THREAD = thread->tid;
	configuracion_suse.ACTUAL_MULTIPROG ++;

	log_info(logger, "El thread %d paso de new a execute \n", thread->tid);
}

void ejecucion_a_listo(t_suse_thread* thread, un_socket socket)
{
	t_process* process = configuracion_suse[socket];

	remover_ULT_exec(process);
	thread->estado = E_READY;
	process->EXEC_THREAD = NULL;
	list_add(process->READY_LIST,thread->tid);
	log_info(logger, "El thread %d paso de execute a ready \n", thread->tid);

}

void bloqueado_a_listo(t_suse_thread* thread,t_process* program)
{
	eliminar_ULT_cola_actual(thread,program);
	thread->estado = E_READY;
	list_add(program->READY_LIST,thread->tid);
	log_info(logger, "El thread %d paso de blocked a ready \n", thread->tid);
}


void ejecucion_a_bloqueado(t_suse_thread* thread,un_socket socket)
{
	t_process* process = configuracion_suse[socket];

	remover_ULT_exec(process);
	thread->estado = E_BLOCKED;
	process->EXEC_THREAD = NULL;
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

	remover_ULT_exec(program);

	thread->estado = E_BLOCKED;

	pthread_mutex_lock(&mutex_semaforos);
	list_add(semaforo->BLOCKED_LIST,thread);
	pthread_mutex_unlock(&mutex_semaforos);

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
	process->EXEC_THREAD = NULL;

}

void nuevo_a_listo(t_suse_thread* ULT, int process_id)
{
	bool comparador(t_process* process)
	{
		return process->PROCESS_ID == process_id;
	}

	t_process* program = list_find(configuracion_suse->process, comparador);

	list_add(program->READY_LIST, ULT->tid);

	eliminar_ULT_cola_actual(ULT, program);
	ULT->estado = E_READY;
	configuracion_suse.ACTUAL_MULTIPROG ++;
}

void remover_ULT_listo(t_suse_thread* thread,t_process* process)
{

	bool comparador(int tid){
		return tid == thread->tid;
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
