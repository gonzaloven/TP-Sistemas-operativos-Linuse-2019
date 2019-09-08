#include "network.h"
#include "client.h"
#include "CUnit/Basic.h"

void client_connect_test()
{
    //The server must be running to test this program
    int sock = connect_to("127.0.0.1",8000);
    CU_ASSERT(sock != -1);    
}