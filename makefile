all:
	#make -C libs/libMuse
	make -C filesystem
	make -C memoria
	make -C planificador

clean:
	#make clean -C libs/libMuse
	make clean -C filesystem
	make clean -C memoria
	make clean -C planificador
