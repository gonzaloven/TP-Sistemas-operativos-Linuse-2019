#ifndef hilolay_alumnos_h__
	#define hilolay_alumnos_h__
	#define HILOLAY_ALUMNO_CONFIG_PATH "../configs/hilolay_alumnos.config"

	/**
	 * TODO: Interface for alumnos (what they should implement in orde to make this work)
	 */
	typedef struct hilolay_operations {
		int (*suse_create) (int);
		int (*suse_schedule_next) (void);
		int (*suse_join) (int);
		int (*suse_close) (int);
		// suse_wait
		// suse_signal
	} hilolay_operations;

	hilolay_operations *main_ops;

	void init_internal(struct hilolay_operations*);

	typedef struct hilolay_alumno_configuracion {
		char* SUSE_IP;
		int SUSE_PORT;
	} hilolay_alumnos_configuracion;

	//Inicializa la libreria Hilolay, se conecta con SUSE.
	void hilolay_init(void);

	//Obtiene el numero de socket de conexion con SUSE
	int get_socket_actual();

	//Conexion con suse
	void conectar_con_suse();

	//Retorna la configuracion de hilolay_alumno
	hilolay_alumnos_configuracion get_configuracion_hilolay();

	hilolay_alumnos_configuracion configuracion_hilolay;


#endif // hilolay_alumnos_h__
