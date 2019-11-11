TARGET:=muse
CC:=gcc
UTILS:= ../utils/
CFLAGS:= -g
INCDIR:= -I$(UTILS)
SRCS := muse.c main_memory.c $(UTILS)network.c $(UTILS)message.c 
LFLAGS := -pthread -lcommons

all: muse

muse:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm -f muse *.o
