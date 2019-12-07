clear
echo "     __             __                        "
echo " .--|  .-----.-----|  .-----.--.--.-----.----."
echo " |  _  |  -__|  _  |  |  _  |  |  |  -__|   _|"
echo " |_____|_____|   __|__|_____|___  |_____|__|  "
echo "             |__|           |_____|           "
echo "               Desplegando SUSE"
echo " "


PLANIFICADORCONFIG="/home/utnso/workspace/tp-2019-2c-Los-Trapitos/configs/configPlanificador.cfg"

LISTEN_PORT=5002
METRICS_TIMER=70
MAX_MULTIPROG=15
SEM_IDS=[solo_hiper_mega_piola, afinado]
SEM_INIT=[1,0]
SEM_MAX=[10, 10]
ALPHA_SJF=0.5


# Escribir el archivo de configuracion
generar_configuracion(){
	echo "Ingrese puerto de SUSE :"
	echo -n "> "
	read LISTEN_PORT
	echo "Ingrese metrics timer :"
	echo -n "> "
	read METRICS_TIMER
	echo "Ingrese maximo de multiprogramacion :"
	echo -n "> "
	read MAX_MULTIPROG
	echo "Ingrese lista de semaforos. :"
	echo -n "> "
	read SEM_IDS
	echo "Ingrese lista de inicializacion de semaforos :"
	echo -n "> "
	read SEM_INIT
	echo "Ingrese lista de maximos de semaforos :"
	echo -n "> "
	read SEM_MAX
	echo "Ingrese alfa de planificacion :"
	echo -n "> "
	read ALPHA_SJF

	echo "LISTEN_PORT=$LISTEN_PORT" >> "$PLANIFICADORCONFIG"
	echo "METRICS_TIMER=$METRICS_TIMER" >> "$PLANIFICADORCONFIG"
	echo "MAX_MULTIPROG=$MAX_MULTIPROG" >> "$PLANIFICADORCONFIG"
	echo "SEM_IDS=$SEM_IDS" >> "$PLANIFICADORCONFIG"
	echo "SEM_INIT=$SEM_INIT" >> "$PLANIFICADORCONFIG"
	echo "SEM_MAX=$SEM_MAX" >> "$PLANIFICADORCONFIG"
	echo "ALPHA_SJF=$ALPHA_SJF" >> "$PLANIFICADORCONFIG"
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