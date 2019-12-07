TARGET:= libmuse
CC:=gcc
TARGET:=libmuse.so
UTILS:= ../../utils_suse/
INCDIR:= -I$(UTILS)
CFLAGS:= -g
SRCS := libmuse_tests.c libmuse.c $(UTILS)network.c $(UTILS)message.c 
LFLAGS := -pthread -lcommons -lm

all: libmuse

#libmuse: libmuse.o
#	$(CC) -g $(LIBFLAGS) $(TARGET) libmuse.o

libmuse.o:makedir libmuse.c
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS)

makedir:
	mkdir -p ../bin

libmuse:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm -f libmuse *.o
