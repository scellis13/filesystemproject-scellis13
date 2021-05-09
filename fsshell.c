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
		// printf ("Usage: fsLowDriver volumeFileName volumeSize blockSize\n");
		return -1;
		}

	if (volumeSize < 5120) {
		// printf("main: Invalid volume size listed. Please choose a size Greater ( > ) than 5120 bytes.");
		return -1;
	}
		
	retval = startPartitionSystem (filename, &volumeSize, &blockSize);	
	// printf("Opened %s, Volume Size: %llu;  BlockSize: %llu; Return %d\n", filename, (ull_t)volumeSize, (ull_t)blockSize, retval);
	
 	myVCB_ptr ptr;
 	ptr = malloc(blockSize);
 	LBAread(ptr, 1, 0);


	if(ptr->magic_number != MAGIC_NUMBER){
		create_volume(ptr, filename, volumeSize, blockSize);
	}

    while(1) {
    	
        printf("user@daemon-demons:~$ ");
        
        char *line = NULL;
        size_t len = 0;
        ssize_t lineSize = 0;

        lineSize = getline(&line, &len, stdin);

        printf("You entered '%s', which has %zu chars.\n", line, lineSize - 1);
        
	 // if(strcmp(token, "pwd") == 0) print_dir(ptr);
     //    if(strcmp(token, "psm") == 0) print_storage_map(ptr, 10);
     //    // if(strcmp(token, "ls") == 0) list_dir(ptr);
     //    // if(strcmp(token, "cd") == 0) change_dir(ptr, dir_name);
     //    // if(strcmp(token, "mkdir") == 0) make_dir(ptr, dir_name);
     //    if(strcmp(token, "-help") == 0) display_menu();
    }

    // printf("\n\n***End of main***\n");
    LBAwrite(ptr, ptr->block_size, 0);
	free(ptr);
	ptr = NULL;
	//closePartitionSystem();
	return 0;
}