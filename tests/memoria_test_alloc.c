/*
*	
*/
#include "network.h"
#include "cJSON.h"
#include "libmuse.h"

int main(int argc,char *argv[])
{
	muse_init(1); //testing this shouldn't be here
	
	uint16_t r=muse_alloc(10);
	printf("%d\n",r);
	int x = 20;
	int *y = malloc(4);
	muse_cpy(r,&x,4);
	muse_get(y,r,4);
	printf("%d\n",*y);


	//Memoria Compartida
	size_t filesize = 16;
	int MAP_PRIVATE = 1;	//esto pa' que compile
	
	uint32_t map = muse_map("hola.txt", filesize, MAP_PRIVATE); 
	/*	
		opens the "hola.txt" file in RDONLY mode
		map is a position _mapped_ of pages of a given 'filesize' of the "hola.txt" file  
		muse_map basically is just putting the hola.txt file in memory    
	*/
		uint32_t my_memory = muse_alloc(10); //única porción central de memoria
	int memory_x = 10;
	int*memory_y = malloc(4);
	muse_cpy(my_memory, &memory_x, 4); //copies 4 bytes from local memory_x area to my_memory area
	muse_get(memory_y, my_memory, 4); //copies 4 bytes from my_memory area to local memory_y area.
	printf("%d", *memory_y);
	
	printf(map, filesize);
	int x = 4;
	muse_cpy(map, &x, 4); //copies 4 bytes from local memory_x area to my_memory area
	muse_sync(map, filesize); //drop a 'filesize' amount of bytes and write them to the file in FileSystem

	muse_unmap(map); 




	muse_free(r);
	free(y);
	muse_close();

	return 0;
}
