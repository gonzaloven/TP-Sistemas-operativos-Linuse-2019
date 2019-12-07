clear
echo "     __             __                        "
echo " .--|  .-----.-----|  .-----.--.--.-----.----."
echo " |  _  |  -__|  _  |  |  _  |  |  |  -__|   _|"
echo " |_____|_____|   __|__|_____|___  |_____|__|  "
echo "             |__|           |_____|           "
echo "               Desplegando Memoria"
echo " "


FILESYSTEMCONFIG="/home/utnso/workspace/tp-2019-2c-Los-Trapitos/configs/memoria.cfg"

# Escribir el archivo de configuracion
generar_configuracion(){
	echo "Ingrese puerto de Memoria :"
	echo -n "> "
	echo "Ingrese el tamanio de la memoria :"
	echo -n "> "
	echo "Ingrese el tamanio de pagina :"
	echo -n "> "
	echo "Ingrese el tamanio de swap :"
	echo -n "> "

	echo "LISTEN_PORT=$LISTEN_PORT" >> "$FILESYSTEMCONFIG"
	echo "MEMORY_SIZE=$MEMORY_SIZE" >> "$FILESYSTEMCONFIG"
	echo "PAGE_SIZE=$PAGE_SIZE" >> "$FILESYSTEMCONFIG"
	echo "SWAP_SIZE=$SWAP_SIZE" >> "$FILESYSTEMCONFIG"
}

#
# Script principal
#

# Opcion: generar el archivo de configuracion
echo "Desea generar el archivo de configuracion?"
select SN in "Si" "No"; do
    case $SN in
        Si ) generar_configuracion; break;;
        No ) break;;
    esac
done
