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

typedef struct directory_node {
	int block_id;
	int block_position;
} node, *node_ptr; //About 8 Bytes * free blocks

typedef struct freespace {
	int blocks_free;
	int total_directory_entries;
	char free_blocks_map[];
} freespace, *freespace_ptr; //About 8 Bytes

typedef struct vcb {
	
	int volumeSize;
	int num_of_blocks;		//Total data blocks in the Volume
	int block_size;			//Size of Each Block
	int magic_number;		//Known Value
	int lba_frsp;			//Block Position for Free Space Data
	int lba_frsp_blocks;	//How big the freespace struct is
	int lba_dirnodes;		//Block Position that starts list of directory nodes
	int lba_dirnodes_blocks;//How big the dirnode list is
	int lba_rtdir; 			//Block Position of root directory
	int lba_curdir;			//Root Directory Block ID
	char volume_name[];		//Name of the volume

} myVCB, *myVCB_ptr;

typedef struct directoryEntry {
	int block_id;
	int total_blocks_allocated;
	int parent_id;
	char entry_type; //0 for folder, 1 for file
	char * fileName;
} directoryEntry, *directoryEntry_ptr;

void init_freespace(myVCB_ptr ptr, int total_blocks, int block_size);
void init_rtdir(myVCB_ptr ptr);
int create_directory_entry(myVCB_ptr ptr, char * file_name, int total_blocks);

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
 	ptr = malloc(blockSize);
 	LBAread(ptr, 1, 0);


	if(ptr->magic_number == MAGIC_NUMBER){

		//Testing VCB Variables
		printf("\n***Volume already formatted.***\n");
		printf("\n***Volume name: %s\n", ptr->volume_name);
		printf("\n***Magic Number in Pointer: %d***\n", ptr->magic_number);
		printf("\n***Magic Number in File: %d***\n", MAGIC_NUMBER);
		printf("\n***Block Size: %d***\n", ptr->block_size);
		printf("\n***Num of Blocks: %d***\n", ptr->num_of_blocks);
		printf("\n***Total Free Space Blocks: %d***\n", ptr->lba_frsp_blocks);


		//Testing Freespace Variables
		freespace_ptr frsp_ptr;
		frsp_ptr = malloc(blockSize);
 		LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

 		printf("\n***Total Blocks Available***%d\n", frsp_ptr->blocks_free);
 		printf("\n***Is the root directory free? %d\n", frsp_ptr->free_blocks_map[0]);


 		//Testing Node List
 		node * node_list;
		node_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
		LBAread(node_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);
 		
 		printf("\n\n***READING Directory Node List***\n");
 		for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
 			printf("\n%d. Node_List[%d]: Position %d\n", i, node_list[i].block_id, node_list[i].block_position);
 		}
 		printf("\n***END OF READ***\n");

	} else {
//			Initialize New VCB and its variables
			strcpy(ptr->volume_name,argv[1]);
			ptr->magic_number = MAGIC_NUMBER;
			ptr->volumeSize = volumeSize;
			ptr->block_size = blockSize;
			ptr->lba_frsp = 1;			//Block Position for Free Space Data
			ptr->lba_curdir = 0;		//Block Position for Current Directory, Set to 0 during initialization
			init_freespace(ptr, volumeSize, blockSize);

			init_rtdir(ptr);
			//ptr->lba_curdir;			//Root Directory Block ID
			printf("\n\nNumber of Data Blocks Allowed: %d\n", ptr->num_of_blocks);
			//printf("\n\nNumber of Node Blocks Created to track Data Blocks: %d\n", (*ptr)->lba_frsp_blocks);

			LBAwrite(ptr, 1, 0);	
	}
}


void init_freespace(myVCB_ptr ptr, int volumeSize, int block_size){
	//int blocks_free;
	//int total_directory_entries;
	//int free_blocks_map[];

	freespace_ptr frsp_ptr;

	int blocks_requested = volumeSize/block_size;
	char arr[volumeSize/block_size];
	printf("\nNum of blocks: %d", blocks_requested);
	printf("\nSize of arr: %d", ((int) sizeof(arr)));
	printf("\nTotal Size of Freespace: %d", (sizeof(arr)+sizeof(freespace)));
	int freespace_blocks = ceil(((double)(sizeof(arr)+sizeof(freespace))/block_size) );
	printf( "\nTotal Blocks for Freespace: %d", freespace_blocks );


	printf("\nSize of Directory Node: %d", sizeof(node));

	int directory_node_bytes = (sizeof(node) * blocks_requested-freespace_blocks);
	int directory_node_blocks = ceil(((double)directory_node_bytes)/block_size);
	printf("\nSize of Directory Node (Bytes): %d", directory_node_bytes);
	printf("\nTotal Blocks for Node List (Blocks): %d", directory_node_blocks);

	int storage_blocks_allowed = blocks_requested - (freespace_blocks + directory_node_blocks + 1);

	printf("\n\nStorage Blocks Allowed: %d", storage_blocks_allowed);
	char actualArr[storage_blocks_allowed];
	freespace_blocks = ceil( ((double)(sizeof(actualArr)+sizeof(freespace))/block_size) );
	printf("\nFree Space Blocks: %d", freespace_blocks);
	directory_node_bytes = (sizeof(node)*storage_blocks_allowed);
	directory_node_blocks = ceil(((double)directory_node_bytes)/block_size);
	printf("\nDirectory Node List Blocks: %d", directory_node_blocks);
	printf("\nVCB Blocks: %d", 1);

	//First get total block size possible.
	
	frsp_ptr = malloc(freespace_blocks*block_size);

	frsp_ptr->blocks_free = storage_blocks_allowed;
	frsp_ptr->total_directory_entries = 0;

	for(int i = 0; i < storage_blocks_allowed; i++){
		frsp_ptr->free_blocks_map[i] = 0;
	}

	ptr->lba_frsp_blocks = freespace_blocks;
	ptr->num_of_blocks = storage_blocks_allowed;
	ptr->lba_dirnodes = ptr->lba_frsp + freespace_blocks;
	ptr->lba_dirnodes_blocks = directory_node_blocks;

	LBAwrite(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);
	free(frsp_ptr);
}

void init_rtdir(myVCB_ptr ptr){
	///Get first free block
	create_directory_entry(ptr, ".", 1*512);
	create_directory_entry(ptr, "New Folder", 4*512);
	create_directory_entry(ptr, "Newer Folder", 3*512);
}

int create_directory_entry(myVCB_ptr ptr, char * file_name, int bytes){
	printf("\nCreating entry: %s", file_name);
	//Check if sequential blocks are available
	int block_start_position = -1;


	freespace_ptr frsp_ptr;
	int block_size = ptr->block_size;
	frsp_ptr = malloc((ptr->lba_frsp_blocks)*(block_size));
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);
	int blocks_needed = ceil((double)bytes/block_size);

	int count = 0;
	
	for(int i = 0; i < ptr->num_of_blocks; i++){
		
		if(frsp_ptr->free_blocks_map[i] == 0) {
			//Begin count
			count++;
			if(count == blocks_needed) {
				block_start_position = (i+1) - blocks_needed; 
				break;
			}
		} else {
			count = 0;
		}
	}
	if(block_start_position > -1) {
		printf("\nCreate DE: Found free block at (%d)", block_start_position);

		for(int j = 0; j < blocks_needed; j++){
			frsp_ptr->free_blocks_map[block_start_position+j] = 1;
		}

		

		node_ptr new_dir_node;
		new_dir_node = malloc(sizeof(node));
		new_dir_node->block_id = frsp_ptr->total_directory_entries;
		new_dir_node->block_position = block_start_position;

		node * node_list;
		node_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
		LBAread(node_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);
		printf("\nSize of Node List: %d", sizeof(node_list));
		node_list[frsp_ptr->total_directory_entries] = *new_dir_node;

		frsp_ptr->total_directory_entries = frsp_ptr->total_directory_entries + 1;
		printf("\n**BLOCKS FREE (%d) - BLOCKS NEEDED (%d) = %d\n", frsp_ptr->blocks_free, blocks_needed, frsp_ptr->blocks_free - blocks_needed);
		frsp_ptr->blocks_free = frsp_ptr->blocks_free - blocks_needed;

		LBAwrite(node_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);

		printf("\n***Writing Node_List[%d]: Position %d\n", node_list[frsp_ptr->total_directory_entries].block_id, node_list[frsp_ptr->total_directory_entries].block_position);

		free(new_dir_node);
		free(node_list);

	} else {
		printf("\nCreate DE: Cannot find free block, unable to create new directory entry.");
	}

	LBAwrite(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);
	free(frsp_ptr);
	return block_start_position;
}