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

typedef struct node {
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
	char volume_name[400];		//Name of the volume

} myVCB, *myVCB_ptr;

typedef struct directoryEntry {
	int block_id;
	int total_blocks_allocated;
	int parent_id;
	char entry_type; //0 for folder, 1 for file
	int fileSize;
	char fileName[75];
} directoryEntry, *directoryEntry_ptr;

void init_freespace(myVCB_ptr ptr, int total_blocks, int block_size);
void init_rtdir(myVCB_ptr ptr);
int create_directory_entry(myVCB_ptr ptr, char * file_name, int total_blocks, char entry_type);

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
 			int offset = ptr->lba_dirnodes_blocks + ptr->lba_frsp_blocks + 1; //Offsets to point to the directory entry/data blocks
 			int block_position = node_list[i].block_position + offset;
 			int block_id = node_list[i].block_id;
 			printf("\n%d. Node_List[%d]: Position %d", i, node_list[i].block_id, block_position);
 			printf("\n***READING Directory Entry***\n");
 			directoryEntry_ptr temp_ptr;
 			temp_ptr = malloc(ptr->block_size);
 			LBAread(temp_ptr, ptr->block_size, block_position);

 			printf("\n\tTemp Pointer block_id: %d", temp_ptr->block_id);
 			printf("\n\tTemp Pointer Total Blocks: %d", temp_ptr->total_blocks_allocated);
 			printf("\n\tTemp Pointer Parent ID: %d", temp_ptr->parent_id);
 			printf("\n\tTemp Pointer Entry Type (0) Folder, (1) File: %d", temp_ptr->entry_type);
 			printf("\n\tTemp Pointer Filename: %s", temp_ptr->fileName);
 			printf("\n\tSize of Directory Entry: %d", sizeof(directoryEntry));
 			printf("\n\tTotal Size Allocated for DE: %d", (temp_ptr->total_blocks_allocated * ptr->block_size));
 			printf("\n\tMaximum buffer size to write data: %d", (temp_ptr->total_blocks_allocated * ptr->block_size)-sizeof(directoryEntry) );
 			printf("\n\tStored File Size calculated during creation: %d", temp_ptr->fileSize);

 			
 			
 			// int actual_file_size = temp_ptr->total_blocks_allocated * ptr->block_size;
 			// int total_blocks = temp_ptr->total_blocks_allocated;
 			// LBAread(temp_ptr, total_blocks, block_position);
 			//printf("\n\tAttempting to print the buffer: %s", buffer);

 			free(temp_ptr);
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
	create_directory_entry(ptr, ".", 1*512, 0);
	create_directory_entry(ptr, "New File", 4*512, 1);
	create_directory_entry(ptr, "Newer File", 3*512, 1);
}

int create_directory_entry(myVCB_ptr ptr, char * file_name, int bytes, char entry_type){
	printf("\n\n\nCreating entry: %s", file_name);
	//Check if sequential blocks are available
	int block_start_position = -1;


	freespace_ptr frsp_ptr;
	int block_size = ptr->block_size;
	frsp_ptr = malloc((ptr->lba_frsp_blocks)*(block_size));
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);
	
	int blocks_needed;
	if(entry_type == 0) {
		blocks_needed = 1;
	} else {
		blocks_needed = ceil(((double)bytes + sizeof(directoryEntry))/block_size);
	}

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

		node * node_list;
		node_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
		LBAread(node_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);
		node_list[frsp_ptr->total_directory_entries].block_id = frsp_ptr->total_directory_entries;
		printf("\n***INSIDE CREATE FILE: Block Id: %d", node_list[frsp_ptr->total_directory_entries].block_id);
		node_list[frsp_ptr->total_directory_entries].block_position = block_start_position;
		printf("\n***INSIDE CREATE FILE: Block Position: %d", node_list[frsp_ptr->total_directory_entries].block_position);
		
		
		
		printf("\n***Writing Node_List[%d]: Position %d\n", node_list[frsp_ptr->total_directory_entries].block_id, node_list[frsp_ptr->total_directory_entries].block_position);
		

		directoryEntry_ptr de_ptr;
		de_ptr = malloc(blocks_needed * ptr->block_size);
		int offset = ptr->lba_dirnodes_blocks + ptr->lba_frsp_blocks + 1; //Offsets to point to the directory entry/data blocks
		LBAread(de_ptr, blocks_needed, block_start_position + offset);

		//Start of Writing Metadata
		printf("\nWriting Directory Entry Information to struct:");
		de_ptr->block_id = node_list[frsp_ptr->total_directory_entries].block_id;
		printf("\n\tBlock Id: %d", de_ptr->block_id);
		de_ptr->total_blocks_allocated = blocks_needed;
		printf("\n\tBlock Allocated: %d", de_ptr->total_blocks_allocated);
		de_ptr->parent_id = ptr->lba_curdir;
		printf("\n\tParent Directory ID: %d", de_ptr->parent_id);
		strcpy(de_ptr->fileName, file_name);
		printf("\n\tFile Name: %s", de_ptr->fileName);
		de_ptr->entry_type = entry_type;
		printf("\n\tEntry Type: %d", de_ptr->entry_type);
		de_ptr->fileSize = ((de_ptr->total_blocks_allocated * ptr->block_size)-sizeof(directoryEntry))-1;
		printf("\n\tFile Size: %d", de_ptr->fileSize);
		
		int fileSize = de_ptr->fileSize;

		LBAwrite(node_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);
		LBAwrite(de_ptr, blocks_needed, block_start_position + offset);

		free(de_ptr);
		free(node_list);
		frsp_ptr->total_directory_entries = frsp_ptr->total_directory_entries + 1;
		printf("\nTotal Directory Entries: %d", frsp_ptr->total_directory_entries);
		printf("\n**BLOCKS FREE (%d) - BLOCKS NEEDED (%d) = %d\n", frsp_ptr->blocks_free, blocks_needed, frsp_ptr->blocks_free - blocks_needed);
		frsp_ptr->blocks_free = frsp_ptr->blocks_free - blocks_needed;

		printf("***END OF CREATING FILE***\n\n\n");

	} else {
		printf("\nCreate DE: Cannot find free block, unable to create new directory entry.");
	}

	LBAwrite(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);
	free(frsp_ptr);
	return block_start_position;
}