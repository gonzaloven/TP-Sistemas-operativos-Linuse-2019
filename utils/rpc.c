#include "rpc.h"
#include "cJSON.h"
#include "network.h"

char* rpc_form_function_msg(char *name,int nparams,rpc_function_params fparams[])
{
	if(fparams == NULL) return NULL;

	cJSON *root = cJSON_CreateObject();
	cJSON *params = cJSON_CreateArray();
	cJSON *args = cJSON_CreateObject();

	cJSON_AddStringToObject(root,"fname",name);
	cJSON_AddNumberToObject(root,"nparams",nparams);
	cJSON_AddItemToArray(params,args);

	for(int index = 0; index<nparams;index++)
	{
		cJSON_AddStringToObject(args,"var_type",fparams[index].type);
		cJSON_AddStringToObject(args,"var_name",fparams[index].name);
		cJSON_AddStringToObject(args,"var_value",fparams[index].value);	
	}

	cJSON_AddItemToObject(root,"params",params);
	
	char *json_data = cJSON_Print(root);
	cJSON_Delete(root);
	return json_data;
}
int rpc_client_call(int socket,char *name,int nparams,rpc_function_params fparams[])
{
	Message m;
	MessageHeader header;
	char buffer[1024];
	char *json_data = rpc_form_function_msg(name,nparams,fparams);

	if(json_data == NULL) return -1;
	
	create_message_header(&header,MESSAGE_CALL);
	create_string_message(&m,&header,json_data);

	send_message(socket,&m);
	receive_packet(socket,buffer,1024);

	message_free_data(&m);

	//TODO: return the result of the calling
	//int *result = (int *)m.data;
	//int r = *result;

	return 0;	
}

int rpc_server_invoke(char *json_msg)
{
	char func_name[20];
	cJSON *rpc_call = cJSON_Parse(json_msg);
	
	if(rpc_call == NULL) 
	{
		//printf("rpc_server_invoke(): error!\n");
		return -1;
	}

	cJSON *fname = cJSON_GetObjectItem(rpc_call,"fname");
	char *json_data = cJSON_Print(fname);
	strcpy(func_name,json_data);

	//TODO: Use a hashtable for the function names
	//This is a test
	//Enviar respuesta
	if((strcmp(func_name,"\"alloc\"")) == 0)
	{
		printf("memory allocated\n");
	}else if((strcmp(func_name,"\"free\"")) == 0)
	{
		printf("memory freed\n");
	}
	
	cJSON_Delete(rpc_call);
	free(json_data);
	//cJSON_Delete(fname);
	
	return 0;
}

