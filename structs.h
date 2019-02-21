#ifndef __structs__
#define __structs__


/*Header node*/
typedef struct
{
	int size_of_block;
	int size_of_file;
	int size_of_header;
	int size_of_dilist_node;
	int size_of_dinode;
	int size_of_dinode_list;
	int metadata_position;
}header_node;



/*Dinode structure*/
typedef struct
{
	char name[30];
	int  id;
}dinode_content;

typedef struct
{
	int num_of_contents;
	int current_id;
	int parent_id;
	dinode_content contents[50];
}dinode_struct;



/*Dinode list structure*/
typedef struct dilist_node *ptrdilist;

typedef struct dilist_node
{
	int  list_position;
	int  size;
	int  hard_links;
	int  user_id;
	int  group_id;
	char perms[11];
	char access[30];
	char modify[30];
	char name[100];
	int  is_directory;
	int  compress;
	int  pointer;
	int  inode;
	dinode_struct dinode;
	ptrdilist next;
	ptrdilist prev;
}dilist_node;

typedef struct dilist_info
{
	int size;
	ptrdilist first;
	ptrdilist last;
}dilist_info;



/*Arguments (functions_info) structure*/
typedef struct
{
	int  print_dinodes;
	int  print_header_info;
	int  home_directory;
	int  read_from_file;
	int  inside_directory;
	int  BLOCKSIZE;
	int  list_size;
	int  cmd_list_size;
	int  list_position;
	int  cmd_list_position;
	int  compress;
	int  pointer;
	int  current_position;
	char archive_file[100];
	char openfile[100];
	char originalfile[100];
	char **list_of_files_dirs;
	FILE *fp1;
	dilist_info *dinode_list_info;
	header_node *header;
}functions_info;


#endif