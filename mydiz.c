#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "functions.h"

int const BLOCKSIZE = 256;											//Size of block - it can be changed if we want to


int main(int argc, char const *argv[])
{
	int count = 0, *tabs = NULL, flags[10];
	functions_info *args;

	args = cmd_arguments(argc, argv, flags, BLOCKSIZE);				//Read program's arguments and create base variables and structures

	if (flags[0] == 1) compose(args);								//"-c", create the .di file
	if (flags[2] == 1) extract(args, 0);							//"-x", extract the .di file contents at "archive_name-extract"
	if (flags[5] == 1) print_metadata(args);						//"-m", print metadata (possible combination with "-h" and "-f")
	if (flags[6] == 1) question(args);								//"-q". search for the files or directories given in cmd into .di file
	if (flags[7] == 1) print_hierarchy(args, &count, tabs);			//"-p", print the hierarchy of .di file

	delete_structs(args);											//Close .di file and delete all dynamically allocated structures

	return 0;
}