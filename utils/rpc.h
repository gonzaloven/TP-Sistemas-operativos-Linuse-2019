#ifndef RPC_H
#define RPC_H

#include <commons/collections/list.h>

typedef struct rpc_function_params_s
{
	int type;
	char name[20];
	void *value;

}rpc_function_params;

typedef struct rpc_function_s
{
	char fname[20]; //function number
	int nparams;	//number of params
	t_list *params;
}rpc_function;

/* This function let's a rpc server
 * ivoke a function called by a client
 * @param json_msg:the message sent by the client 
 * in json format
 */
int rpc_server_invoke(char *json_msg);

/* This function let's a client process call a remote 
 * procedure.
 * @param socket:the socket of the server to send
 * @param name of the functio to call example(alloc,cpy etc..).
 * @param nparams: the number of parameters of the function
 * @param param: the parameters od the function TODO change this make it variable!
 */
int rpc_client_call(int socket,char* name,int nparams,int param);
//int rpc_client_call(int socket,char *name,int params,...) TODO: use va_list

#endif
