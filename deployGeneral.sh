#!/bin/bash

mkdir workspace

cd /home/utnso/workspace

echo "Clonando hilolay..."
git clone https://github.com/sisoputnfrba/hilolay

echo "Clonando pruebas..."
git clone https://github.com/sisoputnfrba/linuse-tests-programs

echo "Clonando commons..."
git clone https://github.com/sisoputnfrba/so-commons-library

echo "Clonando tp..."
git clone https://github.com/sisoputnfrba/tp-2019-2c-Los-Trapitos.git

echo "Clonando proyecto con scripts de deploy..."
git clone https://github.com/lopezjazmin/pruebasSO2019.git

cd /home/utnso/workspace/so-commons-library

echo "Ingrese password para instalar las commons..."
sudo make install

cd /home/utnso/workspace/hilolay
echo "Ingrese password para instalar Hilolay..."
sudo make && sudo make install

cd /home/utnso/workspace/tp-2019-2c-Los-Trapitos/utils_suse/
echo "Ingrese password para instalar utils de SUSE..."
make clean
make && make all
echo "Compiled with exit code $?"¡

echo "Copiando MakeFile de pruebas..."
cd /home/utnso/workspace/pruebasSO2019
cp Makefile ../linuse-tests-programs

echo "Copiando hilolay_alumnos..."
cd /home/utnso/workspace/pruebasSO2019
cp hilolay_alumnos.c ../linuse-tests-programs
cd ..

echo "Copiando libmuse..."
cd /home/utnso/workspace/pruebasSO2019
cp libmuse.h ../linuse-tests-programs
cd ..

cd /home/utnso/workspace/tp-2019-2c-Los-Trapitos/utils_suse
make clean
make && sudo make all
echo "Compiled utils with exit code $?"

cd /home/utnso/workspace/tp-2019-2c-Los-Trapitos/libs/libMuse
make clean
make && sudo make all
echo "Compiled utils with exit code $?"

cd /home/utnso/workspace/tp-2019-2c-Los-Trapitos
make clean
make && sudo make all
echo "Compiled tp exit code $?"

echo "Haciendo make a todo..."
cd /home/utnso/workspace/linuse-tests-programs/
make clean
make && make all
make entrega
echo "Compiled pruebas exit code $?"

echo "Todo listo!!!!!"

