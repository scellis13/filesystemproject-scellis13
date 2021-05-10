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
#include "vol_struc.c"
#include "vol_func.c"

void display_help();

int main(int argc, char *argv[]) {

	char * filename;
	uint64_t volumeSize;
	uint64_t blockSize;
    int retval;
    
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

	if (volumeSize < 5120) {
		printf("main: Invalid volume size listed. Please choose a size Greater ( > ) than 5120 bytes.");
		return -1;
	}
		
	retval = startPartitionSystem (filename, &volumeSize, &blockSize);	
	printf("Opened %s, Volume Size: %llu;  BlockSize: %llu; Return %d\n", filename, (ull_t)volumeSize, (ull_t)blockSize, retval);
	printf("\033[31mDaemon Demon File System starting...\n");

 	myVCB_ptr ptr;
 	ptr = malloc(blockSize);
 	LBAread(ptr, 1, 0);


	if(ptr->magic_number != MAGIC_NUMBER){
		create_volume(ptr, filename, volumeSize, blockSize);
	}

	char user_command[100];
	char buffer[100];
	char firstCommand[6];
	char secondCommand[55];

    while(1) {
    	
        printf("\033[1;32muser@daemon-demons");
        printf("\033[0m:");
        printf("\033[1;34m");
        print_dir(ptr);
        printf("\033[0m$ ");
        gets(user_command);
        if(strcasecmp(user_command, "exit") == 0) break;
        memcpy(buffer, user_command, sizeof(buffer));

        const char separator = ' ';
        char * const command = strchr(buffer, separator);

        
    	if(command != NULL){ //Two Argument Commands
        	*command = '\0';
        	if(strlen(buffer) > 5){
	        	printf("%s: Unable to execute command with first argument. Length must be <= 5 characters.\n", buffer);
	        	printf("Enter '-help' for a list of valid commands.\n");
	        	continue;
	        } else {
	        	memcpy(firstCommand, buffer, sizeof(firstCommand));
	        	if(strlen(command+1) > 55) {
	        		printf("%s: Unable to execute command with second argument. Length must be < 55 Characters.\n", firstCommand);
	        		continue;
	        	} else {
	        		memcpy(secondCommand, command+1, sizeof(secondCommand));
	        	}
	        }

	        if(strcasecmp(firstCommand, "mkdir") == 0) { 
	        	make_dir(ptr, secondCommand); 
	        	continue; 
	        }

	        if(strcasecmp(firstCommand, "psm") == 0) { 
	        	//Check that secondCommand is an integer
	        	print_storage_map(ptr, atoll(secondCommand)); 
	        	continue; 
	        }
	        if(strcasecmp(firstCommand, "cd") == 0) { 
	        	change_dir(ptr, secondCommand); 
	        	continue; 
	        }

        } else { //Single Argument Commands

        	if(strlen(buffer) > 5){
        		printf("%s: Unable to execute command with first argument. Length must be <= 5 characters.\n", buffer);
        		continue;
        	} else {
        		memcpy(firstCommand, buffer, sizeof(firstCommand));
        	}

        	if(strcasecmp(firstCommand, "-help") == 0) {
        		display_help();
        		continue;
        	}
        	if(strcasecmp(firstCommand, "ls") == 0) {
        		list_dir(ptr);
        		continue;
        	}
        	if(strcasecmp(firstCommand, "pwd") == 0) {
        		print_dir(ptr);
        		printf("\n");
        		continue;
        	}
        }

	    
	    printf("%s: Invalid command. Enter '-help' for a list of valid commands.\n", user_command);

    }

    // printf("\n\n***End of main***\n");
	free(ptr);
	ptr = NULL;
	printf("\033[31mDaemon Demon File System shutting down...\n");
	//closePartitionSystem();
	return 0;
}

void display_help(){

	printf("\033[0m\n----------[ Single Argument Commands ]----------\n");

	printf("\033[0m\n -help\t\t\t-- \033[32mYou are currently using this command.");
	printf("\033[0m\n ls\t\t\t-- \033[32mLists all files within the current working directory.");
	printf("\033[0m\n pwd\t\t\t-- \033[32mPrints the current working directory path.");

	printf("\033[0m\n\n----------[ Double Argument Commands ]----------\n");

	printf("\033[0m\n mkdir\t%%filename%%\t-- \033[32mCreates folder of name: '%%filename%%' in current working directory.");
	printf("\033[0m\n psm\t%%number%%\t-- \033[32mPrints a map view of all used and free memory blocks. %%number%% specifies the newline breakpoint.");
	printf("\033[0m\n cd\t%%filename%%\t-- \033[32mChanges Directory to specified '%%filename%%'. Only changes directories based on relative path.");
	
	printf("\n");
}