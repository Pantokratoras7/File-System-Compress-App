#ifndef __functions__
#define __functions__

#include <sys/stat.h>
#include "structs.h"


/*Declarations of every function*/
functions_info* arguments_info_creation(char *archive_file, char **list_of_files_dirs, int list_size, int compress, 
											int const BLOCKSIZE, int const read);
functions_info* cmd_arguments(int argc, char const *argv[], int flags[], int const BLOCKSIZE);
header_node* header_creation(int const BLOCKSIZE);
dilist_info* dinode_list_info_creation();
dilist_node* dilist_node_import(functions_info *args);
void header_modification(functions_info *args);
void delete_structs(functions_info *args);
void file_directory_info(functions_info *args);
void insert_metadata(functions_info *args);
void print_error(char const *func);
void compose(functions_info *args);
void extract(functions_info *args, int count);
void print_metadata(functions_info *args);
void question(functions_info *args);
void print_hierarchy(functions_info *args, int *count, int *tabs);
void dinode_update(functions_info *args, dilist_node *help_node, char *parentfile, char* originalfile, int current_position);
int  handle_file(functions_info *args, dilist_node *current_node, struct stat statbuf);
int  handle_directory(functions_info *args, dilist_node *current_node, struct stat statbuf);


#endif