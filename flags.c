#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "structs.h"
#include "functions.h"



/*Create the .di file and all of its entities*/
void compose(functions_info *args)
{
	char cwd[100];
	int  i, blocks;

	/*Open a new .di file (bin) and write the header information initially*/
	if ((args->fp1 = fopen(args->archive_file, "wb+")) == NULL) print_error("Open .di file");
	blocks = sizeof(header_node) / args->BLOCKSIZE;
	if ((sizeof(header_node) % args->BLOCKSIZE) != 0) blocks++;
	fwrite(args->header, blocks * args->BLOCKSIZE, 1, args->fp1);

	/*The first position of dinode list is about the current working directory*/
	if (getcwd(cwd, sizeof(cwd)) == NULL) print_error("getcwd() error");
	args->home_directory = 1;
	strcpy(args->openfile, cwd);
	strcpy(args->originalfile, cwd);
	file_directory_info(args);
	args->home_directory = 0;

	/*Store all the files and create the list of metadata given in command line at the .di file*/
	for (i = 0; i < args->cmd_list_size; i++)
	{
		args->inside_directory  = 0;
		args->cmd_list_position = i;
		file_directory_info(args);
	}
	insert_metadata(args);					//Add the metadata list in the .di file (contains all files and directories)
	header_modification(args);				//Modify the .di file header properly
}




/*Extract the files from .di file in the proper hierarchy*/
void extract(functions_info *args, int count)
{
	int   i, blocks, offset;
	char  openfile[100], dir_now[100];
	pid_t pid;
	FILE  *fp2;
	dilist_node current_node;

	/*Calculate how many blocks a dinode_list node requires*/
	blocks = sizeof(dilist_node) / args->BLOCKSIZE;
	if ((sizeof(dilist_node) % args->BLOCKSIZE) != 0) blocks++;

	/*Read the metadata from the proper point of .di file*/
	offset = args->header->metadata_position + (count * blocks * args->BLOCKSIZE);
	fseek(args->fp1, offset, SEEK_SET);
	fread(&current_node, sizeof(dilist_node), 1, args->fp1);

	/*Check if this node of metadata points at a file (=0) or directory (=1)*/
	if (current_node.is_directory == 0)
	{
		strcpy(openfile, current_node.name);
		if (current_node.compress == 1)	strcat(openfile, ".gz");

		/*Open a new binary file of same size to write the data from the stored file*/
		if ((fp2 = fopen(openfile, "wb+")) == NULL) print_error("Open binary file");
		fseek(args->fp1, current_node.pointer, SEEK_SET);
		unsigned char buff[current_node.size];
		fread(buff, sizeof(buff), 1, args->fp1);
		fwrite(buff, sizeof(buff), 1, fp2);
		fclose(fp2);

		if (current_node.compress == 1)				//If the file is compressed, decompress first
		{
			if ((pid = fork()) < 0) print_error("fork");
			if (pid == 0) execlp("gunzip", "gunzip", openfile, NULL);
			wait(NULL);
		}
	}
	else {					//This metadata node points to directory
		if (count == 0)
		{											//If ithis is the first node, create the directory in which we will extract the files
			strcpy(dir_now, args->archive_file);
			strcat(dir_now, "-extract");
		}
		else strcpy(dir_now, current_node.name);

		mkdir(dir_now, S_IRWXU);					//Create the directory and move inside it
		chdir(dir_now);
		for (i = 0; i < current_node.dinode.num_of_contents; i++)
		{
			count = current_node.dinode.contents[i].id;		//For each file or directory of current dinode call extract recursively
			extract(args, count);
		}
		chdir("..");		//After all of its entities have been extracted, return to the previous directory (for proper hierarchy)
	}
}




/*Print the metadata information, possibly along with header information and directories' dinodes contents*/
void print_metadata(functions_info *args)
{
	int  i, j, blocks, offset;
	dilist_node current_node;
	header_node header_back;

	/*Calculate how many blocks a dinode_list node requires*/
	blocks = sizeof(dilist_node) / args->BLOCKSIZE;
	if ((sizeof(dilist_node) % args->BLOCKSIZE) != 0) blocks++;

	for (i = 0; i < args->header->size_of_dinode_list; i++)
	{
		/*Read the metadata from the proper point of .di file*/
		offset = args->header->metadata_position + (i * blocks * args->BLOCKSIZE);
		fseek(args->fp1, offset, SEEK_SET);
		fread(&current_node, sizeof(dilist_node), 1, args->fp1);

		/*Print the directory (or file) information*/
		printf(" %s %4d  %d/%d %7d  Access:%s  Modify:%s  %-7s ", current_node.perms, current_node.hard_links, current_node.user_id, 
			current_node.group_id, current_node.size, current_node.access, current_node.modify, current_node.name);
		if (current_node.compress == 1) printf("(compressed)\n");
		else printf("\n");

		if (current_node.is_directory == 1 && args->print_dinodes == 1)		//Possible print of dinodes contents for directories ("-f")
		{
			printf("Dinode Contents:\n 	    .    %d\n 	   ..    %d\n", current_node.dinode.current_id, current_node.dinode.parent_id);
			for (j = 0; j < current_node.dinode.num_of_contents; j++)
				printf("	%7s  %d\n", current_node.dinode.contents[j].name, current_node.dinode.contents[j].id);
		}
	}
	rewind(args->fp1);

	if (args->print_header_info == 1)			//Possible print of .di header information ("-h")
	{
		fread(&header_back, sizeof(header_node), 1, args->fp1);
		printf("block size=%d\nfile size=%d\nheader size=%d\ndinodelistnode size=%d\ndinode_list size=%d\nmetadata_position=%d\n", 
			header_back.size_of_block, header_back.size_of_file, header_back.size_of_header, header_back.size_of_dilist_node, 
			header_back.size_of_dinode_list, header_back.metadata_position);
	}
}




/*Search for some files or directories in .di file and show if found or not*/
void question(functions_info *args)
{
	int i, j, offset, blocks, found = 0;
	dilist_node current_node;

	/*Calculate how many blocks a dinode_list node requires*/
	blocks = sizeof(dilist_node) / args->BLOCKSIZE;
	if ((sizeof(dilist_node) % args->BLOCKSIZE) != 0) blocks++;

	for (i = 0; i < args->cmd_list_size; i++)		//Repeat this procedure for every file/directory asked to be found from cmd
	{
		for (j = 0; j < args->header->size_of_dinode_list; j++)
		{
			/*Read the metadata from the proper point of .di file*/
			offset = args->header->metadata_position + (j * blocks * args->BLOCKSIZE);
			fseek(args->fp1, offset, SEEK_SET);
			fread(&current_node, sizeof(dilist_node), 1, args->fp1);
			if (strcmp(current_node.name, args->list_of_files_dirs[i]) == 0)
			{
				found = 1;		//If we find the asked file/directory, terminate the search
				break;
			}
		}
		rewind(args->fp1);

		/*Print the results*/
		if (found == 1) printf("%8s: Yes\n", args->list_of_files_dirs[i]);
		else printf("%8s: No\n", args->list_of_files_dirs[i]);
		found = 0;
	}

	/*Free the dynamically allocated names of files/directories given in command line*/
	for (i = 0; i < args->cmd_list_size; i++)
		free(args->list_of_files_dirs[i]);
	free(args->list_of_files_dirs);
}




/*Print the hierarchy of file system stored in .di file*/
void print_hierarchy(functions_info *args, int *count, int *tabs)
{
	int i, blocks, offset, first;
	dilist_node current_node;

	if (*count == 0)		//For the first print_hierarchy() called, allocate an array which keeps the depth of each file or directory
	{
		tabs = malloc(args->header->size_of_dinode_list * (sizeof(int)));
		for (i = 0; i < args->header->size_of_dinode_list; i++)
			tabs[i] = 0;											//Initialize the tabs array
		first = 1;
	}
	else first = 0;

	/*Calculate how many blocks a dinode_list node requires*/
	blocks = sizeof(dilist_node) / args->BLOCKSIZE;
	if ((sizeof(dilist_node) % args->BLOCKSIZE) != 0) blocks++;

	do {
		/*Read the metadata from the proper point of .di file*/
		offset = args->header->metadata_position + (*count * blocks * args->BLOCKSIZE);
		fseek(args->fp1, offset, SEEK_SET);
		fread(&current_node, sizeof(dilist_node), 1, args->fp1);
		(*count)++;

		for (i = 0; i < tabs[current_node.dinode.parent_id]; i++)	//Print as many tabs as the depth of file/directory
			printf("   ");
		printf("%s\n", current_node.name);

		if (current_node.is_directory == 1)			//If this is a directory, move inside it by calling the print_hierarchy() recursively
		{
			tabs[current_node.list_position] = tabs[current_node.dinode.parent_id] + 1;		//But first adjust the depth
			print_hierarchy(args, count, tabs);
		}
	} while(*count < args->header->size_of_dinode_list);	//Repeat the procedure for every metadata node

	if (first == 1) free(tabs);				//Free the dynamically allocated tabs array
}