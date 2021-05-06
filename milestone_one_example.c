#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include "fsLow.h"
#include "mfs.h"
//Init VCB
//	Logical Block - 0 - Contains Info to keep track of the volume
//		Check Magic Number to see if i've been initialized before or not
//			If it is mine, no need to init
//			If it isn't mine, free space
//
#define MAGIC_NUMBER 0x92073783 //STUDENT_ID minus last digit


typedef struct freespace {
	int blocks_free;
	int head_block;
	int tail_block;
} freespace, *freespace_ptr; //About 12 Bytes

typedef struct block_node {
	int next_free_block;
} node, *block_node;

typedef struct vcb {
	int volumeSize;
	int num_of_blocks;		//Total data blocks in the Volume
	int block_size;			//Size of Each Block
	int lba_frsp;			//Positions to Free Space
	int lba_frsp_blocks;	//Total number of blocks used for freespace struct / linked list
	int lba_rtdir;			//Positions to Root Directory
	int magic_number;		//Known Value
	int free_blocks;		//Total Number of Blocks in 
								//volume that are unallocated
	freespace * frsp_ptr; 	//Not persistent

} myVCB, *myVCB_ptr;



typedef struct directoryEntry {
	struct directoryEntry * parentDirectoryEntry;
	char * fileName;
} directoryEntry, *directoryEntry_ptr;

int init_freespace(int total_blocks, int block_size);


int main (int argc, char *argv[])
	{	

	char * filename;
	uint64_t volumeSize;
	uint64_t blockSize;
    int retVal;
    
	if (argc > 3)
		{
		filename = argv[1];
		volumeSize = atoll (argv[2]);
		blockSize = atoll (argv[3]);
		}
	else
		{
		printf ("Usage: fsLowDriver volumeFileName volumeSize blockSize\n");
		return -1;
		}
		
	retVal = startPartitionSystem (filename, &volumeSize, &blockSize);	
	printf("Opened %s, Volume Size: %llu;  BlockSize: %llu; Return %d\n", filename, (ull_t)volumeSize, (ull_t)blockSize, retVal);
	

 	myVCB_ptr ptr;
 	ptr = malloc(blockSize);
 	LBAread(ptr, 1, 0);


	if(ptr->magic_number == MAGIC_NUMBER){
		printf("\n***Volume already formatted.***\n");
		printf("\n***Previous Block Count: %d***\n", ptr->num_of_blocks);
		printf("\n***Magic Number in Pointer: %d***\n", ptr->magic_number);
		printf("\n***Magic Number in File: %d***\n", MAGIC_NUMBER);
		printf("\n***Block Size: %d***\n", ptr->block_size);
		printf("\n***Num of Blocks: %d***\n", ptr->num_of_blocks);

		freespace_ptr frsp_ptr;
		frsp_ptr = malloc(blockSize);
 		LBAread(frsp_ptr, 1, 1);

 		printf("\n******** PRINTING ********%d\n", frsp_ptr->blocks_free);


	} else {	

			ptr->magic_number = MAGIC_NUMBER;
			ptr->volumeSize = volumeSize;
			ptr->num_of_blocks = (volumeSize) / blockSize;
			ptr->block_size = blockSize;
			ptr->lba_frsp = 1;
			ptr->lba_frsp_blocks = init_freespace(ptr->num_of_blocks, blockSize);
			//int lba_rtdir;			//Positions to Root Directory
			ptr->free_blocks = ptr->num_of_blocks;
			printf("\n\nNumber of Data Blocks Requested: %d\n", ptr->num_of_blocks);
			//printf("\n\nNumber of Node Blocks Created to track Data Blocks: %d\n", (*ptr)->lba_frsp_blocks);

			LBAwrite(ptr, 1, 0);
	}
}


int init_freespace(int total_blocks, int block_size){

	printf("\nCreating freespace for %d total blocks.\n", total_blocks);
	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(block_size);
	frsp_ptr->blocks_free = total_blocks;
	frsp_ptr->head_block = 0;
	frsp_ptr->tail_block = total_blocks - 1; 
	printf("\n******** PRINTING FS BLOCKS FREE ********%d\n", frsp_ptr->blocks_free);
	LBAwrite(frsp_ptr, 1, 1);

	//***Different Attempt***
	//void * buffer = malloc(block_size);
	//memcpy(buffer, *frsp_ptr, block_size);
	//LBAwrite(buffer, 1, 1);
	
	return total_blocks;
}