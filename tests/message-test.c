#include "message-test.h"
#include "message.h"
#include "CUnit/Basic.h"


void message_encode_test()
{
	MessageHeader header; 
	Message msg;
	
	create_message_header(&header,MESSAGE_STRING);
	message_alloc_data(&msg,5);
	create_string_message(&msg,&header,"Hola");

	unsigned short int buffer_size = sizeof(uint8_t) + sizeof(uint16_t) + 4;
	char buffer[buffer_size];
	int result = message_encode(&msg,buffer,buffer_size);
	message_free_data(&msg);

	CU_ASSERT(result == buffer_size);
}

void message_decode_test()
{
	MessageHeader header; 
	Message msg;
	Message msg_decoded;
	char *hello;

	create_message_header(&header,MESSAGE_STRING);
	message_alloc_data(&msg,5);
	create_string_message(&msg,&header,"Hola");
	
	unsigned short int buffer_size = sizeof(uint8_t) + sizeof(uint16_t) + 4;

	char buffer[buffer_size];
	int encode_result = message_encode(&msg,buffer,buffer_size);
	message_free_data(&msg);

	int decode_result = message_decode(buffer,buffer_size,&msg_decoded);
	if(decode_result != -1)
	{
		CU_ASSERT(decode_result == buffer_size);
		CU_ASSERT_EQUAL(msg_decoded.header.message_type,MESSAGE_STRING);
		CU_ASSERT_EQUAL(msg_decoded.data_size,strlen(msg_decoded.data));
		CU_ASSERT(strcmp(msg_decoded.data,"Hola") == 0);
	}else{
		CU_ASSERT(1 == 2);
	}
}
