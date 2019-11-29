TARGET:=suse
CC:=gcc
UTILS_SUSE:= ../utils_suse/
CFLAGS:= -g
INCDIR:= -I$(UTILS_SUSE)
# Las metrics no se si van en un include de suse, no tengo idea para que la usa porque es un empty file
# por las dudas la pongo como que la necesita para compilar pero al hacerla fijense
SRCS := suse.c metrics.c $(UTILS_SUSE)libraries.c
LFLAGS := -pthread -lcommons

all: suse

suse:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm -f suse *.o

