#ifndef VOL_FUNC_H
#define VOL_FUNC_H

#include "vol_struc.h"

void list_dir(myVCB_ptr ptr);
void make_dir(myVCB_ptr ptr, char * dir_name);
void print_dir(myVCB_ptr ptr);
void print_storage_map(myVCB_ptr ptr, int break_point);
void change_dir(myVCB_ptr ptr, char * dir_name);
void print_history(myVCB_ptr ptr);
void write_history(myVCB_ptr ptr, char * command);
void remove_entry(myVCB_ptr ptr, char * file_name);
void print_all(myVCB_ptr ptr);
void copy_to_linux(myVCB_ptr ptr, char * secondCommand);
void copy_to_system(myVCB_ptr ptr, char * secondCommand);

#endif