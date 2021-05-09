#ifndef VOL_FUNC_H
#define VOL_FUNC_H

#include "vol_struc.h"

void list_dir(myVCB_ptr ptr);
void make_dir(myVCB_ptr ptr, char * dir_name);
void print_dir(myVCB_ptr ptr);
void print_storage_map(myVCB_ptr ptr, int break_point);
void change_dir(myVCB_ptr ptr, char * dir_name);

#endif