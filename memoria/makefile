TARGET:=muse
CC:=gcc
UTILS:= ../utils/
CFLAGS:= -g
INCDIR:= -I$(UTILS)
SRCS := muse.c main_memory.c $(UTILS)network.c $(UTILS)message.c 
LFLAGS := -lpthread -lcommons

muse:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm muse *.o
