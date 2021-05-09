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
	
 	myVCB_ptr ptr;
 	ptr = malloc(blockSize);
 	LBAread(ptr, 1, 0);


	if(ptr->magic_number != MAGIC_NUMBER){
		create_volume(ptr, filename, volumeSize, blockSize);
	}

	char user_command[100];
	char buffer[100];
	char firstCommand[5];
	char secondCommand[55];

    while(1) {
    	
        printf("user@daemon-demons:~$ ");
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


	        // if(strcasecmp(firstCommand, "mkdir") == 0) { 
	        // 	make_dir(ptr, secondCommand); 
	        // 	continue; 
	        // }
	        
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
    LBAwrite(ptr, ptr->block_size, 0);
	free(ptr);
	ptr = NULL;
	printf("Daemon Demon File System shutting down...\n");
	//closePartitionSystem();
	return 0;
}

void display_help(){

	printf("\nListing Possible Commands:\n");

}