TARGET:=libutils_suse
CC:=gcc
CFLAGS:= -g
SRCS := libraries.c message.c network.c
LFLAGS := -pthread -lcommons -shared

all: utils_suse

utils_suse:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET).so $(LFLAGS)

clean:
	rm -f libraries *.o
	rm -f message *.o
	rm -f network *.o