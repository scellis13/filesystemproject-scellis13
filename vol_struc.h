#ifndef VOL_STRUC_H
#define VOL_STRUC_H
#define MAGIC_NUMBER 0x92073783 //STUDENT_ID minus last digit

typedef struct freespace {
	int blocks_free;
	int total_directory_entries;
	int lba_history_index;  //Index of the command history
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
	int lba_data_offset;	//Offset to datablocks (Starts at root directory)
	int lba_rtdir; 			//Block ID of root directory
	int lba_curdir;			//Root Directory Block ID
	int lba_history_pos;	//Offset to the position of Command History blocks.
	int lba_history_blocks;	//How many blocks are dedicated to history.
	int history_count;		//Records the number of commands recorded in history.
	char volume_name[200];		//Name of the volume

} myVCB, *myVCB_ptr;

typedef struct directoryEntry {
	int block_id;
	int block_position;
	int total_blocks_allocated;
	int parent_id;
	int file_size;
	int entry_type; //0 for folder, 1 for file
	char fileName[55];

} directoryEntry, *directoryEntry_ptr;

int create_volume(myVCB_ptr ptr, char * filename, int volumeSize, int blockSize);
void init_freespace(myVCB_ptr ptr);
void init_rtdir(myVCB_ptr ptr);
int create_directory_entry(myVCB_ptr ptr, char * file_name, int size_bytes, int entry_type, int init);
int get_setup_blocks(myVCB_ptr ptr, int blocks_requested, int block_size);

#endif