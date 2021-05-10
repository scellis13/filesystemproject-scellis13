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

#include "vol_struc.h"
#include "vol_func.h"

int create_volume(myVCB_ptr ptr, char * filename, int volumeSize, int blockSize){
	int retVal = -1;
	strcpy(ptr->volume_name, filename);
	ptr->magic_number = MAGIC_NUMBER;
	ptr->volumeSize = volumeSize;
	ptr->block_size = blockSize;
	ptr->lba_frsp = 1;			
	ptr->lba_curdir = 0;		

	init_freespace(ptr);

	init_rtdir(ptr);

	LBAwrite(ptr, 1, 0);
	retVal = 0;
	return retVal;
}


void init_freespace(myVCB_ptr ptr){
	/* Variables needed from ptr */

	int block_size = ptr->block_size;
	int volume_size = ptr->volumeSize;

	// printf("\ninit_freespace: Initializing Free Space.");
	freespace_ptr frsp_ptr;

	int original_block_request = (volume_size / block_size);

	int blocks_requested = original_block_request;
	int sum_of_blocks = 0;

	int wasted_blocks = blocks_requested;

	while(wasted_blocks > 1) {
		// printf("\n\nCurrent Sum of Allocated Blocks: %d", sum_of_blocks);
		// printf("\n\nTotal Blocks Requested: %d", blocks_requested);
		// printf("\n\tBlocks Requested: %d", blocks_requested);
		double size_adjustment = (double) sizeof(directoryEntry)/(block_size);
		// printf("\n\tSize Adjustment: %f", size_adjustment);

		int adjusted_block_amount = blocks_requested - (size_adjustment * blocks_requested) - 1;
		// printf("\n\tAdjusted Blocks Requested: %d", adjusted_block_amount);

		int total_setup_blocks = get_setup_blocks(ptr, adjusted_block_amount, block_size);

		int total_blocks_used = total_setup_blocks + adjusted_block_amount;
		// printf("\n\nTotal Blocks Used: %d", total_blocks_used);

		sum_of_blocks = sum_of_blocks + adjusted_block_amount;
		wasted_blocks = blocks_requested - total_blocks_used;
		// printf("\n\nWasted Blocks: %d", wasted_blocks);
		blocks_requested = wasted_blocks;
	
	}
	
	int new_setup_block_amount = get_setup_blocks(ptr, sum_of_blocks, block_size);

	// printf("\n\n\tAdjusted Blocks for Data Storage: %d", sum_of_blocks);
	// printf("\n\tTotal Blocks needed for setup: %d", new_setup_block_amount);
	// printf("\n\tOriginal Block Request: %d", original_block_request);
	// printf("\n\tTotal Blocks Used: %d", sum_of_blocks + new_setup_block_amount);

	ptr->num_of_blocks = sum_of_blocks + new_setup_block_amount;
	ptr->total_data_blocks = sum_of_blocks;
	ptr->volumeSize = ptr->num_of_blocks * ptr->block_size;
	
	frsp_ptr = malloc(ptr->lba_frsp_blocks);

	frsp_ptr->blocks_free = sum_of_blocks;
	frsp_ptr->total_directory_entries = 0;



	
	// printf("\n\n\n****TOTAL DATA BLOCKS LOOPING: %d\n\n\n", ptr->total_data_blocks);
	for(int i = 0; i < (ptr->total_data_blocks); i++){
		(frsp_ptr)->data_blocks_map[i] = malloc(sizeof(int));
		(frsp_ptr)->data_blocks_map[i] = 0; //0 is free, 1 is used
	}


	// printf("\n\nTotal Elements Stored So Far:");
	// printf("\n\tTotal VCB Blocks: %d", 1);
	// printf("\n\tTotal Free Space Blocks: %d", ptr->lba_frsp_blocks);
	// printf("\n\tTotal Directory Entry List Blocks: %d", ptr->lba_dirnodes_blocks);
	// printf("\n\tTotal Blocks Free: %d", frsp_ptr->blocks_free);


	LBAwrite(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);
	free(frsp_ptr);
	frsp_ptr = NULL;
}

void init_rtdir(myVCB_ptr ptr){
	// printf("\n\ninit_rtdir: Initializing Root Directory.");
	create_directory_entry(ptr, "~", 512, 0);
}

int create_directory_entry(myVCB_ptr ptr, char * file_name, int size_bytes, int entry_type) {
	int return_value = -1;

	char * type_string = (entry_type == 0) ? "Folder" : "File";
	// printf("\n\ncreate_directory_entry: Attempting to create %s (%s) of size (%d).", type_string, file_name, size_bytes);
	
	//Reusable VCB Variables
	int block_size = ptr->block_size;
	//void * buffer = malloc(block_size);
	
	int blocks_needed = (entry_type == 0) ? -1 : ceil(((double)size_bytes)/block_size);

	// printf("\n\tTotal Blocks Needed: %d", blocks_needed);

	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
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
			// printf("\nCreate DE: Found free block at (%d)", block_start_position);

			for(int j = 0; j < blocks_needed; j++){
				frsp_ptr->data_blocks_map[block_start_position+j] = 1;
			}
			frsp_ptr->blocks_free = frsp_ptr->blocks_free - blocks_needed;
		} else {
			return -2; //Sufficient Space not available
		}
	}

	if(frsp_ptr->total_directory_entries >= ptr->total_data_blocks) return -1; //Too many Directory Entries for Volume

	// printf("\n\ncreate_directory_entry: Attempting to create directory entry list:");
	// printf("\n\tSize of Directory Entry: %d", sizeof(directoryEntry));
	// printf("\n\tTotal Data Blocks: %d", ptr->total_data_blocks);
	int size_de_list_bytes = (sizeof(directoryEntry)*ptr->total_data_blocks);
	// printf("\n\tTotal Size of DE List (bytes): %d", size_de_list_bytes);
	int size_de_list_blocks = ceil((double) size_de_list_bytes / block_size);
	// printf("\n\tTotal Size of DE List (blocks): %d", size_de_list_blocks );

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
		
		// printf("\n\n\tCalculating Required Blocks for Freespace:");
		// printf("\n\tBlocks Requested: %d", blocks_requested);
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
		// printf("\n\tTotal Size of Freespace (blocks): %d", total_freespace_blocks);

		// printf("\n\n\tCalculating Required Blocks for Directory Entry List:");
		int entry_bytes = sizeof(directoryEntry);
		// printf("\n\tSize of Directory Entry Struct (bytes): %d", entry_bytes);
		int entry_list_bytes = entry_bytes * blocks_requested;
		// printf("\n\tSize of Directory Entry List (bytes): %d", entry_list_bytes);
		int total_entry_list_blocks = ceil(((double)entry_list_bytes) / block_size);
		ptr->lba_dirnodes_blocks = total_entry_list_blocks;
		ptr->lba_data_offset = ptr->lba_dirnodes + total_entry_list_blocks;
		// printf("\n\tSize of Directory Entry List (blocks): %d", total_entry_list_blocks);

		int total_setup_blocks = total_freespace_blocks + total_entry_list_blocks + 1;
		// printf("\n\nTotal Blocks Required for Initialization: %d", total_setup_blocks);

		return total_setup_blocks;
}
