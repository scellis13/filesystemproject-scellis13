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

void list_dir(myVCB_ptr ptr) {
	printf(".\t..\t");
	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

	directoryEntry * de_list;
	de_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
	LBAread(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);

	for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
		if((de_list[i].parent_id == ptr->lba_curdir) && (de_list[i].block_id != ptr->lba_rtdir)){
			printf("%d.%s\t", i, de_list[i].fileName);
		}
	}
	free(de_list);
	de_list = NULL;

	free(frsp_ptr);
	frsp_ptr = NULL;
	printf("\n");
}

void make_dir(myVCB_ptr ptr, char * dir_name) {
	int success = create_directory_entry(ptr, dir_name, -1, 0);
	if(success == -2) printf("\nmake_dir: Insufficient space. Enter command 'psm' to Print the Block Storage Map and view free space.");
}

//Prints directory name with no 'new lines' before or after
void print_dir(myVCB_ptr ptr) {

	if (ptr->lba_curdir == ptr->lba_rtdir){
		printf("~");
	} else {
		freespace_ptr frsp_ptr;
		frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
		LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);
	
		directoryEntry * de_list;
		de_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
		LBAread(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);
	
		for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
			if(ptr->lba_curdir == de_list[i].block_id) {
				printf("%s", de_list[i].fileName);
				break;
			}
		}
	
		free(de_list);
		de_list = NULL;
	
		free(frsp_ptr);
		frsp_ptr = NULL;
	}
}

void print_storage_map(myVCB_ptr ptr, int break_point) {
	
	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

	printf("\n\tBit Map Preview:");
	printf("\n\t 0 = Free Space\t1 = Used Space\n");
	printf("\n\t[");
	int loopcount = 1;
	for(int i = 0; i < ptr->total_data_blocks; i++){
		printf(" %04d:%d ", i, (frsp_ptr->data_blocks_map)[i]);
		if((loopcount % break_point == 0)) printf("\n\t ");
		loopcount++;
	}
	printf(" ]\n");

	free(frsp_ptr);
	frsp_ptr = NULL;
}

void change_dir(myVCB_ptr ptr, char * dir_name) {



	int found = 0;

	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

	directoryEntry * de_list;
	de_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
	LBAread(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);

	

	if (strcmp(dir_name, "") == 0) {
		ptr->lba_curdir = ptr->lba_rtdir;
		found = 1;
	} else if (strcmp(dir_name, "..") == 0) {
		for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
			if (de_list[i].block_id==ptr->lba_curdir){
				ptr->lba_curdir = de_list[i].parent_id;
				found = 1;
				break;
			}
		}
	} else if (strcmp(dir_name, ".") == 0) {
		//do nothing
		found = 1;
	} else {
		for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
			if( (strcmp(de_list[i].fileName, dir_name) == 0) && (de_list[i].parent_id == ptr->lba_curdir) ){
				ptr->lba_curdir = de_list[i].block_id;
				found = 1;
				break;
			}
		}
	}

	if(found == 0) { 
		printf("\nchange_dir: %s: No such file or directory.\n", dir_name);
	} 

	free(de_list);
	de_list = NULL;

	free(frsp_ptr);
	frsp_ptr = NULL;
}
