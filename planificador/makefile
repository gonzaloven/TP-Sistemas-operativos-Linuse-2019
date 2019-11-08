TARGET:=suse
CC:=gcc
UTILS:= ../utils/
CFLAGS:= -g
INCDIR:= -I$(UTILS)
SRCS := suse.c metrics.c $(UTILS)network.c $(UTILS)message.c
LFLAGS := -pthread -lcommons

all: suse

suse:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm -f suse *.o

