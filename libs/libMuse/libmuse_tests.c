#include "libmuse.h"
#include <unistd.h>
#include <string.h>     /* strcat */


void foo1();
void foo2();
void foo3();
void foo4();
void foo5();
void foo6();
void foo7();
void foo8();
void foo9();

const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}


int main(int argc,char *argv[])
{
	int pid = getpid();
	char ch;
	int todoOK = muse_init(pid);
	if (todoOK == 0)
	{
		printf("Libmuse with proccess_id = %d initialized!\n",pid);
	
		printf("Introduzca un nro del 1-8: ");

		ch=getchar();
		switch(ch) 
		{
			case '1': foo1();
			break;
			case '2': foo2();
			break;
			case '3': foo3();
			break;
			case '4': foo4();
			break;
			case '5': foo5();
			break;
			case '6': foo6();
			break;
			case '7': foo7();
			break;
			case '8': foo8();
			break;
			case '9': foo9();
			break;
			default: puts("Error");
		}
	}else printf("Libmuse with proccess_id = %d couldn't be initialized :(\n",pid);

	return 0;
}
void imprimir(int alocados, int dir){
	printf("La direccion_vir de los %d 	bytes reservados es %d(dec),	%s(bin)\n", alocados, dir, byte_to_binary(dir));
}

void alocar_y_mostrar(int size)
{
	int	dir = muse_alloc(size);
	imprimir(size, dir);
}


void foo1(){
	
		int dir = muse_alloc(20);
		imprimir(20, dir);
		int dir2 = muse_alloc(30);
		imprimir(30, dir2);
		int dir3 = muse_alloc(50);
		imprimir(50, dir3);
		int dir4 = muse_alloc(200);
		imprimir(200, dir4);

		muse_free(dir);
		printf("Se libero la direccion logica %d\n",dir);
		muse_free(dir2);
		printf("Se libero la direccion logica %d\n",dir2);

		int dir5 = muse_alloc(50);
		imprimir(50, dir5);

		muse_free(dir3);
		printf("Se libero la direccion logica %d\n",dir3);
		muse_free(dir4);
		printf("Se libero la direccion logica %d\n",dir4);

		int dir6 = muse_alloc(20);
		imprimir(20, dir6);

		muse_close();
}	

void foo2()
{
	alocar_y_mostrar(10);
	alocar_y_mostrar(30);
	alocar_y_mostrar(10);
	alocar_y_mostrar(500);
	alocar_y_mostrar(1000);
	alocar_y_mostrar(1000);
	alocar_y_mostrar(1000);
	alocar_y_mostrar(500);
	alocar_y_mostrar(63);
	alocar_y_mostrar(32);
}



void foo3(){
	
		int dir = muse_alloc(30);
		imprimir(30, dir);
		int dir2 = muse_alloc(55);
		imprimir(55, dir2);
		int dir3 = muse_alloc(67);
		imprimir(67, dir3);

		// muse_free(dir);		
		// printf("Se libero la direccion logica %d\n",dir);

		muse_close();
}

void foo4(){
		int dir = muse_alloc(10);
		imprimir(10, dir);
		int dir2 = muse_alloc(20);
		imprimir(20, dir2);
		int dir3 = muse_alloc(30);
		imprimir(30, dir3);

		muse_free(dir);		
		printf("Se libero la direccion logica %d\n",dir);

		muse_close();
}

void foo5(){

	int direccion1 = muse_map("/home/utnso/tp-2019-2c-Los-Trapitos/memoria/hola.txt", 26, MAP_SHARED);
	printf("La direccion es %d(dec),	%s(bin)\n", direccion1, byte_to_binary(direccion1));

	muse_cpy(direccion1, "hola", strlen("hola") + 1);

	int direccion2 = muse_map("/home/utnso/tp-2019-2c-Los-Trapitos/memoria/holiwis", 450, MAP_PRIVATE);
	printf("La direccion es %d(dec),	%s(bin)\n", direccion2, byte_to_binary(direccion2));
	int direccion3 = muse_map("/home/utnso/tp-2019-2c-Los-Trapitos/memoria/forrazo", 200, MAP_SHARED);
	printf("La direccion es %d(dec),	%s(bin)\n", direccion3, byte_to_binary(direccion3));

	muse_unmap(direccion1);
	printf("Unmapeada.\n");
	muse_unmap(direccion2);
	printf("Unmapeada.\n");
	muse_unmap(direccion3);
	printf("Unmapeada.\n");

	int direccion4 = muse_map("/home/utnso/tp-2019-2c-Los-Trapitos/memoria/hola.txt", 26, MAP_SHARED);
	printf("La direccion es %d(dec),	%s(bin)\n", direccion4, byte_to_binary(direccion4));
	muse_unmap(direccion4);
	printf("Unmapeada.\n");

	muse_unmap(38392);
	printf("Deberia haber tirado seg fault sin romperse\n");

	printf("Anduvo bien, test finalizo\n");

	muse_close();
}

void foo6(){
	int dir7 = muse_alloc(strlen("hola")+ 1);
	imprimir(sizeof(int), dir7);
	muse_cpy(dir7, "hola", strlen("hola")+ 1);

	muse_close();
}

void foo7(){

	// int direccion1 = muse_map("/home/utnso/tp-2019-2c-Los-Trapitos/memoria/putoElQueLee", 50, MAP_SHARED);
	// 
	// //int direccion2 = muse_alloc(strlen(" y sigue leyendo")+ 1);
	// //imprimir(sizeof(int), direccion2);

	// //int direccion2 = (direccion1 + strlen("puto el que lee"));

	// muse_cpy(direccion1, "t", strlen(" y sigue leyendo"));
	// printf("Copio data en la direccion: %d(dec)\n", direccion1);

	int dir7 = muse_map("/home/utnso/tp-2019-2c-Los-Trapitos/memoria/putoElQueLee", 350, MAP_SHARED);
	printf("Mapeo el archivo putoElQueLee en: %d(dec)\n", dir7);
	muse_cpy(dir7 + 5, "hola como estas todo bien xd xdx xd lol 123456 hola gola", strlen("hola como estas todo bien xd xdx xd lol 123456 hola gola")+ 1);

	char* verGet = malloc(strlen("hola como estas todo bien xd xdx xd lol 123456 hola gola")+ 7);
	muse_get((void*)verGet, dir7, strlen("hola como estas todo bien xd xdx xd lol 123456 hola gola")+ 7);
	printf("Lo leido es: %s\n", verGet);

	int retorno = muse_sync(dir7, 350);
	printf("Se synqueo putoElQueLee y el resultado fue (if 0 todo ok): %d \n", retorno);

	muse_close();
}

void foo8(){
	int dir7 = muse_alloc(strlen("dasasddasdasdasdasadsadsdasadsdasadsadsadsadsasdadsadsdasadsadsadsadsasd")+ 1);
	imprimir(sizeof(int), dir7);
	muse_cpy(dir7, "dasasddasdasdasdasadsadsdasadsdasadsadsadsadsasdadsadsdasadsadsadsadsasd", strlen("dasasddasdasdasdasadsadsdasadsdasadsadsadsadsasdadsadsdasadsadsadsadsasd")+ 1);

	char* verGet = malloc(strlen("dasasddasdasdasdasadsadsdasadsdasadsadsadsadsasdadsadsdasadsadsadsadsasd")+ 1);
	muse_get((void*)verGet, dir7, strlen("dasasddasdasdasdasadsadsdasadsdasadsadsadsadsasdadsadsdasadsadsadsadsasd")+ 1);
	printf("Lo leido es: %s\n", verGet);
}


void foo9(){
	printf("Funcion prohibida!!\n\n");
	foo1();
	foo2();
	foo3();
	foo4();
	foo5();
	foo6();
	foo7();
	foo8();
}
