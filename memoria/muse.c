/*
 * De la manera en como está implementado, muse.c lo que hace es levantar el servidor central,
 * levanta los loggers y el archivo de configuración, se queda escuchando y esperando a los
 * clientes y dependiendo que funciones le manden, muse se encargará de llamar a dichas funciones,
 * toda la lógica de estas funciones está en main_memory.c 
 */

#include "muse.h"

int tamDataFunction(Function f){
	int tamano = 0;
	tamano+= sizeof(uint8_t);
	tamano+= sizeof(uint8_t);
	for(int y=0; y < f.num_args; y++){
		tamano+= sizeof(uint8_t);
		tamano+= sizeof(uint32_t);
		tamano+= f.args[y].size;
	}
	return tamano;
}

t_log *muse_logger = NULL;
muse_configuration *muse_config = NULL;

int main(int argc,char *argv[])
{
	signal(SIGINT,muse_stop_service);
	muse_start_service(handler); 
	return 0;
}

void muse_start_service(ConnectionHandler ch)
{
	muse_config = load_configuration(MUSE_CONFIG_PATH);
	muse_logger = log_create(MUSE_LOG_PATH,"MUSE",true,LOG_LEVEL_TRACE);
	muse_main_memory_init(muse_config->memory_size,muse_config->page_size,muse_config->swap_size);
	server_start(muse_config->listen_port,ch);
	//Va a funcionar hasta que le manden un ctrl+c
}

void muse_stop_service()
{
	log_info(muse_logger,"SIGINT received. Shuting down!");	
	free(muse_config);
	log_destroy(muse_logger);
	muse_main_memory_stop();
	server_stop();
	printf("Thanks for using MUSE, goodbye!\n");
}

void liberarMemoria(Function* f){
	switch(f->type){

	case FUNCTION_GET:
	case FUNCTION_MAP:
		free(f->args[0].value.val_charptr);
		f->args[0].value.val_charptr = NULL;
		break;
	case FUNCTION_COPY:
		free(f->args[1].value.val_voidptr);
		f->args[1].value.val_voidptr = NULL;
		break;
	default:
		break;
	}
}

void* handler(void *args)
{
	ConnectionArgs *conna = (ConnectionArgs *)args;
	Message msg;
	char buffer[5000];
	int n=0;
	int socket = conna->client_fd;
	struct sockaddr_in client_address = conna->client_addr;
	
	log_debug(muse_logger,"A client in socket: %d has connected!",socket);

	int pid = NULL;

	while((n=receive_packet(socket,buffer,1024)) > 0)
	{
		if((n = message_decode(buffer,n,&msg)) > 0)
		{
			message_handler(&msg,socket);
			pid = msg.header.caller_id;
			memset(buffer, '\0', 5000);
		}else{
			free(msg.data);
			msg.data = NULL;
		}
	}
	if(pid != NULL)
		el_cliente_se_tomo_el_palo(pid);
	log_error(muse_logger,"The client in socket: %d was disconnected!",socket);
	close(socket);

	return (void*)NULL;
}

muse_configuration *load_configuration(char *path)
{
	t_config *config = config_create(path);
	muse_configuration *configuracion = (muse_configuration *)malloc(sizeof(muse_configuration)); 

	if(config == NULL)
	{
		log_error(muse_logger,"Configuration couldn't be loaded.Quitting program!");
		muse_stop_service();
		exit(-1);
	}
	
	configuracion->listen_port = config_get_int_value(config,"LISTEN_PORT");
	configuracion->memory_size = config_get_int_value(config,"MEMORY_SIZE");
	configuracion->page_size = config_get_int_value(config,"PAGE_SIZE");
	configuracion->swap_size = config_get_int_value(config,"SWAP_SIZE");
	config_destroy(config);
	return configuracion;
}

void message_handler(Message *m,int socket)
{
	uint32_t res= 0;
	Message msg;
	MessageHeader head;
	void* res_get;
	switch(m->header.message_type)
	{
		case MESSAGE_CALL:
			if(((Function*)m->data)->type == FUNCTION_GET){
				res_get = (void*) muse_invoke_function((Function *)m->data,m->header.caller_id);

				Function f;
				f.type = RTA_FUNCTION_GET;
				f.num_args = 1;
				f.args[0].type = VAR_VOID_PTR;
				f.args[0].size = ((Function *)m->data)->args[2].value.val_u32;
				f.args[0].value.val_voidptr = res_get;

				create_message_header(&head,MESSAGE_CALL,1,tamDataFunction(f));
				create_function_message(&msg,&head,&f);
				send_message(socket,&msg);

			}else{
				res = (int)muse_invoke_function((Function *)m->data,m->header.caller_id);
				//log_trace(muse_logger,"Call received!");
				message_free_data(m);

				create_message_header(&head,MESSAGE_FUNCTION_RET,2,sizeof(uint32_t));
				create_response_message(&msg,&head,res);
				send_message(socket,&msg);
				message_free_data(&msg);
			}

			break;
		default:
			log_error(muse_logger,"Undefined message");
			break;
	}
	return;
}


/*
function vendria a ser la funcion que libmuse quiere ejecutar, 
pid es el process_id de libmuse
*/
void* muse_invoke_function(Function *function,uint32_t pid)
{
	uint32_t func_ret = 0;
	void* funcion_ret;
	switch(function->type)
	{
		case FUNCTION_MALLOC:
			log_debug(muse_logger,"Malloc called with args -> %d bytes",function->args[0].value.val_u32);
			func_ret = memory_malloc(function->args[0].value.val_u32,pid);
			break;
		case FUNCTION_FREE:
			log_debug(muse_logger,"Free called");
			func_ret = memory_free(function->args[0].value.val_u32,pid);
			break;
		case FUNCTION_GET:
			log_debug(muse_logger,"Get called");
			return memory_get(function->args[0].value.val_voidptr,
								function->args[1].value.val_u32,function->args[2].value.val_sizet,pid);
			break;
		case FUNCTION_COPY:
			log_debug(muse_logger,"Copy called with args -> Memoria destino %d - Bytes a copiar %d",function->args[0].value.val_u32,function->args[2].value.val_u32);
			func_ret = memory_cpy(function->args[0].value.val_u32,
								function->args[1].value.val_voidptr,function->args[2].value.val_u32,pid);
			break;
		case FUNCTION_MAP:
			log_debug(muse_logger,"Map called");
			func_ret = memory_map(function->args[0].value.val_charptr,
								function->args[1].value.val_sizet,function->args[2].value.val_u32,pid);
			break;
		case FUNCTION_SYNC:
			log_debug(muse_logger,"Sync called");
			func_ret = memory_sync(function->args[0].value.val_u32,function->args[1].value.val_sizet,pid);
			break;
		case FUNCTION_UNMAP:
			log_debug(muse_logger,"Unmap called with args -> Direccion: %d", function->args[0].value.val_u32);
			func_ret = memory_unmap(function->args[0].value.val_u32, pid);
			break;
		default:
			log_error(muse_logger,"Unknown function");
			func_ret = 0;
			break;
	}
	return (void*)func_ret;
}
