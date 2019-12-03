TARGET:= libmuse
CC:=gcc
UTILS:= ../../utils/
INCDIR:= -I$(UTILS)
CFLAGS:= -g
SRCS := libmuse_tests.c libmuse.c $(UTILS)network.c $(UTILS)message.c 
LFLAGS := -pthread -lcommons -lm

all: libmuse

libmuse:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm -f libmuse *.o
