all:
	$(info *******************************************************)
	$(info ********************   MAKE ALL   *********************)
	$(info *******************************************************)
#make -C libs/Hilolay
	make -C libs/libMuse
#make -C filesystem
	make -C memoria
#make -C planificador
	make test-libmuse -C tests
	make test-muse-alloc -C tests

clean:
	$(info *******************************************************)
	$(info ********************   CLEAN ALL   ********************)
	$(info *******************************************************)
	make clean -C libs/Hilolay
	make clean -C libs/libMuse
	make clean -C filesystem
	make clean -C memoria
	make clean -C planificador
	make clean -C tests	
