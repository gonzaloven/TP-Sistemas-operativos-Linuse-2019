#include "rpc.h"
#include "network.h"
#include "cJSON.h"
#include <stdarg.h>

int rpc_client_call(int socket,char *name,int nparams,rpc_function_params fparams[],...)
{
	Message m;
	MessageHeader header;
	va_list arg_list;

	if(fparams == NULL) return -1;
	
	create_message_header(&header,MESSAGE_CALL);

	cJSON *root = cJSON_CreateObject();
	cJSON *params = cJSON_CreateArray();
	cJSON *args = cJSON_CreateObject();

	cJSON_AddStringToObject(root,"fname",name);
	cJSON_AddNumberToObject(root,"nparams",nparams);
	cJSON_AddItemToArray(params,args);

	va_start(arg_list,nparams);
	for(int index = 0; index<nparams;index++)
	{
		cJSON_AddStringToObject(args,"var_type",fparams[index].type);
		cJSON_AddStringToObject(args,"var_name",fparams[index].name);
		cJSON_AddStringToObject(args,"var_value",fparams[index].value);	
	}	

	cJSON_AddItemToObject(root,"params",params);
	va_end(arg_list);
	//cJSON_AddNumberToObject(root,"param",param);
	
	char *json_data = cJSON_Print(root);
	message_alloc_data(&m,strlen(json_data));

	create_string_message(&m,&header,json_data);
	send_message(socket,&m);

	message_free_data(&m);
	cJSON_Delete(root);
	free(json_data);
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
	if((strcmp(func_name,"\"alloc\"")) == 0)
	{
		printf("memory allocated\n");
	}
	
	cJSON_Delete(rpc_call);
	free(json_data);
	//cJSON_Delete(fname);
}
