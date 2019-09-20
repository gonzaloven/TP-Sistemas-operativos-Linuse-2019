#include<sac_servidor.h>

// TODO add logs
ptrGBloque determine_nodo(const char* path){

	// If it is the root directory, it returns 0
	if(!strcmp(path, "/")) return 0;

	int i, nodo_anterior, err = 0;

	// Super_path used to get the top of the path, without the name.
	char *super_path = (char*) malloc(strlen(path) +1), *nombre = (char*) malloc(strlen(path)+1);
	char *start = nombre, *start_super_path = super_path; //Pointers used to free memory.
	struct sac_server_gfile *node;
	unsigned char *node_name;

	split_path(path, &super_path, &nombre);

	nodo_anterior = determinar_nodo(super_path);


	pthread_rwlock_rdlock(&rwlock); //Takes a lock for read
	node = node_table_start;

	// Search the node where is the name
	node_name = &(node->fname[0]);
	for (i = 0; ( (node->parent_dir_block != nodo_anterior) | (strcmp(nombre, (char*) node_name) != 0) | (node->state == 0)) &  (i < GFILEBYTABLE) ; i++ ){
		node = &(node[1]); // next node position
		node_name = &(node->fname[0]);
	}

	// Close connections and free memory. Contemplate error cases.
	pthread_rwlock_unlock(&rwlock);
	free(start);
	free(start_super_path);
	if (err != 0) return err;
	if (i >= GFILEBYTABLE) return -1;

	return (i+1); // +1 because /home is node position 1 (0 is the root), and i = 0 because doest
				 //enter the loop
}

int split_path(const char* path, char** super_path, char** name){
	int aux;
	strcpy(*super_path, path);
	strcpy(*name, path);
	// Obtain and accommodate the file name.
	if (lastchar(path, '/')) {
		(*name)[strlen(*name)-1] = '\0';
	}
	*name = strrchr(*name, '/');
	*name = *name + 1; // Accommodate the name, since the first digit is always '/'

	// Acomoda el super_path
	if (lastchar(*super_path, '/')) {
		(*super_path)[strlen(*super_path)-1] = '\0';
	}
	aux = strlen(*super_path) - strlen(*name);
	(*super_path)[aux] = '\0';

	return 0;
}

int lastchar(const char* str, char chr){
	if ( ( str[strlen(str)-1]  == chr) ) return 1;
	return 0;
}

