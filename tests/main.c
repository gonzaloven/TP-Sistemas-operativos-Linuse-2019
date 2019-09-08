#include <stdio.h>
#include "CUnit/Basic.h"
#include "client.h"
#include "message-test.h"

int main(int argc,char *argv[])
{
	if(CU_initialize_registry() != CUE_SUCCESS)
	{
		return CU_get_error();
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_pSuite message_suite = NULL;
	CU_pSuite connection_suite = NULL;

	message_suite = CU_add_suite("Messages test suite",0,0);
	connection_suite = CU_add_suite("Client connection test suite",0,0);

	if(message_suite == NULL || connection_suite == NULL)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	//CU_add_test(connection_suite,"server start test",client_connect_test);
	CU_add_test(message_suite,"message encoding test",message_encode_test);
	CU_add_test(message_suite,"message_decoding_test",message_decode_test);

	CU_basic_run_tests();
	CU_cleanup_registry();
	return 0;
}
