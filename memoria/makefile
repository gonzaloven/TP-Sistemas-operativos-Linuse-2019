TARGET:=muse
CC:=gcc
UTILS:= ../utils/
INCDIR:= -I$(UTILS)
SRCS := muse.c $(UTILS)network.c $(UTILS)message.c
LFLAGS := -lpthread -lcommons

muse:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm muse *.o
