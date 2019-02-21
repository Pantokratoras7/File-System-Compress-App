#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "structs.h"
#include "functions.h"


/*Create the variables and structures needed for all the functions*/
functions_info* arguments_info_creation(char *archive_file, char **list_of_files_dirs, int list_size, int compress, 
											int const BLOCKSIZE, int const read)
{
	functions_info *args = malloc(sizeof(functions_info));		//Allocate the arguments dynamically and initialize some members
	args->print_dinodes		 = 0;
	args->print_header_info  = 0;
	args->home_directory     = 0;
	args->inside_directory   = 0;
	args->read_from_file     = 0;
	args->current_position   = 0;
	args->list_position		 = 0;
	args->BLOCKSIZE 		 = BLOCKSIZE;
	args->list_size			 = list_size;
	args->cmd_list_size      = list_size;
	args->compress			 = compress;
	args->list_of_files_dirs = list_of_files_dirs;
	args->header 			 = header_creation(BLOCKSIZE);
	strcpy(args->archive_file, archive_file);

	if (read == 0)													//Write mode ("-c" parameter)
		args->dinode_list_info = dinode_list_info_creation();
	else {															//Read mode (not "-c" parameter)
		args->read_from_file = 1;
		if ((args->fp1 = fopen(archive_file, "rb")) == NULL) print_error("Open .di file");		//Read from .di file
		fread(args->header, sizeof(header_node), 1, args->fp1);
		rewind(args->fp1);
	}
	return args;
}



/*Create header structure dynamically and initialize its members*/
header_node* header_creation(int const BLOCKSIZE)
{
	header_node* header = malloc(sizeof(header_node));
	header->size_of_block       = BLOCKSIZE;
	header->size_of_file 		= BLOCKSIZE;
	header->size_of_header      = sizeof(header_node);
	header->size_of_dilist_node = sizeof(dilist_node);
	header->size_of_dinode 		= sizeof(dinode_struct);
	header->size_of_dinode_list = 0;
	header->metadata_position   = BLOCKSIZE;
	return header;
}



/*Modify header's information and write them into the .di file*/
void header_modification(functions_info *args)
{
	fseek(args->fp1, 0, SEEK_END);
	args->header->size_of_file = ftell(args->fp1);
	rewind(args->fp1);
	fwrite(args->header, args->BLOCKSIZE, 1, args->fp1);
	rewind(args->fp1);
}



/*Allocate and initialize the first info node of dinode list*/
dilist_info* dinode_list_info_creation()
{
	dilist_info* linfo = malloc(sizeof(dilist_info));
	linfo->size  = 0;
	linfo->first = NULL;
	linfo->last  = NULL;
	return linfo;
}



/*Import a new node in the end of dinode list*/
dilist_node* dilist_node_import(functions_info *args)
{
	dilist_node* new_node = malloc(sizeof(dilist_node));
	if (args->dinode_list_info->size != 0)
		args->dinode_list_info->last->next = new_node;
	else
		args->dinode_list_info->first = new_node;
	new_node->prev = args->dinode_list_info->last;
	new_node->next = NULL;
	args->dinode_list_info->last = new_node;
	args->dinode_list_info->size++;
	return new_node;
}



/*Update the dinodes related with the current file or directory*/
void dinode_update(functions_info *args, dilist_node *current_node, char *parentfile, char* originalfile, int current_position)
{
	int i;
	struct stat parentbuf;
	dilist_node *help_node;

	/*Start looking for parent's dinode from the end for speed*/ 
	help_node = args->dinode_list_info->last;
    if (stat(parentfile, &parentbuf) == -1) print_error("Failed to get file status");

	while (help_node != NULL)		//Look until the first one if needed
	{
		if (help_node->inode == parentbuf.st_ino)		//Found the parent directory's dinode
		{
			current_node->dinode.parent_id = help_node->list_position;		//Update current_node's dinode parent_id
			for (i = 0; i < help_node->dinode.num_of_contents; i++)
			{
				if (strcmp(help_node->dinode.contents[i].name, originalfile) == 0)
				{
					help_node->dinode.contents[i].id = current_position;	//Update parent's dinode content points into current file
					break;
				}
			}
			break;
		}
		help_node = help_node->prev;		//Move to the previous node of dinode list
	}
	help_node = NULL;
}



/*Delete every dynamically allocated structure*/
void delete_structs(functions_info *args)
{
	short int i;
	ptrdilist help_dinode;

	fclose(args->fp1);			//Close .di file
	free(args->header);			//Free header structure

	if (args->read_from_file == 0)		//If we were in write mode, free the cmd arguments and the dinode list structure
	{
		for (i = 0; i < args->list_size; i++)
			free(args->list_of_files_dirs[i]);
		free(args->list_of_files_dirs);

		for (i = 0; i < args->dinode_list_info->size; i++)
		{
			help_dinode = args->dinode_list_info->first;
			args->dinode_list_info->first = help_dinode->next;
			free(help_dinode);
		}
		free(args->dinode_list_info);
	}

	free(args);					//Free arguments structure
}


/*Print the errors and exit program*/
void print_error(char const *func)
{
	perror(func);
	exit(-1);
}