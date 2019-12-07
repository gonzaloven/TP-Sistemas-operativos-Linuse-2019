TARGET:=libutils
CC:=gcc
CFLAGS:= -g
SRCS := network.c semaphore.c message.c
LFLAGS := -pthread -lcommons -shared

all: utils

utils:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET).so $(LFLAGS)

clean:
	rm -f network *.o
	rm -f semaphore *.o
	rm -f message *.o