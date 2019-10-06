#import "sac_servidor.h"

void writeNodeTable(GBlock *punteroDisco){
    GFile *nodeTable = (GFile*) punteroDisco;

    for(int nFile = 0; nFile < MAX_NUMBER_OF_FILES; nFile++){
        nodeTable[nFile].state = 0;
    }
}

void writeHeader(GBlock *punteroDisco){
    Header nuevoHeader = (Header*) punteroDisco;

    memcpy(nuevoHeader->identificador, "SAC", MAGIC_NUMBER_NAME);
    nuevoHeader->bitmap_size = BITMAP_SIZE_BLOCKS;
    nuevoHeader->bitmap_start = BITMAP_START_BLOCK;
    nuevoHeader->version = 1;
}

/*
void writeBitmap(GBlock *punteroDisco){
    int* blockAsBytes = (int*) punteroDisco;
    memset(blockAsBytes, '\0', BLOQUE_SIZE * BITMAP_SIZE_BLOCKS);


    int bitsToSet = 1 + BITMAP_SIZE_BLOCKS + MAX_NUMBER_OF_FILES;
    
    for(int bit = 0, bit < bitsToSet; bit++){

    }
}
*/