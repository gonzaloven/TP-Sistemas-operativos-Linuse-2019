clear
echo "     __             __                        "
echo " .--|  .-----.-----|  .-----.--.--.-----.----."
echo " |  _  |  -__|  _  |  |  _  |  |  |  -__|   _|"
echo " |_____|_____|   __|__|_____|___  |_____|__|  "
echo "             |__|           |_____|           "
echo "               Desplegando File System"
echo " "


FILESYSTEMCONFIG="/home/utnso/workspace/tp-2019-2c-Los-Trapitos/configs/filesystem.config"

LISTEN_PORT=5002


# Escribir el archivo de configuracion
generar_configuracion(){
	echo "Ingrese puerto de File System :"
	echo -n "> "

	echo "LISTEN_PORT=$LISTEN_PORT" >> "$FILESYSTEMCONFIG"
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