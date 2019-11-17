TARGET:=muse
CC:=gcc
UTILS:= ../utils/
INCDIR:= -I$(UTILS)
CFLAGS:= -g
SRCS := muse.c main_memory.c $(UTILS)network.c $(UTILS)message.c 
LFLAGS := -pthread -lcommons

all: muse

muse:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm -f muse *.o
