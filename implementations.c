#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "structs.h"
#include "functions.h"



/*Read the parameters and arguments from command line and create the basic variables and structures needed*/
functions_info* cmd_arguments(int argc, char const *argv[], int flags[], int const BLOCKSIZE)
{
	int  i, j, length, cmd_list_size, found_params = 0, compress = 0;
	char archive_file[100], **list_of_files_dirs;
	functions_info *args;

	for (i = 0; i < 10; i++)		//Initialize the flags array (keeps the kind of parameters given)
		flags[i] = 0;

	for (i = 0; i < argc; i++)		//Read every argument from command line
	{
		if (argc == 1) print_error("No parameters are given");	//If there is the name of the program exit

		if (strcmp(argv[i], "-c") == 0) flags[0] = 1;			//Compose
		else if (strcmp(argv[i], "-x") == 0) flags[2] = 1;		//Extract
		else if (strcmp(argv[i], "-j") == 0) flags[3] = 1;		//Compress
		else if (strcmp(argv[i], "-m") == 0) flags[5] = 1;		//Print metadata
		else if (strcmp(argv[i], "-q") == 0) flags[6] = 1;		//Question - search for files/directories
		else if (strcmp(argv[i], "-p") == 0) flags[7] = 1;		//Print hierarchy
		else if (strcmp(argv[i], "-f") == 0) flags[8] = 1;		//Print dinodes contents for directories (extra)
		else if (strcmp(argv[i], "-h") == 0) flags[9] = 1;		//Print header information (extra)
		else if (i != 0)
		{
			for (j = 0; j < 10; j++)
				if (flags[j] == 1) found_params = 1;			//Look if any parameter has been given

			if (found_params == 0) print_error("No parameters are given");		//If none, exit

			strcpy(archive_file, argv[i]);						//Keep the name of .di file (archive file)
			if (flags[0] || flags[1] || flags[6])
			{													//For compose and question read <list-of-files/dirs>
				cmd_list_size = argc - i - 1;
				list_of_files_dirs = malloc(cmd_list_size * sizeof(char*));		//Allocate the names
				for (j = 0; j < cmd_list_size; j++)
				{
					length = strlen(argv[++i]);
					list_of_files_dirs[j] = malloc((length + 1) * sizeof(char));
					strcpy(list_of_files_dirs[j], argv[i]);
				}
			}
		}
	}

	if (flags[3] == 1) {								//Check for proper compress declaration
		if ((flags[0] == 0) && (flags[1] == 0))
			print_error("Compress can be implemented only with -c or -a arguments");
		else compress = 1;
	}
	if (flags[8] == 1 || flags[9] == 1) {				//Header and dinode print only along with metadata print
		if (flags[5] == 0)
			print_error("Dinodes' and Header's information can be printed only with -m argument");
	}

	/*Create the arguments and structures needed for every function*/
	if (flags[0] == 1 || flags[1] == 1)
		args = arguments_info_creation(archive_file, list_of_files_dirs, cmd_list_size, compress, BLOCKSIZE, 0);
	else args = arguments_info_creation(archive_file, list_of_files_dirs, cmd_list_size, compress, BLOCKSIZE, 1);

	if (flags[8] == 1) args->print_dinodes = 1;			//Dinodes will be printed
	if (flags[9] == 1) args->print_header_info = 1;		//Header information will be printed

	return args;
}




/*Store files and directories in archive file and create metadata*/
void file_directory_info(functions_info *args)
{
	struct stat statbuf, helpbuf;
	struct tm lt;
	pid_t  pid;
	int    i, j, compress;
	char   *modes[] = {"---","--x","-w-","-wx","r--","r-x","rw-","rwx"};
	char   zipfile[100], originalfile[100], perms[15];
	dilist_node *current_node;

	current_node = dilist_node_import(args);		//Import a new node at the end of dinode list structure

	if (args->home_directory != 1)					//Not the first node (home directory)
	{
		args->current_position++;
		if (args->inside_directory == 0)			//First level of hierarchy, cmd files and directories
		{
			strcpy(args->originalfile, args->list_of_files_dirs[args->cmd_list_position]);
			strcpy(args->openfile, args->list_of_files_dirs[args->cmd_list_position]);
		}
	}
	current_node->list_position = args->current_position;		//Keep the position of file in metadata

	strcpy(originalfile, args->originalfile);
	if (stat(args->openfile, &statbuf) == -1) print_error("Failed to get file status");		//File status

	if ((statbuf.st_mode & S_IFMT) == S_IFREG)		//This is a file
	{
		if (args->compress == 1)					//If we add compress option ("-j"), first compress and then store the file
		{
			if ((pid = fork()) < 0) print_error("fork");
			if (pid == 0) execlp("gzip", "gzip", "-k", args->originalfile, NULL);
			wait(NULL);

			strcpy(zipfile, args->originalfile);
			strcat(zipfile, ".gz");
			strcpy(args->openfile, zipfile);
			if (stat(args->openfile, &helpbuf) == -1) print_error("Failed to get file status");
			current_node->size = helpbuf.st_size;
		}

		compress = handle_file(args, current_node, statbuf);		//Handle the file in dinode list and store it
		strcpy(perms, "-");
	}
	else if ((statbuf.st_mode & S_IFMT) == S_IFDIR)		//This is a directory
	{
		compress = handle_directory(args, current_node, statbuf);	//Handle the directory in dinode list and move into it
		strcpy(perms, "d");
	}
	else strcpy(perms, "?");

	/*Keep file/directory data int dinode list*/
	if (compress != 1) current_node->size = statbuf.st_size;
	current_node->hard_links = statbuf.st_nlink;
	current_node->user_id  	 = statbuf.st_uid;
	current_node->group_id	 = statbuf.st_gid;
	current_node->compress 	 = compress;
	current_node->pointer	 = args->pointer;

	for (i = 2; i >= 0; i--)
	{
   		j = (statbuf.st_mode >> (i*3)) & 07;		//Find and keep the file/directory permissions
	  	strcat(perms,modes[j]); 
	}
	strcpy(current_node->perms, perms);

	strcpy(current_node->name, originalfile);		//Keep the original file/directory name

	localtime_r(&statbuf.st_atime, &lt);
   	strftime(current_node->access, sizeof(current_node->access), "%c", &lt);	//Keep access time information
	localtime_r(&statbuf.st_mtime, &lt);
   	strftime(current_node->modify, sizeof(current_node->modify), "%c", &lt);	//Keep modify time information
}




/*Handle the files*/
int handle_file(functions_info *args, dilist_node *current_node, struct stat statbuf)
{
	int  blocks, compress;
	FILE *fp2;

	/*Open the file and copy its binary content in buff array*/
	if ((fp2 = fopen(args->openfile, "rb")) == NULL) print_error("Open bin file");
	unsigned char buff[statbuf.st_size];
	fread(buff, sizeof(buff), 1, fp2);
	fclose(fp2);

	/*Write the file's content in the proper position of .di file*/
	blocks = sizeof(buff) / args->BLOCKSIZE;
	if ((sizeof(buff) % args->BLOCKSIZE) != 0) blocks++;
	args->pointer = ftell(args->fp1);
	fwrite(buff, blocks * args->BLOCKSIZE, 1, args->fp1);
	if (args->compress == 1) remove(args->openfile);			//Remove the possible created compressed file

	/*Update the dinodes information in dinode list*/
	dinode_update(args, current_node, ".", args->originalfile, args->current_position);

	/*Keep some data of file for future use*/
	current_node->is_directory = 0;
	current_node->inode		   = statbuf.st_ino;
	compress                   = args->compress;
	return compress;
}




/*Handle the directories*/
int handle_directory(functions_info *args, dilist_node *current_node, struct stat statbuf)
{
	struct dirent *in_file;
	char   originalfile[100];
	int    i, j, compress, cur_pos;
	DIR    *dir_ptr;

	current_node->dinode.num_of_contents = 0;
	current_node->inode		  		     = statbuf.st_ino;

	if (args->current_position == 0)				//This is the first file (home directory)
	{
		current_node->dinode.current_id = 0;
		current_node->dinode.parent_id  = 0;

		for (j = 0; j < args->cmd_list_size; j++)	//Keep its dinode information
		{
			current_node->dinode.num_of_contents++;
			current_node->dinode.contents[j].id = 0;
			strcpy(current_node->dinode.contents[j].name, args->list_of_files_dirs[j]);
		}
	}
	else {				//This is an inner directory in file system
		i = 0;
		cur_pos = args->current_position;						//Keep the current position for future use (after the ed of recursion)
		current_node->dinode.num_of_contents = 0;
		current_node->dinode.current_id 	 = args->current_position;
		strcpy(originalfile, args->openfile);

		/*Open the directory and and look for its contents*/
		if ((dir_ptr = opendir(args->openfile)) == NULL) print_error("Cannot open directory");
		chdir(args->openfile);
		while ((in_file = readdir(dir_ptr)) != NULL)
		{
			if (strcmp(in_file->d_name, ".")  == 0) continue;
       		if (strcmp(in_file->d_name, "..") == 0)						//Update all relative dinodes information
       		{
       			dinode_update(args, current_node, in_file->d_name, originalfile, cur_pos);
       			continue;
       		}

       		/*Keep the number and the name of directory's contents in its dinode*/
			current_node->dinode.num_of_contents++;
			strcpy(current_node->dinode.contents[i].name, in_file->d_name);
			i++;

			args->inside_directory = 1;
			strcpy(args->originalfile, in_file->d_name);
			strcpy(args->openfile, in_file->d_name);
			file_directory_info(args);						//Call file_directory_info() recursively for current directory's files/dirs
		}
		chdir("..");				//Return to the previous directory after ending the read of all the files beneath current hierarchy
		closedir(dir_ptr);			//Close working directory
	}

	/*Keep some data of directory for future use*/
	current_node->is_directory    = 1;
	args->pointer 			      = -1;
	compress  				      = -1;
	return compress;
}




/*Insert the metadata (dinode list structure) in .di file*/
void insert_metadata(functions_info *args)
{
	int i, blocks = 0;
	dilist_node *current_node;

	for (i = 0; i < args->dinode_list_info->size; i++)					//Insert every node of dinode list
	{
		blocks = sizeof(dilist_node) / args->BLOCKSIZE;					//Calculate how many blocks a dinode_list node requires
		if ((sizeof(dilist_node) % args->BLOCKSIZE) != 0) blocks++;
		if (i == 0)
		{																//Find the position in .di file to write the metadata
			current_node = args->dinode_list_info->first;
			args->header->metadata_position = ftell(args->fp1);
		}
		fwrite(current_node, blocks * args->BLOCKSIZE, 1, args->fp1);	//Write the metadata
		current_node = current_node->next;								//Move to the next node of dinode list structure
	}
	args->header->size_of_dinode_list = args->dinode_list_info->size;	//Update the header information about the size of dinode list
}