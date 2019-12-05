#include "libmuse.h"
#include <unistd.h>
#include <string.h>     /* strcat */


void foo1();
void foo2();
void foo3();
void foo4();
void foo5();

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
	
		printf("Introduzca un nro del 1-5: ");

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
			default: puts("Error");
		}
	}else printf("Libmuse with proccess_id = %d couldn't be initialized :(\n",pid);

	return 0;
}
void imprimir(int alocados, int dir){
	printf("La direccion_vir de los %d bytes reservados es %d(dec),	%s(bin)\n", alocados, dir, byte_to_binary(dir));
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
	int dir4 = muse_alloc(20);
	imprimir(20, dir4);
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

	muse_map("hola.txt", 20, 0);
	muse_unmap("hola.txt");
	muse_close();
}
