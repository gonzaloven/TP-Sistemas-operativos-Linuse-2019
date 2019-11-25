#include "message.h"
#include <stdlib.h>
#include <string.h>

int message_function_return_encode(Message *msg,void *buffer,size_t buffer_size)
{
	int min_size = sizeof(uint8_t) + sizeof(uint16_t)*2 + msg->header.data_size;
	if(buffer_size < min_size) return -1;

	void *cursor = buffer;
	memcpy(cursor,msg->data,sizeof(uint32_t));
	cursor += sizeof(uint32_t);
	return cursor - buffer;
}

int message_function_return_decode(void *buffer,size_t buffer_size,Message *result)
{
	void *cursor = buffer;
	result->data = malloc(sizeof(uint32_t));
	memcpy(result->data,cursor,sizeof(uint32_t));
	cursor += sizeof(uint32_t);
	return cursor - buffer;
}

int header_encode(MessageHeader *header,void *buffer,size_t buffer_size)
{
	if(buffer_size < sizeof(uint8_t)) return -1;

	void *cursor = buffer;
	memcpy(cursor,&(header->message_type),sizeof(uint8_t));
	cursor += sizeof(uint8_t);
	
	memcpy(cursor,&(header->caller_id),sizeof(uint16_t));
	cursor += sizeof(uint16_t);

	memcpy(cursor,&(header->data_size),sizeof(uint16_t));
	cursor += sizeof(uint16_t);

	return cursor - buffer;
}

int header_decode(void *buffer,size_t buffer_size,MessageHeader *result)
{
	if(buffer_size < sizeof(uint8_t)) return -1;
	
	void *cursor = buffer;
	memcpy(&result->message_type,cursor,sizeof(uint8_t));
	cursor += sizeof(uint8_t);
	
	memcpy(&result->caller_id,cursor,sizeof(uint16_t));
	cursor += sizeof(uint16_t);

	memcpy(&result->data_size,cursor,sizeof(uint16_t));
	cursor += sizeof(uint16_t);

	return cursor - buffer;
}

int message_encode(Message *msg,void *buffer,size_t buffer_size)
{
	int result = 0;
	size_t min_size = sizeof(uint8_t) + sizeof(uint16_t)*2 + msg->header.data_size;
	if(buffer_size < min_size) return -1;

	void *cursor = buffer;
	result += header_encode(&msg->header,cursor,buffer_size);
	if(result == -1) return -1;
	
	cursor += result;
	switch(msg->header.message_type)
	{
		case MESSAGE_CALL:
			result += message_function_encode(msg,cursor,buffer_size);
			break;
		case MESSAGE_NEW_ULT:
			printf("TODO\n");
			break;
		case MESSAGE_FUNCTION_RET:
			result += message_function_return_encode(msg,cursor,buffer_size);
			break;
		default:
			result = -1;
			break;
	}
	return result;
}

int message_decode(void *buffer,size_t buffer_size,Message *result)
{
	int res = 0;
	void *cursor = buffer;

	res += header_decode(buffer,buffer_size,&result->header);
	if(res == -1) return -1;

	cursor += res;
	switch(result->header.message_type)
	{
		case MESSAGE_CALL:
			res += message_function_decode(cursor,buffer_size,result);
			break;
		case MESSAGE_NEW_ULT:
			printf("TODO\n");
			break;
		case MESSAGE_FUNCTION_RET:
			res += message_function_return_decode(cursor,buffer_size,result);
			break;
		default:
			res = -1;
			break;
	}
	return res;
}

int function_arg_encode(Arg arg,void *buffer,size_t buffer_size)
{
	void *cursor = buffer;
	memcpy(cursor,&arg.type,sizeof(uint8_t));
	cursor += sizeof(uint8_t);

	memcpy(cursor,&arg.size,sizeof(uint16_t));
	cursor += sizeof(uint16_t);

	switch(arg.type)
	{
		case VAR_UINT32:
			memcpy(cursor,&(arg.value.val_u32),arg.size);
			cursor += arg.size;
			break;
		case VAR_CHAR_PTR:
			memcpy(cursor,arg.value.val_charptr,arg.size);
			cursor += arg.size;
			break;
		case VAR_SIZE_T:
			memcpy(cursor,&(arg.value.val_sizet),arg.size);
			cursor += arg.size;
			break;
		case VAR_VOID_PTR:
			memcpy(cursor,arg.value.val_voidptr,arg.size);
			cursor += arg.size;
	}
	return cursor - buffer;
}

int function_arg_decode(void *buffer,size_t buffer_size,Arg *arg)
{
	void *cursor = buffer;
	
	memcpy(&arg->type,cursor,sizeof(uint8_t));
	cursor += sizeof(uint8_t);
	
	memcpy(&arg->size,cursor,sizeof(uint16_t));
	cursor += sizeof(uint16_t);


	switch(arg->type)
	{
		case VAR_UINT32:
			memcpy(&(arg->value.val_u32),cursor,arg->size);
			cursor += arg->size;
			break;
		case VAR_CHAR_PTR:
			arg->value.val_charptr = malloc(arg->size); //Aca tambien hay memory leaks
			memcpy(arg->value.val_charptr,cursor,arg->size);
			cursor += arg->size;
			break;
		case VAR_VOID_PTR:
			arg->value.val_voidptr = malloc(arg->size);
			memcpy(arg->value.val_voidptr,cursor,arg->size);
			cursor += arg->size;
			break;
		case VAR_SIZE_T:
			memcpy(&(arg->value.val_sizet),cursor,arg->size);
			cursor += arg->size;
			break;
		default:
			return -1;
	}
	return cursor - buffer;
}

int message_function_encode(Message *msg,void *buffer,size_t buffer_size)
{
	int min_size = sizeof(uint8_t) + sizeof(uint16_t)*2 + msg->header.data_size;
	if(buffer_size < min_size) return -1;

	Function *f = (Function *)msg->data;
	int nargs = 0;
	void *cursor = buffer;
	memcpy(cursor,&f->type,sizeof(uint8_t));
	cursor += sizeof(uint8_t);

	memcpy(cursor,&f->num_args,sizeof(uint8_t));
	cursor += sizeof(uint8_t);	
	
	for(nargs=0;nargs < f->num_args;nargs++)
	{
		 function_arg_encode(f->args[nargs],cursor,buffer_size);
		cursor += sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t);	
	}
	return cursor - buffer;
}

int message_function_decode(void *buffer,size_t buffer_size,Message *result) // ACA ESTOY 2
{
	Function *f= malloc(sizeof(uint8_t)*2 + sizeof(Arg)*10); //OJO CON LOS TAMAÑOS //Aca tambien hay memory leaks aparentemente
	int nargs;
	f->type = 0;
	f->num_args = 0;
	
	for(nargs= 0;nargs < 9;nargs++)
	{
		f->args[nargs].size = 0;
		f->args[nargs].type = 0;
		f->args[nargs].value.val_u32 = 0;
		f->args[nargs].value.val_charptr = 0;
		f->args[nargs].value.val_voidptr = 0;	
	}	
	
	void *cursor = buffer;
	memcpy(&f->type,cursor,sizeof(uint8_t));
	cursor += sizeof(uint8_t);
	
	
	memcpy(&f->num_args,cursor,sizeof(uint8_t));
	cursor += sizeof(uint8_t);

	for(nargs = 0;nargs < f->num_args;nargs++)
	{
		function_arg_decode(cursor,buffer_size,&f->args[nargs]);	
		cursor += sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t);	
	}
	result->data = (void *)f;
	return cursor - buffer;
}

int create_response_message(Message *msg,MessageHeader *header,uint32_t response)
{
	msg->header = *header;
	msg->data = malloc(sizeof(uint32_t));
	uint32_t *data = (uint32_t *) msg->data;
	*data = response;
	return 0;
}
int create_message_header(MessageHeader *header,uint8_t message_type,uint16_t caller_id,uint16_t data_size)
{
	header->message_type = message_type;
	header->caller_id = caller_id;
	header->data_size = data_size;
	return 0;
}

int create_function_message(Message *msg,MessageHeader *header,Function *f)
{
	msg->header = *header;
	msg->data = malloc(sizeof(*f)); //Aca hay memory leaks
	msg->data = (void *)(f);
	return 0; 
}

int message_alloc_data(Message *msg,unsigned int size)
{
		msg->data = malloc(size);
		return 0;
}

int message_free_data(Message *msg)
{
	if(msg->data != NULL)
	{
		free(msg->data);
		msg->data = NULL;
		return 0;
	}
	return -1;
}


