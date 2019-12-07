TARGET:= libmuse
CC:=gcc
TARGET:=libmuse.so
UTILS:= ../../utils_suse/
INCDIR:= -I$(UTILS)
CFLAGS:= -g
SRCS := libmuse_tests.c libmuse.c $(UTILS)network.c $(UTILS)message.c 
LFLAGS := -pthread -lcommons -lm
NAME:= libmuseex

all: $(NAME).so

$(NAME).o: 
	gcc -c -fpic -lutils_suse -lcommons -o libmuse.o libmuse.c -L ../../utils_suse/ -I ../../utils_suse/
	
$(NAME).so: $(NAME).o
	gcc -shared -o libmuse.so libmuse.o

clean:
	rm -f libmuse *.o
