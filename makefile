all:
	cd libs/Hilolay && make
	cd libs/libMuse && make
	cd filesystem && make
	cd memoria && make
	cd planificador && make

clean:
	cd libs/Hilolay && make clean
	cd libs/libMuse && make clean
	cd filesystem && make clean
	cd memoria && make clean
	cd planificador && make clean

