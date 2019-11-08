TARGET:=suse
CC:=gcc
UTILS:= ../utils/
CFLAGS:= -g
INCDIR:= -I$(UTILS)
# Las metrics no se si van en un include de suse, no tengo idea para que la usa porque es un empty file
# por las dudas la pongo como que la necesita para compilar pero al hacerla fijense
SRCS := suse.c metrics.c $(UTILS)network.c $(UTILS)message.c
LFLAGS := -pthread -lcommons

all: suse

suse:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm -f suse *.o

