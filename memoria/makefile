TARGET:=muse
CC:=gcc
UTILS:= ../utils/
CFLAGS:= -g
INCDIR:= -I$(UTILS)
SRCS := muse.c $(UTILS)network.c $(UTILS)message.c $(UTILS)rpc.c $(UTILS)cJSON.c
LFLAGS := -lpthread -lcommons

muse:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm muse *.o
