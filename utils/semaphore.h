#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdio.h>

typedef struct semaphore_t{
	char * id;
	int val;
	int max_val;
	struct TCB * waiting_queue;
} semaphore_t;

#endif
