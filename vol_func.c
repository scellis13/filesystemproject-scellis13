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
	printf("\033[1;34m.\t..\t");
	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

	directoryEntry * de_list;
	de_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
	LBAread(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);

	for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
		if((de_list[i].parent_id == ptr->lba_curdir) && (de_list[i].block_id != ptr->lba_rtdir)){
			if(de_list[i].entry_type == 0) {
				printf("\033[1;34m%s\t", de_list[i].fileName);
			} else {
				printf("\033[0m%s\t", de_list[i].fileName);
			}
		}
	}
	free(de_list);
	de_list = NULL;

	free(frsp_ptr);
	frsp_ptr = NULL;
	printf("\n");
}

void make_dir(myVCB_ptr ptr, char * dir_name) {
	int success = create_directory_entry(ptr, dir_name, -1, 0, 0);
	if(success == -1) printf("make_dir: Too many entries than volume can handle. Please delete some files and try again.\n");
	if(success == -2) printf("make_dir: Insufficient space. Enter command 'psm' to Print the Block Storage Map and view free space.\n");
	if(success == -3) printf("make_dir: Entry already exists.\n");
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
	
		const char * file_names[frsp_ptr->total_directory_entries * 55];
		int num_files = 0;

		int curdir = ptr->lba_curdir;
		int parent = -1;
		// for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
		// 	if(ptr->lba_curdir == de_list[i].block_id) {
		// 		printf("%s", de_list[i].fileName);
		// 		file_names[num_files] = de_list[i].fileName;
		// 		num_files++;
		// 		parent = de_list[i].parent_id;
		// 		break;
		// 	}
		// }
		for(int i = (frsp_ptr->total_directory_entries); i >= 0; i--){
			if(de_list[i].block_id == curdir){
				file_names[num_files] = de_list[i].fileName;
				num_files++;
				curdir = de_list[i].parent_id;
			}
		}

		for(int i = num_files-1; i >= 0; i--){
			if(i == 0) { printf("%s", file_names[i]); break;}
			printf("%s/", file_names[i]);
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
		printf("\033[33m");
		if(frsp_ptr->data_blocks_map[i] == 0) {
			printf(" %4d\033[0m:\033[32m%d ", i, (frsp_ptr->data_blocks_map)[i]);
		} else {
			printf(" %4d\033[0m:\033[31m%d ", i, (frsp_ptr->data_blocks_map)[i]);
		}
		if((loopcount % break_point == 0)) printf("\n\t ");
		loopcount++;
	}
	printf("\033[0m ]\n");

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

	

	if (strcmp(dir_name, "~") == 0) {
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
		printf("change_dir: %s: No such file or directory.\n", dir_name);
	} 

	free(de_list);
	de_list = NULL;

	free(frsp_ptr);
	frsp_ptr = NULL;
}

void print_history(myVCB_ptr ptr){
	// printf("print_history: Starting buffer...");
	char * historyBuffer = malloc(ptr->lba_history_blocks * ptr->block_size);
	LBAread(historyBuffer, ptr->lba_history_blocks, ptr->lba_history_pos);
	int i = 0;
	while(i < strlen(historyBuffer)){
		printf("%c", historyBuffer[i]);
		i++;
	}
	
	free(historyBuffer);
	historyBuffer = NULL;
	// printf("\nprint_history: Closing buffer...\n");
}

void write_history(myVCB_ptr ptr, char * command){
	// printf("write_history: Starting buffer...");
	// printf("\nwrite_history: length of command: %d", strlen(command));
	// printf("\nwrite_history: size of command: %d", sizeof(command));
	char tempcommand[strlen(command)+2];
	// printf("\nwrite_history: length of tempcommand: %d", strlen(tempcommand));
	// printf("\nwrite_history: size of tempcommand: %d", sizeof(tempcommand));
	for(int i = 0; i < strlen(command); i++){
		tempcommand[i] = command[i];
	}
	tempcommand[strlen(command)] = '\n';
	tempcommand[strlen(command)+1] = '\0';
	//printf("\nwrite_history: size of before command to write: %d", sizeof(tempcommand));
	




	//printf("\nwrite_history: size of after command to write: %d", sizeof(tempcommand));
	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

	char * historyBuffer = malloc(ptr->lba_history_blocks * ptr->block_size);
	LBAread(historyBuffer, ptr->lba_history_blocks, ptr->lba_history_pos);

	//printf("\nwrite_history: Last Character %d: %c", strlen(command), command[strlen(command)-1]);
	//command[strlen(command)] = '\n';
	//printf("\nwrite_history: Last Character %d: %c", strlen(command), command[strlen(command)-1]);
	
	//printf("\nwrite_history: strlen of command to write: %d", strlen(command));
	//printf("\nwrite_history: size of command to write: %d", strlen(command));
	//printf("\nwrite_history: index position of history: %d", frsp_ptr->lba_history_index);
	if(frsp_ptr->lba_history_index + strlen(tempcommand) >= (ptr->block_size * ptr->lba_history_blocks)){
		frsp_ptr->lba_history_index = 0;
		char * wipeBuffer = malloc(ptr->lba_history_blocks * ptr->block_size);
		memcpy(historyBuffer, wipeBuffer, ptr->lba_history_blocks*ptr->block_size);
		free(wipeBuffer);
		wipeBuffer = NULL;
	} else {
		memcpy(historyBuffer + frsp_ptr->lba_history_index, tempcommand, strlen(tempcommand));
		frsp_ptr->lba_history_index = frsp_ptr->lba_history_index + strlen(tempcommand);
		//printf("\nwrite_history: index position of history: %d", frsp_ptr->lba_history_index);
	}


	LBAwrite(historyBuffer, ptr->lba_history_blocks, ptr->lba_history_pos);

	free(historyBuffer);
	historyBuffer = NULL;

	LBAwrite(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

	free(frsp_ptr);
	frsp_ptr = NULL;

	// printf("\nwrite_history: Closing buffer...\n");

}


void remove_entry(myVCB_ptr ptr, char * file_name){

	if( strcasecmp(file_name, "~") == 0) {
		printf("remove_entry: Cannot delete the root directory and its contents, sorry.\n");
		return;
	}

	int removal_successful = 0;
	int remove_id = -1;
	int entry_type = -1;
	
	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

	directoryEntry * de_list;
	de_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
	LBAread(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);

	int parent_chain_id[frsp_ptr->total_directory_entries];
	int parents_to_remove = 0;

	//Find directory entry to remove
	for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
		if((strcasecmp(de_list[i].fileName, file_name) == 0) && (de_list[i].parent_id == ptr->lba_curdir)){
			remove_id = de_list[i].block_id;
			parent_chain_id[parents_to_remove] = remove_id;
			parents_to_remove++;
			entry_type = de_list[i].entry_type;
		}
	}

	if(remove_id <= 0) { 
		printf("remove_entry: Invalid filename within current working directory.\n");
		free(de_list);
		de_list = NULL;

		free(frsp_ptr);
		frsp_ptr = NULL;
		return;
	}

	//Find all sub-directories if entry_type == 0
	if(entry_type == 0) { //Remove Folder and Subfolders

		//Prompt user to enter Y or N to continue removing this folder and subfolders and their contents.
		char user_command[100];
		char buffer[100];
		char firstCommand[10];
		char secondCommand[55];

		printf("remove_entry: ");
        printf("\033[1;31mWARNING");
        printf("\033[0m:");
        printf(" Removing directory '%s' will remove itself and all its subfolders and files.", file_name);
        printf("\nEnter the character 'Y' to continue or any other key to cancel deletion: ");
        gets(user_command);

        if(strcmp(user_command, "Y") != 0) {
        	printf("remove_entry: Canceling directory removal.\n");
			free(de_list);
			de_list = NULL;

			free(frsp_ptr);
			frsp_ptr = NULL;
			return;
        }

	    


		int i = 0;

		while(i < parents_to_remove){
			for(int j = 0; j < frsp_ptr->total_directory_entries; j++){
				if(de_list[j].parent_id == parent_chain_id[i]) {
					//printf("\nremove_entry: Found block id: %d with parent_id: %d.", de_list[j].block_id, de_list[j].parent_id);
					parent_chain_id[parents_to_remove] = de_list[j].block_id;
					parents_to_remove++;
				}
			}
			i++;
		}

		//Remove directory sub-trees and files
		for(int i = parents_to_remove-1; i >= 0; i--){
			for(int j = frsp_ptr->total_directory_entries-1; j >= 0; j--){
				if(de_list[j].parent_id == parent_chain_id[i]){
					if(de_list[j].entry_type == 1){
						for(int l = de_list[j].block_position; l < de_list[j].total_blocks_allocated+de_list[j].block_position; l++){
							(frsp_ptr)->data_blocks_map[l] = 0; //0 is free, 1 is used
						}
						frsp_ptr->blocks_free = frsp_ptr->blocks_free + de_list[j].total_blocks_allocated;
					}

					for(int k = j; k < frsp_ptr->total_directory_entries; k++){
						
						de_list[k].block_id = de_list[k+1].block_id;
						de_list[k].block_position = de_list[k+1].block_position;
						de_list[k].total_blocks_allocated = de_list[k+1].total_blocks_allocated;
						de_list[k].parent_id = de_list[k+1].parent_id;
						de_list[k].file_size = de_list[k+1].file_size;
						de_list[k].entry_type = de_list[k+1].entry_type; //0 for folder, 1 for file
						strcpy(de_list[k].fileName, de_list[k+1].fileName);
						
					}
					removal_successful = 1;
					frsp_ptr->total_directory_entries = frsp_ptr->total_directory_entries - 1;
				}
			}
			
		}

		//Remove the actual directory
		for(int j = frsp_ptr->total_directory_entries-1; j >= 0; j--){
			if(de_list[j].block_id == remove_id){
				for(int k = j; k < frsp_ptr->total_directory_entries-1; k++){
					de_list[k].block_id = de_list[k+1].block_id;
					de_list[k].block_position = de_list[k+1].block_position;
					de_list[k].total_blocks_allocated = de_list[k+1].total_blocks_allocated;
					de_list[k].parent_id = de_list[k+1].parent_id;
					de_list[k].file_size = de_list[k+1].file_size;
					de_list[k].entry_type = de_list[k+1].entry_type; //0 for folder, 1 for file
					strcpy(de_list[k].fileName, de_list[k+1].fileName);
				}
				frsp_ptr->total_directory_entries = frsp_ptr->total_directory_entries - 1;
				removal_successful = 1;
				break;
			}
		}





	} else { //Remove file

		for(int j = frsp_ptr->total_directory_entries-1; j >= 0; j--){
			if(de_list[j].block_id == remove_id){

				for(int i = de_list[j].block_position; i < de_list[j].total_blocks_allocated+de_list[j].block_position; i++){
					(frsp_ptr)->data_blocks_map[i] = 0; //0 is free, 1 is used
				}
				frsp_ptr->blocks_free = frsp_ptr->blocks_free + de_list[j].total_blocks_allocated;

				for(int k = j; k < frsp_ptr->total_directory_entries; k++){
					de_list[k].block_id = de_list[k+1].block_id;
					de_list[k].block_position = de_list[k+1].block_position;
					de_list[k].total_blocks_allocated = de_list[k+1].total_blocks_allocated;
					de_list[k].parent_id = de_list[k+1].parent_id;
					de_list[k].file_size = de_list[k+1].file_size;
					de_list[k].entry_type = de_list[k+1].entry_type; //0 for folder, 1 for file
					strcpy(de_list[k].fileName, de_list[k+1].fileName);
				}
				frsp_ptr->total_directory_entries = frsp_ptr->total_directory_entries - 1;
				removal_successful = 1;
				break;
			}
		}
	}
	
	if(removal_successful == 0){
		
		printf("remove_entry: Unable to remove '%s'.", file_name);
	} else {
		LBAwrite(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);
		LBAwrite(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);
		printf("remove_entry: Successfully removed '%s'.", file_name);
	}
	printf("\n");
	free(de_list);
	de_list = NULL;

	free(frsp_ptr);
}

void print_all(myVCB_ptr ptr){
	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

	directoryEntry * de_list;
	de_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
	LBAread(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);

	printf("\033[4m Running List of Directory Entries\033[0m\n");
	for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
		int entry_type = de_list[i].entry_type;
		char * type_string = (entry_type == 0) ? "Folder" : "File";

		if(entry_type == 0){
			printf(" %4d. \033[1;34m%8s \033[0m%16s\t:\tBlock ID (%d)\t:\tParent ID (%d)\n", i+1, type_string, de_list[i].fileName, de_list[i].block_id, de_list[i].parent_id);
	
		} else {
			printf(" %4d. %8s %16s\t:\tBlock ID (%d)\t:\tParent ID (%d)\n", i+1, type_string, de_list[i].fileName, de_list[i].block_id, de_list[i].parent_id);
		}
	}
	free(de_list);
	de_list = NULL;

	free(frsp_ptr);
	frsp_ptr = NULL;
}

void copy_to_linux(myVCB_ptr ptr, char * secondCommand){
	//printf("copy_to_linux: Attempting to copy '%s' to the linux system.\n", secondCommand);
	freespace_ptr frsp_ptr;
	frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
	LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

	directoryEntry * de_list;
	de_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
	LBAread(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);
	int found = 0;

	for(int i = 0; i < frsp_ptr->total_directory_entries; i++){
		if((de_list[i].parent_id == ptr->lba_curdir) && (strcmp(de_list[i].fileName, secondCommand) == 0) && (de_list[i].block_id != ptr->lba_rtdir)){
			if(de_list[i].entry_type == 0) {

				printf("copy_to_linux: Error. Copying entire folders isn't currently supported.\n");
				found = 1;
				break;
			}

			struct stat st;
			int exists = stat(secondCommand, &st);
			int size = st.st_size;
			if(exists == 0) {
				printf("copy_to_linux: File '%s' ALREADY exists. The Daemon Demons decide not to overwrite the file.\n", secondCommand);
				printf("Try again or remove existing file from the outer file system directory.\n");
			} else {

				char * writeBuffer = malloc(ptr->block_size * de_list[i].total_blocks_allocated);
				LBAread(writeBuffer, de_list[i].total_blocks_allocated, de_list[i].block_position + ptr->lba_data_offset);

				int fd = open (secondCommand, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
				write(fd, writeBuffer, de_list[i].file_size);
				close(fd);
				free(writeBuffer);
				writeBuffer = NULL;
			}
			found = 1;
			break;
		}
	}
	if(found == 0) printf("copy_to_linux: Error. Specified file '%s' not found in current working directory.\n", secondCommand);
	if(found == 1) printf("copy_to_linux: '%s' successfully copied into the linux file system.\n", secondCommand);
	free(de_list);
	de_list = NULL;

	free(frsp_ptr);
	frsp_ptr = NULL;
}

void copy_to_system(myVCB_ptr ptr, char * secondCommand){
	struct stat st;
	int exists = stat(secondCommand, &st);
	int size = st.st_size;
	
	if(exists == 0) {

		int de_position = create_directory_entry(ptr, secondCommand, size, 1, 0);
		//return -1 = Too many directory entries for volume.
		//return -2 = Sufficient Space not available.
		//return -3 = Filename exists in current working directory.
		if(de_position == -1) printf("copy_to_system: Error. Volume contains too many directory entries.\n");
		if(de_position == -2) printf("copy_to_system: Error. Not enough sufficient space available to copy file.\n");
		if(de_position == -3) printf("copy_to_system: Error. Filename already exists in the current working directory.\n");
		if(de_position > 0) {
			freespace_ptr frsp_ptr;
			frsp_ptr = malloc(ptr->lba_frsp_blocks * ptr->block_size);
			LBAread(frsp_ptr, ptr->lba_frsp_blocks, ptr->lba_frsp);

			directoryEntry * de_list;
			de_list = malloc(ptr->lba_dirnodes_blocks * ptr->block_size);
			LBAread(de_list, ptr->lba_dirnodes_blocks, ptr->lba_dirnodes);

			de_list[de_position].file_size = size;
			char * writeBuffer = malloc(ptr->block_size * de_list[de_position].total_blocks_allocated);
			
			int fd = open (secondCommand, O_RDONLY);
			read(fd, writeBuffer, size);
			LBAwrite(writeBuffer, de_list[de_position].total_blocks_allocated, de_list[de_position].block_position + ptr->lba_data_offset);

			close(fd);
			free(writeBuffer);
			writeBuffer = NULL;

			free(de_list);
			de_list = NULL;

			free(frsp_ptr);
			frsp_ptr = NULL;

			printf("copy_to_system: '%s' successfully copied into the current working directory.\n", secondCommand);
		}
	} else {
		printf("copy_to_system: File '%s' does not exist, or is not located within the Daemon Demon File System.\n", secondCommand);
		printf("copy_to_system: Please place the file you wish to copy inside the same folder as the shell (fsshell).\n");
	}
	
}