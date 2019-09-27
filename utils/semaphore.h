#ifndef SEMAPHORE_H
#define SEMAPHORE_H

typedef struct {
	char * id;
	int val;
	int max_val;
	struct TCB * waiting_queue;
} semaphore_t;

#endif
