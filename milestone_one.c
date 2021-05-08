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
	int total_directory_entries;
	int data_blocks_map[];
} freespace, *freespace_ptr; //About 8 Bytes

typedef struct vcb {
	
	int volumeSize;			//Size of volume
	int block_size;			//Size of Each Block
	int num_of_blocks;		//Total Blocks in the Volume
	int total_data_blocks;	//Total Data Blocks in the Volume
	int magic_number;		//Known Value
	int lba_frsp;			//Block Position for Free Space Data
	int lba_frsp_blocks;	//How big the freespace struct is
	int lba_dirnodes;		//Block Position that starts list of directory nodes
	int lba_dirnodes_blocks;//How big the dirnode list is
	int lba_data_offset;	//Offset to datablocks
	int lba_rtdir; 			//Block Position of root directory
	int lba_curdir;			//Root Directory Block ID
	char volume_name[250];		//Name of the volume

} myVCB, *myVCB_ptr;

typedef struct directoryEntry {
	int block_id;
	int block_position;
	int total_blocks_allocated;
	int parent_id;
	int entry_type; //0 for folder, 1 for file
	char fileName[55];

} directoryEntry, *directoryEntry_ptr;

void init_freespace(myVCB_ptr ptr);
void init_rtdir(myVCB_ptr ptr);
int create_directory_entry(myVCB_ptr ptr, char * file_name, int total_blocks, int entry_type);
int get_setup_blocks(myVCB_ptr ptr, int blocks_requested, int block_size);

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

		//Testing VCB Variables
		printf("\n***Volume already formatted.***");

		printf("\n\nTesting Volume Control Struct Variables:");
			printf("\n\tVolume Size: %d", ptr->volumeSize);
			printf("\n\tBlock Size: %d", ptr->block_size);
			printf("\n\tNum of Vol. Blocks: %d", ptr->num_of_blocks);
			printf("\n\tTotal Data Blocks: %d", ptr->total_data_blocks);
			printf("\n\tMagic # Pointer (%d) == Magic # File (%d)", ptr->magic_number, MAGIC_NUMBER);
			printf("\n\tFreespace Starting Block Position: %d", ptr->lba_frsp);
			printf("\n\tFreespace Total Blocks: %d", ptr->lba_frsp_blocks);
			printf("\n\tDirectory Entry List Starting Block Position: %d", ptr->lba_dirnodes);
			printf("\n\tDirectory Entry List Total Blocks: %d", ptr->lba_dirnodes_blocks);
			printf("\n\tData Blocks Starting Block Position (Offset): %d", ptr->lba_data_offset);
			printf("\n\tRoot Directory Block ID: %d", ptr->lba_rtdir);
			printf("\n\tCurrent Directory Block ID: %d", ptr->lba_curdir);
			printf("\n\tVolume Name: %s", ptr->volume_name);

		printf("\n\nTesting Freespace Struct Variables:");

			freespace_ptr frsp_ptr;
			frsp_ptr = malloc(ptr->lba_frsp_blocks);
			LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

			printf("\n\tFreespace Blocks Free: %d", frsp_ptr->blocks_free);
			printf("\n\tFreespace Total Directory Entries: %d", frsp_ptr->total_directory_entries);
			printf("\n\tFreespace Bitmap:");
			printf("\n\t[");
			int loopcount = 1;
			for(int i = 0; i < ptr->total_data_blocks; i++){
				printf(" %04d:%d ", i, (frsp_ptr->data_blocks_map)[i]);
				if((loopcount % 10 == 0)) printf("\n\t ");
				loopcount++;
			}
			printf(" ]");

			

			printf("\n\n\tTesting Directory Entry Struct List:");

				directoryEntry * de_list;
				de_list = malloc(sizeof(directoryEntry)*frsp_ptr->total_directory_entries);
				LBAread(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);

				for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
					printf("\n\t\tDirectory Entry #%d, listing Metadata:", i);
					printf("\n\t\t\tBlock ID: %d", de_list[i].block_id);
					printf("\n\t\t\tData Block Position: %d", de_list[i].block_position);
					printf("\n\t\t\tTotal Blocks Allocated: %d", de_list[i].total_blocks_allocated);
					printf("\n\t\t\tParent ID: %d", de_list[i].parent_id);
					printf("\n\t\t\tEntry Type: %d", de_list[i].entry_type);
					printf("\n\t\t\tFile Name: %s", de_list[i].fileName);
				}

				free(de_list);
				de_list = NULL;

			free(frsp_ptr);
			frsp_ptr = NULL;

	} else {
			strcpy(ptr->volume_name,argv[1]);
			ptr->magic_number = MAGIC_NUMBER;
			ptr->volumeSize = volumeSize;
			ptr->block_size = blockSize;
			ptr->lba_frsp = 1;			
			ptr->lba_curdir = 0;		

			init_freespace(ptr);

			init_rtdir(ptr);

			LBAwrite(ptr, 1, 0);	
	}
	printf("\n\n***End of main***\n");
	free(ptr);
	ptr = NULL;
}


void init_freespace(myVCB_ptr ptr){
	//int blocks_free;
	//int total_directory_entries;
	//int free_blocks_map[];

	/* Variables needed from ptr */

	int block_size = ptr->block_size;
	int volume_size = ptr->volumeSize;

	printf("\ninit_freespace: Initializing Free Space.");
	freespace_ptr frsp_ptr;

	int original_block_request = (volume_size / block_size);

	int blocks_requested = original_block_request;
	int sum_of_blocks = 0;

	int wasted_blocks = blocks_requested;

	while(wasted_blocks > 1) {
		//printf("\n\nCurrent Sum of Allocated Blocks: %d", sum_of_blocks);
		//printf("\n\nTotal Blocks Requested: %d", blocks_requested);
		//printf("\n\tBlocks Requested: %d", blocks_requested);
		double size_adjustment = (double) sizeof(directoryEntry)/(block_size);
		//printf("\n\tSize Adjustment: %f", size_adjustment);

		int adjusted_block_amount = blocks_requested - (size_adjustment * blocks_requested) - 1;
		//printf("\n\tAdjusted Blocks Requested: %d", adjusted_block_amount);

		int total_setup_blocks = get_setup_blocks(ptr, adjusted_block_amount, block_size);

		int total_blocks_used = total_setup_blocks + adjusted_block_amount;
		//printf("\n\nTotal Blocks Used: %d", total_blocks_used);

		sum_of_blocks = sum_of_blocks + adjusted_block_amount;
		wasted_blocks = blocks_requested - total_blocks_used;
		//printf("\n\nWasted Blocks: %d", wasted_blocks);
		blocks_requested = wasted_blocks;
	
	}
	
	int new_setup_block_amount = get_setup_blocks(ptr, sum_of_blocks, block_size);

	printf("\n\n\tAdjusted Blocks for Data Storage: %d", sum_of_blocks);
	printf("\n\tTotal Blocks needed for setup: %d", new_setup_block_amount);
	printf("\n\tOriginal Block Request: %d", original_block_request);
	printf("\n\tTotal Blocks Used: %d", sum_of_blocks + new_setup_block_amount);

	ptr->num_of_blocks = sum_of_blocks + new_setup_block_amount;
	ptr->total_data_blocks = sum_of_blocks;
	ptr->volumeSize = ptr->num_of_blocks * ptr->block_size;
	
	frsp_ptr = malloc(ptr->lba_frsp_blocks);

	frsp_ptr->blocks_free = sum_of_blocks;
	frsp_ptr->total_directory_entries = 0;



	
	printf("\n\n\n****TOTAL DATA BLOCKS LOOPING: %d\n\n\n", ptr->total_data_blocks);
	for(int i = 0; i < (ptr->total_data_blocks); i++){
		(frsp_ptr)->data_blocks_map[i] = malloc(sizeof(int));
		(frsp_ptr)->data_blocks_map[i] = 0; //0 is free, 1 is used
	}
	//void * buffer = malloc(512);


	printf("\n\nTotal Elements Stored So Far:");
	printf("\n\tTotal VCB Blocks: %d", 1);
	printf("\n\tTotal Free Space Blocks: %d", ptr->lba_frsp_blocks);
	printf("\n\tTotal Directory Entry List Blocks: %d", ptr->lba_dirnodes_blocks);
	printf("\n\tTotal Blocks Free: %d", frsp_ptr->blocks_free);


	LBAwrite(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);
	free(frsp_ptr);
	frsp_ptr = NULL;
}

void init_rtdir(myVCB_ptr ptr){
	printf("\n\ninit_rtdir: Initializing Root Directory.");

	create_directory_entry(ptr, ".", 512, 0);
	create_directory_entry(ptr, "New Folder", 13423, 0);
	create_directory_entry(ptr, "File.txt", 1024, 1);
	create_directory_entry(ptr, "New File.txt", 10000, 1);
}

int create_directory_entry(myVCB_ptr ptr, char * file_name, int size_bytes, int entry_type){
	int return_value = -1;

	char * type_string = (entry_type == 0) ? "Folder" : "File";
	printf("\n\ncreate_directory_entry: Attempting to create %s (%s) of size (%d).", type_string, file_name, size_bytes);
	
	//Reusable VCB Variables
	int block_size = ptr->block_size;
	//void * buffer = malloc(block_size);
	
	int blocks_needed = (entry_type == 0) ? -1 : ceil(((double)size_bytes)/block_size);

	printf("\n\tTotal Blocks Needed: %d", blocks_needed);

	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(ptr->lba_frsp_blocks);
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

	//Get First Block of Free Space that can store the requested size
	int count = 0;
	int block_start_position = -1;

	if(entry_type != 0){
		for(int i = 0; i < ptr->total_data_blocks; i++){
			
			if(frsp_ptr->data_blocks_map[i] == 0) {
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
				frsp_ptr->data_blocks_map[block_start_position+j] = 1;
			}
			frsp_ptr->blocks_free = frsp_ptr->blocks_free - blocks_needed;
		} else {
			return -1; //Sufficient Space not available
		}
	}

	if(frsp_ptr->total_directory_entries >= ptr->total_data_blocks) return -1; //Too many Directory Entries for Volume

	printf("\n\ncreate_directory_entry: Attempting to create directory entry list:");
	printf("\n\tSize of Directory Entry: %d", sizeof(directoryEntry));
	printf("\n\tTotal Data Blocks: %d", ptr->total_data_blocks);
	int size_de_list_bytes = (sizeof(directoryEntry)*ptr->total_data_blocks);
	printf("\n\tTotal Size of DE List (bytes): %d", size_de_list_bytes);
	int size_de_list_blocks = ceil((double) size_de_list_bytes / block_size);
	printf("\n\tTotal Size of DE List (blocks): %d", size_de_list_blocks );

	directoryEntry * de_list;
	de_list = malloc(size_de_list_blocks * block_size);
	LBAread(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);



	de_list[frsp_ptr->total_directory_entries].block_id = frsp_ptr->total_directory_entries;
	de_list[frsp_ptr->total_directory_entries].block_position = block_start_position;
	de_list[frsp_ptr->total_directory_entries].total_blocks_allocated = blocks_needed;
	de_list[frsp_ptr->total_directory_entries].parent_id = ptr->lba_curdir;
	de_list[frsp_ptr->total_directory_entries].entry_type = entry_type;
	strcpy(de_list[frsp_ptr->total_directory_entries].fileName, file_name);

	LBAwrite(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);

	return_value = frsp_ptr->total_directory_entries;

	frsp_ptr->total_directory_entries = frsp_ptr->total_directory_entries + 1;
	LBAwrite(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);


	free(frsp_ptr);
	free(de_list);
	frsp_ptr = NULL;
	de_list = NULL;

	return return_value;
}

int get_setup_blocks(myVCB_ptr ptr, int blocks_requested, int block_size){
		
		printf("\n\n\tCalculating Required Blocks for Freespace:");
		printf("\n\tBlocks Requested: %d", blocks_requested);
		int freespace_bytes = sizeof(freespace);
		int bitmapArr[blocks_requested];
		int size_bitmap = sizeof(bitmapArr);
		int total_freespace_bytes = sizeof(freespace) + size_bitmap;
		// printf("\n\tSize of Freespace Struct (bytes): %d", freespace_bytes);
		// printf("\n\tSize of Bitmap (bytes): %d", size_bitmap);
		// printf("\n\tTotal Size of Freespace (bytes): %d", total_freespace_bytes);
		int total_freespace_blocks = ceil( ((double) total_freespace_bytes)/block_size );
		ptr->lba_frsp_blocks = total_freespace_blocks;
		ptr->lba_dirnodes = 1 + total_freespace_blocks;//0 VCB, 1 FS + FS_BLOCKS
		printf("\n\tTotal Size of Freespace (blocks): %d", total_freespace_blocks);

		//printf("\n\n\tCalculating Required Blocks for Directory Entry List:");
		int entry_bytes = sizeof(directoryEntry);
		//printf("\n\tSize of Directory Entry Struct (bytes): %d", entry_bytes);
		int entry_list_bytes = entry_bytes * blocks_requested;
		//printf("\n\tSize of Directory Entry List (bytes): %d", entry_list_bytes);
		int total_entry_list_blocks = ceil(((double)entry_list_bytes) / block_size);
		ptr->lba_dirnodes_blocks = total_entry_list_blocks;
		ptr->lba_data_offset = ptr->lba_dirnodes + total_entry_list_blocks;
		//printf("\n\tSize of Directory Entry List (blocks): %d", total_entry_list_blocks);

		int total_setup_blocks = total_freespace_blocks + total_entry_list_blocks + 1;
		//printf("\n\nTotal Blocks Required for Initialization: %d", total_setup_blocks);

		return total_setup_blocks;
}
