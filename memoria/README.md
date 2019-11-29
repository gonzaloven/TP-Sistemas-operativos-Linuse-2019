# MUSE: Memoria Dinámica

----

## Como probar muse? 

1. Clonás el repo y hacer checkout a la branch de memoria
2. Desde la carpeta raíz de TP-2019-2C-LOS-TRAPITOS le das a make
3. Se creará el ejecutable muse (memoria/muse) y el archivo de pruebas (libs/libMuse/libmuse)
4. El archivo de pruebas se puede ir modificando para que llame a las distintas funciones de libMuse
5. Podemos ejecutar muse y abrirá el server y el mismo podrá escuchar, luego ir ejecutás libmuse (desde otra terminal) y muse irá logueando lo que va ocurriendo
6. muse se cierra al presionar " ctrl + c " en la terminal que está corriendo

----

## Como funciona? 

- muse.c levanta el servidor central, levanta los loggers y el archivo de configuración, se queda escuchando y esperando a los clientes y dependiendo que funciones le manden, muse se encargará de llamar a dichas funciones, toda la lógica de estas funciones está en main_memory.c 

- main_memory.c recibe las mismas funciones que posee libmuse y de cada función a parte, recibe el pid del programa que llamo a la función