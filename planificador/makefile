TARGET:=suse
CC:=gcc
UTILS:= ../utils/
CFLAGS:= -g
INCDIR:= -I$(UTILS)
SRCS := suse.c $(UTILS)network.c $(UTILS)message.c
LFLAGS := -lpthread -lcommons

suse:
		$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
		rm suse *.o

