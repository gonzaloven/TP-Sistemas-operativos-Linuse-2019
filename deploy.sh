#!/bin/bash

#Este script permite automatizar el despliegue del tp
#compilando todos los modulos necesario inclusive sus 
#librerias ademas de setear las variables de entorno

#compilar libreria muse
echo Compiling libMuse 
cd libs
cd libMuse 
make clean
make libmuse
cd .. 

if [-d "/bin/"];	then
	mv -f libMuse/libmuse.so bin
else
	mkdir bin
	mv libMuse/libmuse.so bin
fi
echo Done..

cd ..

#compilar muse
echo Compiling MUSE module
cd memoria
make clean
make muse
echo Done
cd ..

#compilar el programa test
echo Compiling Program Test
cd tests
make clean
make test-libmuse
echo Done

#echo Creating Environment Variables
#unset LD_LIBRARY_PATH
#LD_LIBRARY_PATH=/home/utnso/tp-2019-2c-Los-Trapitos/libs/bin:$LD_LIBRARY_PATH
#export LD_LIBRARY_PATH
#echo Done
