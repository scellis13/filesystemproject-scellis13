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
int init_freespace(int total_blocks, int block_size);

typedef struct freespace {
	int blocks_free;
	int head_block;
	int tail_block;
} freespace, *freespacePtr; //About 12 Bytes

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
	

// //	Very first steps
// //	1. buffer = malloc(block_size) //Allocate 512 bytes, and load block 0 (Volume Control Block) into the block

 	myVCB_ptr ptr;
 	LBAread(ptr, 1, 1);


	if(ptr->magic_number == MAGIC_NUMBER){
		printf("\n***Volume already formatted.***\n");
		printf("\n***Previous Block Count: %d***\n", ptr->num_of_blocks);

	} else {
//			Initialize New VCB and its variables
			

			ptr->magic_number = MAGIC_NUMBER;
			ptr->volumeSize = volumeSize;
			ptr->num_of_blocks = (volumeSize) / blockSize;
			ptr->block_size = blockSize;
			ptr->lba_frsp = 1;
			ptr->lba_frsp_blocks = init_freespace(ptr->num_of_blocks, blockSize);
			//int lba_rtdir;			//Positions to Root Directory
			ptr->free_blocks = ptr->num_of_blocks;
			printf("\n\nNumber of Data Blocks Requested: %d\n", ptr->num_of_blocks);
			printf("\n\nNumber of Node Blocks Created to track Data Blocks: %d\n", (ptr->lba_frsp_blocks));
//			Init Free Space
			

//			Init Root Directory

			LBAwrite(ptr, 1, 1);
	}

// //			If myVCBptr->magic_number == MAGIC_NUMBER //Look at the 5th set of 4 bytes
// //				{
// //					formatted already
// //				}
// //			Else
// //				{
	
// //				}
//
//
//Freespace 
//	Bitmap? One bit indicates it is allocated, one bit inidicates it is free
//	Linked List? Pointer to the next free block
//	Clustered Array (~NTFS)? map of LBA | Free Space Block Count
//
//		***int init_frsp(int num_of_blocks, int lba_startingAddress);
//
//	Allocate Freespace - I need N blocks
//
//		***int allo_frsp(int blocks_needed); //Returns address of first block allocated
//
//	Release Freespace - Block no longer needed, gives block back to Filesystem
//
//		***int release_frsp(int lba, int num_of_blocks); //0 if fail, 1 if success

//Init Root Dir
//	What is a DE?
//	How many DE will I put in my root directory?
//	What files exist in my Root Directory?
//		. 	Points to Self
//		.. 	Points to Parent (Self in this case)
}


int init_freespace(int total_blocks, int block_size){

	printf("\nCreating freespace for %d total blocks.\n", total_blocks);
	freespace * frsp_ptr = malloc(sizeof(freespace));

	frsp_ptr->blocks_free = total_blocks;
	frsp_ptr->head_block = 0;
	frsp_ptr->tail_block = total_blocks - 1; //1999 is last free block

	int freespace_buffer_size = sizeof(freespace); //Should be 32



	block_node block_node_array = NULL;
	block_node_array = malloc(sizeof(node) * total_blocks);

	for(int i = 0; i < total_blocks; i++){
		block_node_array[i].next_free_block = i+1;
	}

	block_node_array[total_blocks-1].next_free_block = -1;

	int block_node_size = sizeof(node)*total_blocks;
	printf("\nSize of Freespace Struct: %d\n", freespace_buffer_size);
	printf("\nSize of Block Node Array: %d\n", block_node_size);


	void * freespace_buffer = malloc(freespace_buffer_size + block_node_size);

	memcpy(freespace_buffer, frsp_ptr, freespace_buffer_size);// 0 - 32
	memcpy(freespace_buffer+freespace_buffer_size, block_node_array, block_node_size); // 33 - Total Array Size of Block Nodes

	int writeBlocks = ((freespace_buffer_size + block_node_size)/block_size)+1; //Should be 32 Blocks total

	printf("\nWriting %d blocks for Freespace.\n", writeBlocks);
	LBAwrite(freespace_buffer, writeBlocks, 2);

	return writeBlocks;
}