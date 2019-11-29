# MUSE: Memoria Dinámica

----

## Como probar muse? 

1. Clonás el repo y hacés checkout a la branch de memoria
2. Desde la carpeta raíz de TP-2019-2C-LOS-TRAPITOS le das a make clean después a make all
3. Se creará el ejecutable muse (memoria/muse) y el ejecutable de pruebas (libs/libMuse/libmuse)
4. El archivo de pruebas se puede ir modificando para que llame a las distintas funciones de libMuse
5. Si ejecutás muse se abrirá el server y luego podes ejecutar libmuse (desde otra terminal) y de esta manera, muse irá logueando lo que va ocurriendo en el servidor
6. muse se cierra al presionar " ctrl + c " en la terminal que está corriendo

----

## Como funciona? 

- muse.c levanta el servidor central, levanta los loggers y el archivo de configuración, se queda escuchando y esperando a los clientes y dependiendo que funciones le manden, muse se encargará de llamar a dichas funciones, toda la lógica de estas funciones está en main_memory.c 

- main_memory.c recibe las mismas funciones que posee libmuse y de cada función a parte, recibe el pid del programa que llamo a la función