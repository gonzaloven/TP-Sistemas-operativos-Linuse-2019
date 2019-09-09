#include "message.h"
#include <stdlib.h>

int header_encode(MessageHeader *header,void *buffer,size_t buffer_size)
{
	if(buffer_size < sizeof(uint8_t)) return -1;

	void *cursor = buffer;
	memcpy(cursor,&header->message_type,sizeof(uint8_t));
	cursor += sizeof(uint8_t);

	return cursor - buffer;
}

int header_decode(void *buffer,size_t buffer_size,MessageHeader *result)
{
	if(buffer_size < sizeof(uint8_t)) return -1;
	
	void *cursor = buffer;
	memcpy(&result->message_type,cursor,sizeof(uint8_t));
	cursor += sizeof(uint8_t);

	return cursor - buffer;
}

int message_encode(Message *msg,void *buffer,size_t buffer_size)
{
	int result = 0;
	size_t min_size = sizeof(uint8_t) + sizeof(uint16_t) + msg->data_size;
	if(buffer_size < min_size) return -1;

	void *cursor = buffer;
	result += header_encode(&msg->header,cursor,buffer_size);
	if(result == -1) return -1;
	
	cursor += result;
	switch(msg->header.message_type)
	{
		case MESSAGE_STRING:
		case MESSAGE_CALL:
			result += message_string_encode(msg,cursor,buffer_size);
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
		case MESSAGE_STRING:
		case MESSAGE_CALL:
			res += message_string_decode(cursor,buffer_size,result);
			break;
		default:
			res = -1;
			break;
	}
	return res;
}

int message_string_encode(Message *msg,void *buffer,size_t buffer_size)
{
	int min_size = sizeof(uint8_t) + sizeof(uint16_t) + msg->data_size;
	if(buffer_size < min_size) return -1;

	void *cursor = buffer;
	memcpy(cursor,&msg->data_size,sizeof(uint16_t));
	cursor += sizeof(uint16_t);

	memcpy(cursor,msg->data,msg->data_size);
	cursor += msg->data_size;

	return cursor - buffer;
}

int message_string_decode(void *buffer,size_t buffer_size,Message *result)
{
	void *cursor = buffer;
	memcpy(&result->data_size,cursor,sizeof(uint16_t));
	cursor += sizeof(uint16_t);

	result->data = malloc(result->data_size);
	memcpy(result->data,cursor,result->data_size);
	cursor += result->data_size;

	return cursor - buffer;
}


int create_message_header(MessageHeader *header,uint8_t message_type)
{
	header->message_type = message_type;
	return 0;
}

int create_string_message(Message *msg,MessageHeader *header,char *data)
{
	msg->header = *header;
	msg->data_size = strlen(data);
	strcpy(msg->data,data);
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

