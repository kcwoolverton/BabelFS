/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` fusefat.c -o fusehello

  Modified by: Evan Amason and Sara McAllister
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#endif

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#define name_size 54

static const size_t block_size = 4096;
static const int num_blocks = 256;
static const char *disk_name = "fat_disk";
static char *current_path;
static char *full_path;
static FILE *disk;
static size_t *FAT;

static FILE *asker;
static FILE *answer;

typedef struct {
	unsigned long size;
	size_t first_block;
	char name[name_size];
	char file_type; /* 1 is directory, 0 is file, 2 is symlink */
	char file_check;
} metadata;

typedef struct {
	metadata root_metadata;
	size_t superblock_size;
	size_t block_size;
	char magic_number;
	size_t free_space_start;
} fat_superblock;

union superblock {
	fat_superblock l_superblock;
	char pad[512];
} superblock;

static fat_superblock fs_superblock;
static const size_t metadata_size = sizeof(metadata);
static  size_t max_metadata;

/* Creates a . and .. entry for the new directory */
void dir_initialize(metadata* current_metadata, metadata* parent_metadata) {
	metadata *new_dir = (metadata*) calloc(max_metadata, metadata_size);
	new_dir[0] = *parent_metadata;
	strcpy(new_dir[0].name, "..");

	new_dir[1] = *current_metadata;
	strcpy(new_dir[1].name, ".");

	fseek(disk, current_metadata->first_block * block_size, SEEK_SET);
	fwrite(new_dir, block_size, 1, disk);
}

/*Find next block in free space, remove it and return its location*/
size_t allocate_new_block() {
	size_t new_block_loc;

	new_block_loc = fs_superblock.free_space_start;
	fs_superblock.free_space_start = FAT[new_block_loc];
	FAT[new_block_loc] = 0;
	return new_block_loc;
}

void* fat_init(struct fuse_conn_info *conn)
{
	char ask_command[15];
	char ans_command[15];
	size_t path_len = strlen(disk_name) + strlen(current_path) + 2;
	metadata root_metadata;
	union superblock local_superblock;
	size_t i;
	max_metadata = block_size / metadata_size;

	// Start the python program
	system("python babel_functions.py &");

	// Handles calls to the babel API
	strcpy(ask_command, "mkfifo ask");
	strcpy(ans_command, "mkfifo ans");
	system(ask_command);
	system(ans_command);

	// open the pipes for reading and writing
	asker = fopen("./ask", "w");
	answer = fopen("./ans", "r");

	// Ensure that each request is properly flushed to asker
	setlinebuf(asker);

	/*Create the entire path name*/
	full_path = (char *) malloc(path_len);
	strcpy(full_path, current_path);
	strcat(full_path, "/");
	strcat(full_path, disk_name);

	/* Create the FAT in memory*/
	FAT = (size_t*) malloc(sizeof(size_t) * num_blocks);

	/*Create new file if doesn't exist otherwise open existing file*/
	if (access(full_path, O_RDWR) == -1) {
		disk = fopen(full_path, "w+");
		/* Make file the right size */
		fseek(disk, block_size * num_blocks - 1, SEEK_SET);
		fputc('\0', disk);

		for (i = 0; i < num_blocks; i++) {
			if (i == 0 || i == 1 || i == (num_blocks - 1) ) {
				FAT[i] = 0;
			} else {
				FAT[i] = i + 1;
			}
		}

		root_metadata.size = block_size;
		root_metadata.first_block = 1;
		root_metadata.file_type = 1;
		root_metadata.file_check = 1;

		dir_initialize(&root_metadata, &root_metadata);

		/* The superblock is in the second position, just after
		 * the root. */
		fs_superblock.free_space_start = 2;
		fs_superblock.superblock_size = 512;
		fs_superblock.block_size = block_size;
		fs_superblock.magic_number = 77;
		fs_superblock.root_metadata = root_metadata;
	} else {
		disk = fopen(full_path, "r+");
		fread(&local_superblock, sizeof(superblock), 1, disk);
		fs_superblock = local_superblock.l_superblock;

		fseek(disk, sizeof(superblock), SEEK_SET);
		fread(FAT, sizeof(size_t), num_blocks, disk);
	}

	return NULL;
}

void fat_destroy(void* private_data)
{
	union superblock u_superblock;
	u_superblock.l_superblock = fs_superblock;
	fseek(disk, 0, SEEK_SET);
	fwrite(&u_superblock, sizeof(superblock), 1, disk);

	fseek(disk, sizeof(superblock), SEEK_SET);
	fwrite(FAT, sizeof(size_t), num_blocks, disk);

	// send shut down signal to python
	fwrite("?", 1, 1, asker);

	fclose(answer);
	fclose(asker);
	fclose(disk);
	free(full_path);
	free(current_path);
	free(FAT);
}

/*
 * Return file metadata for the inputted path, metadata has a
 * file_check value of 0 if file could not be found
 * */
void find_metadata(const char *path, metadata* file_metadata) {
	char* path_copy;
	char* token;
	size_t block_num = 1;
	size_t i;

	*file_metadata = fs_superblock.root_metadata;
	path_copy = (char*) malloc(strlen(path) + 1);
	strcpy(path_copy, path);
	token = strtok(path_copy, "/");

	metadata* current_metadata = (metadata*) malloc(sizeof(metadata) *
		max_metadata);


	while (token) {
		fseek(disk, block_size * block_num, SEEK_SET);
		fread(current_metadata, sizeof(metadata), max_metadata, disk);
		for (i = 0; i < max_metadata; i++) {
			*file_metadata = current_metadata[i];
			if (file_metadata->file_check &&
				!strcmp(file_metadata->name, token)) {
				break;
			} else if (i == max_metadata - 1) {
				/*If file does not exist, return invalid metadata*/
				file_metadata->file_check = 0;
				free(current_metadata);
				free(path_copy);
				return;
			}
		}

		block_num = file_metadata->first_block;
		token = strtok(NULL, "/");
	}

	free(path_copy);
	free(current_metadata);
}

static int fat_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	metadata file_metadata;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		find_metadata(path, &file_metadata);
		if (file_metadata.file_check == 0) {
			return -ENOENT;
		}
		if (file_metadata.file_type == 1) {
			stbuf->st_mode = S_IFDIR | 0755;
			stbuf->st_nlink = 2;
		} else if (file_metadata.file_type == 2) {
			stbuf->st_mode = S_IFLNK | 0755;
			stbuf->st_nlink = 1;
		} else {
			stbuf->st_mode = S_IFREG | 0755;
			stbuf->st_nlink = 1;
		}
		stbuf->st_size = file_metadata.size;
	}
	return res;
}

static int fat_access(const char *path, int mask)
{
	metadata file_metadata;
	find_metadata(path, &file_metadata);
	if (file_metadata.file_check == 0) {
		return -ENOENT;
	}
	return 0;
}

static int fat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	metadata dir_metadata;
	metadata current_metadata;
	metadata* current_data;
	int ret = 0;
	(void) fi;
	size_t i;

	find_metadata(path, &dir_metadata);

	if (dir_metadata.file_check == 0) {
		return -ENOENT;
	} else if (dir_metadata.file_type != 1) {
		return -EBADF;
	}

	current_data = (metadata*) malloc(max_metadata * sizeof(metadata));
	fseek(disk, block_size * dir_metadata.first_block, SEEK_SET);
	fread(current_data, sizeof(metadata), max_metadata, disk);
	for (i = 0; i < max_metadata; i++) {
		current_metadata = current_data[i];
		if (current_metadata.file_check) {
			ret = filler(buf, current_metadata.name, NULL, 0);
		}
		if (ret != 0) {
			break;
		}
	}
	free(current_data);
	return 0;
}
/* When given the path for a new file, this function will find the path to the
 * new file and the parent directory of that file. */
int find_parent_dir(const char *path, char *parent_path, char *file_name)
{
	size_t path_len = strlen(path);
	char* start_of_name;

	start_of_name = strrchr(path, '/') + 1;

	/* We're making sure that we don't overflow our parent_path. */
	if ((start_of_name - path + 1) > path_len) {
		return -1;
	}

	strncpy(file_name, start_of_name, name_size);
	strncpy(parent_path, path, start_of_name - path);
	parent_path[start_of_name - path] = '\0';

	/* Make sure we don't overflow our file_name. */
	if ((path_len + start_of_name - path)  > (name_size - 1)) {
		return -1;
	}

	strcpy(file_name, start_of_name);

	parent_path[start_of_name - path] = '\0';

	return 0;
}

/* Zero out an block at block_num */
void zero_out_block(size_t block_num)
{
	char data_block[block_size];
	memset(data_block, '0', block_size);
	fseek(disk, block_num * block_size, SEEK_SET);
	fwrite(data_block, block_size, 1, disk);
}


/* Remove free blocks of everything under given metadata in tree */
int free_metadata(metadata* current_metadata) {
	metadata next_metadata;
	size_t current_block = current_metadata->first_block;
	size_t next_block;
	metadata* current_data;
	int check;
	size_t i;

	if (current_metadata->file_type == 1) {
		current_data = (metadata*) malloc(sizeof(metadata) * max_metadata);
		/* Loop through metadata in file, calling this function recursively
		 * on any files within the the directory being deleted */
		while(1) {
			fseek(disk, current_block * block_size, SEEK_SET);
			fread(current_data, sizeof(metadata), max_metadata, disk);
			for (i = 2; i < max_metadata; i++) {
				if (current_data[i].file_check == 1) {
					next_metadata = current_data[i];
					check = free_metadata(&next_metadata);
					if (check != 0) {
						free(current_data);
						return check;
					}
				}
			}

			/* Add current block to free list and go through the next block
			 * of directory if it exists */
			next_block = FAT[current_block];
			FAT[current_block] = fs_superblock.free_space_start;
			fs_superblock.free_space_start = FAT[current_block];
			current_block = next_block;

			if (current_block == 0) {
				break;
			}
		}
		free(current_data);
		return 0;
	} else {
		while(FAT[current_block] != 0) {
			next_block = FAT[current_block];
			FAT[current_block] = fs_superblock.free_space_start;
			fs_superblock.free_space_start = FAT[current_block];
			current_block = next_block;
		}
		return 0;
	}

	return 0;
}

/*
 * Change valid bit for given path name, if free_blocks is non-zero,
 * will add blocks to free list
 */
int remove_path(const char *path, size_t free_blocks){
	size_t path_len = strlen(path);
	char name[name_size];
	char *parent_path;
	int check;
	size_t current_block;
	metadata *current_data;
	metadata parent_metadata;
	metadata start_of_delete;
	size_t i;
	size_t j;

	/* Split up path names */
	parent_path = malloc(path_len);
	check = find_parent_dir(path, parent_path, name);
	if (check == -1) {
		return -ENAMETOOLONG;
	}

	find_metadata(parent_path, &parent_metadata);
	current_data = (metadata*) malloc(sizeof(metadata) * max_metadata);

	/* Set file_check bit of metadata to 0 */
	current_block = parent_metadata.first_block;
	for (j = num_blocks; j > 0; j--) {
		fseek(disk, current_block * block_size, SEEK_SET);
		fread(current_data, sizeof(metadata), max_metadata, disk);
		for (i = 0; i < max_metadata; i++) {
			if (current_data[i].file_check == 1 &&
					!strncmp(current_data[i].name, name, name_size)) {
				current_data[i].file_check = 0;
				start_of_delete = current_data[i];
				fseek(disk, current_block * block_size, SEEK_SET);
				fwrite(current_data, sizeof(metadata), max_metadata, disk);
				free(current_data);
				if (free_blocks) {
					free_metadata(&start_of_delete);
				}
				return 0;
			}
		}

		if (FAT[current_block] != 0) {
			current_block = FAT[current_block];
		} else {
			break;
		}

	}
	free(current_data);
	return -1;
}

/* Write metadata to specific block,
 * rieturn 0 on success, -ENOMEM if no more space*/
int write_metadata_to_block(size_t block_num, metadata *file_metadata) {
	metadata* current_data;
	size_t i;
	size_t j;
	size_t new_block;
	size_t current_block;
	int ret = 0;

	current_data = (metadata*) malloc(sizeof(metadata) * max_metadata);

	/* Find place to put new dir in previous dir's data */
	current_block = block_num;
	for (j = num_blocks; j > 0; j--) {
		fseek(disk, current_block * block_size, SEEK_SET);
		fread(current_data, sizeof(metadata), max_metadata, disk);
		for (i = 0; i < max_metadata; i++) {
			if (current_data[i].file_check == 0) {
				printf("In if statement of write metadata.\n");
				ret++;
				current_data[i] = *file_metadata;
				printf("File_metadata name %s\n", current_data[i].name);
				printf("Flag for metadata %u\n", current_data[i].file_check);
				fseek(disk, current_block * block_size, SEEK_SET);
				fwrite(current_data, sizeof(metadata), max_metadata, disk);
				free(current_data);
				return ret;
			}
		}

		if (FAT[current_block] == 0) {
			new_block = allocate_new_block();

			if (new_block > num_blocks) {
				return -1;
			}

			FAT[current_block] = new_block;
			zero_out_block(new_block);
			current_block = new_block;
		} else {
			current_block = FAT[current_block];
		}

	}

	free(current_data);
	return -1;
}

static int fat_mknod(const char *path, mode_t mode, dev_t rdev)
{
	size_t block_num;
	size_t path_len = strlen(path);
	char name[name_size];
	char *parent_path;
	int check;
	metadata new_metadata;
	metadata parent_metadata;

	parent_path = malloc(path_len);
	check = find_parent_dir(path, parent_path, name);

	printf("The parent path is %s, name is %s.\n", parent_path, name);
	if (check == -1) {
		return -ENAMETOOLONG;
	}

	block_num = allocate_new_block();
	printf("block_num is %u.\n", block_num);
	FAT[block_num] = 0;

	new_metadata.size = 0;
	new_metadata.first_block = block_num;
	memcpy(new_metadata.name, name, name_size);
	new_metadata.file_type = 0;
	new_metadata.file_check = 1;

	find_metadata(parent_path, &parent_metadata);

	printf("Before parent_metadata.first_block\n");
	check = write_metadata_to_block(parent_metadata.first_block, &new_metadata);

	if (check == -1) {
		return -ENOMEM;
	}

	free(parent_path);
	return 0;
}


static int fat_mkdir(const char *path, mode_t mode)
{
	metadata new_dir;
	metadata prev_dir;
	char name[name_size];
	char* parent_path;
	size_t path_size = strlen(path);
	int check;

	/* find name of dir and path to dir */
	parent_path = (char*) malloc(path_size);
	check = find_parent_dir(path, parent_path, name);

	if (check == -1) {
		return -ENAMETOOLONG;
	}

	/* Find the metadata of the parent directory. */
	find_metadata(parent_path, &prev_dir);
	if (prev_dir.file_check == 0) {
		return -ENOENT;
	}

	/* Define metadata for new file */
	new_dir.size = block_size;
	strncpy(new_dir.name, name, sizeof(name));
	new_dir.file_type = 1;
	new_dir.file_check = 1;
	new_dir.first_block = allocate_new_block();

	dir_initialize(&new_dir, &prev_dir);

	/* Write the new metadata to the parent directory */
	check = write_metadata_to_block(prev_dir.first_block, &new_dir);

	free(parent_path);

	return 0;
}

static int fat_unlink(const char *path)
{
	int check;
	check = remove_path(path, 1);
	if (check != 0) {
		return -ENOENT;
	}

	return 0;
}

static int fat_rmdir(const char *path)
{
	int check;
	check = remove_path(path, 1);
	if (check != 0) {
		return -ENOENT;
	}

	return 0;
}

static int fat_symlink(const char *to, const char *from)
{
	size_t from_len = strlen(from);
	char from_name[name_size];
	char *from_path;
	int check;
	metadata from_metadata;
	metadata from_parent_metadata;
	char from_block[block_size];

	/* Split up path names */
	from_path = malloc(from_len);
	check = find_parent_dir(from, from_path, from_name);
	if (check == -1) {
		return -ENAMETOOLONG;
	}

	find_metadata(from_path, &from_parent_metadata);
	from_metadata.file_check = 1;
	from_metadata.file_type = 2;
	from_metadata.first_block = allocate_new_block();
	from_metadata.size = strlen(to);
	memcpy(from_metadata.name, from_name, name_size);

	write_metadata_to_block(from_parent_metadata.first_block, &from_metadata);

	memcpy(from_block, to, from_metadata.size);
	fseek(disk, block_size * from_metadata.first_block, SEEK_SET);
	fwrite(from_block, block_size, 1, disk);

	return 0;
}

static int fat_rename(const char *from, const char *to)
{
	fprintf(stderr, "fat_rename was called.");
	return -ENOSYS;
}

static int fat_link(const char *from, const char *to)
{
	fprintf(stderr, "fat_link was called.");
	return -ENOSYS;
}

static int fat_chmod(const char *path, mode_t mode)
{
	fprintf(stderr, "fat_chmod was called.");
	return -ENOSYS;
}

static int fat_chown(const char *path, uid_t uid, gid_t gid)
{
	fprintf(stderr, "fat_chown was called.");
	return -ENOSYS;
}

static int fat_truncate(const char *path, off_t size)
{
	char name[name_size];
	char* parent_path;
	size_t path_size = strlen(path);
	int check;
	metadata parent_dir;
	metadata *current_data;
	size_t current_block;
	size_t old_size;
	size_t num_blocks_needed;
	size_t next_block;
	size_t i;
	size_t j;

	printf("Inside Truncate\n");

	/* find name of dir and path to dir */
	parent_path = (char*) malloc(path_size);
	check = find_parent_dir(path, parent_path, name);

	if (check == -1) {
		return -ENAMETOOLONG;
	}

	/* Find the metadata of the parent directory. */
	find_metadata(parent_path, &parent_dir);
	if (parent_dir.file_check == 0) {
		return -ENOENT;
	}

	printf("Found metadata and got past beginning\n");

	current_data = (metadata*) malloc(sizeof(metadata) * max_metadata);
	current_block = parent_dir.first_block;
	while (1) {
		fseek(disk, current_block * block_size, SEEK_SET);
		fread(current_data, sizeof(metadata), max_metadata, disk);
		/* loop through current block of parent data until file metadata is found */
		for (i = 2; i < max_metadata; i++) {
			if (current_data[i].file_check == 1 &&
					!strncmp(current_data[i].name, name, name_size)) {
				old_size = current_data[i].size;

				/* update metadata and write it back to disk */
				current_data[i].size = size;
				num_blocks_needed = size/block_size;
				printf("Current block is %u in truncate.\n", current_block);
				fseek(disk, current_block * block_size, SEEK_SET);
				fwrite(current_data, sizeof(metadata), max_metadata, disk);

				/* allocate enough blocks to hold size amount of data*/
				if (old_size > size) {
					while(current_block != 0 && num_blocks != 0){
						if (num_blocks_needed == 1) {
							FAT[current_block] = 0;
							current_block = FAT[current_block];
						} else if (num_blocks_needed > 0) {
							current_block = FAT[current_block];
						} else {
							next_block = FAT[current_block];
							FAT[current_block] = fs_superblock.free_space_start;
							fs_superblock.free_space_start = current_block;
							current_block = next_block;
						}
						num_blocks_needed--;
					}
				} else {
					for (j = 0; j < num_blocks_needed; j++) {
						if (FAT[current_block] == 0) {
							FAT[current_block] = allocate_new_block();
						}
						current_block = FAT[current_block];
					}
				}
				free(current_data);
				printf("Well this return shouldn't be a problem\n");
				return 0;
			}
		}

		if (FAT[current_block] == 0) {
			free(current_data);
			printf("ENOENT everyone\n");
			return -ENOENT;
		} else {
			current_block = FAT[current_block];
		}
	}
}

static int fat_utimens(const char *path, const struct timespec ts[2])
{
	fprintf(stderr, "fat_utimens was called.");
	return -ENOSYS;
}

static int fat_open(const char *path, struct fuse_file_info *fi)
{
	metadata file_metadata;

	find_metadata(path, &file_metadata);

	/* If our find_metadata look-up is unsuccessful, we return an error. */
	if (file_metadata.file_check == 0) {
		return -ENOENT;
	}

	return 0;
}

int find_offset(const char *path, size_t starting_block)
{
	size_t current_block;
	size_t i;
	metadata file_metadata;

	printf("starting_block is %u in find_offset.\n", starting_block);

	find_metadata(path, &file_metadata);

	if (file_metadata.file_check == 0) {
		return -ENOENT;
	}

	current_block = file_metadata.first_block;

	/* Find the block we want to start reading at. */
	for (i = 0; i < starting_block; i++) {

		/* In this case, our offset is outside of the file. */
		if (FAT[current_block] == 0) {
			return 0;
		}

		current_block = FAT[current_block];
	}

	printf("current_block is %u.\n", current_block);

	return current_block;
}

static int fat_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	size_t current_block;
	size_t offset_in_block;
	size_t bytes_read;
	size_t starting_block;
	char block_read[block_size];

	offset_in_block = offset % block_size;
	starting_block = offset/block_size;

	current_block = find_offset(path, starting_block);

	if (current_block < 1) {
		return current_block;
	}

	fseek(disk, block_size * current_block, SEEK_SET);
	fread(block_read, block_size, 1, disk);

	bytes_read = size;

	if (bytes_read > (block_size - offset)) {
		bytes_read = (block_size - offset);
	}

	memcpy(buf, block_read + offset_in_block, bytes_read);

	return bytes_read;
}

static int fat_readlink(const char *path, char *buf, size_t size)
{
	metadata file_metadata;
	size_t amount_copied = size - 1;
	char file_block[block_size];

	find_metadata(path, &file_metadata);

	if (file_metadata.file_check == 0) {
		return -ENOENT;
	} else if (file_metadata.file_type != 2) {
		return 0;
	}

	if (file_metadata.size < amount_copied) {
		amount_copied = file_metadata.size;
	}

	fseek(disk, block_size * file_metadata.first_block, SEEK_SET);
	fread(file_block, block_size, 1, disk);
	memcpy(buf, file_block, amount_copied);

	return 0;
}


static int fat_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	size_t current_block;
	size_t offset_in_block;
	size_t bytes_read;
	size_t starting_block;
	metadata file_metadata;
	char seed_read[block_size + 1];
	char unencoded_read[2 * block_size + 1];
	int check = 0;

	printf("Inside write function.\n");

	// These characters at the beginning are needed to determine whether
	// we want to encode it into a seed or unencode it into text. The
	// python script will remove them.
	seed_read[0] = 'e';
	unencoded_read[0] = 'u';

	find_metadata(path, &file_metadata);

	if (file_metadata.file_check == 0) {
		return -ENOENT;
	}

	offset_in_block = offset % block_size;
	starting_block = offset/block_size;

	if (offset + size > file_metadata.size) {
		check = fat_truncate(path, offset + size);
		printf("Check is %d\n", check);

		if (check != 0) {
			printf("Truncate is sad, check Truncate\n");
			return check;
		}
	}

	current_block = find_offset(path, starting_block);

	// Read the seed that was already in the disk
	fseek(disk, block_size * current_block, SEEK_SET);
	fread(seed_read + 1, block_size, 1, disk);

	// Send nencode read seed request
	fwrite(seed_read, 1, block_size + 1, asker);

	// Get answer from python program for the unencoded message
	fread(unencoded_read + 1, 1, 2 * block_size, answer);

	bytes_read = size;

	if (bytes_read > (block_size - offset)) {
		bytes_read = (block_size - offset);
	}

	// Do the write
	memcpy(unencoded_read + offset_in_block, buf, bytes_read);

	printf("Current block is %u in write.\n", current_block);

	// Send ask for seed
	fwrite(unencoded_read, 1, 2 * block_size + 1, asker);

	// Get answer from python program
	fread(seed_read, 1, block_size + 1, answer);

	fseek(disk, block_size * current_block, SEEK_SET);
	fwrite(seed_read, block_size, 1, disk);

	printf("Bytes read is %u", bytes_read);

	return bytes_read;
}

static int fat_statfs(const char *path, struct statvfs *stbuf)
{
	size_t num_free_blocks = 1;
	size_t current_block_num = fs_superblock.free_space_start;

	stbuf->f_bsize = block_size;
	stbuf->f_frsize = block_size;
	stbuf->f_fsid = fs_superblock.magic_number;
	stbuf->f_namemax = name_size;
	stbuf->f_blocks = num_blocks;

	while (current_block_num != 0) {
		num_free_blocks++;
		current_block_num = FAT[current_block_num];
	}
	stbuf->f_bavail = num_free_blocks;
	stbuf->f_bfree = num_free_blocks;
	return 0;
}

static int fat_release(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int fat_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	fprintf(stderr, "fat_fsync was called.");
	return -ENOSYS;
}

static int fat_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	dev_t rdev;
	return fat_mknod(path, mode | S_IFREG, rdev);
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int fat_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	fprintf(stderr, "fat_setxattr was called.");
	return -ENOSYS;
}

static int fat_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	fprintf(stderr, "fat_getxattr was called.");
	return -ENOSYS;
}

static int fat_listxattr(const char *path, char *list, size_t size)
{
	fprintf(stderr, "fat_listxattr was called.");
	return -ENOSYS;
}

static int fat_removexattr(const char *path, const char *name)
{
	fprintf(stderr, "fat_removexattr was called.");
	return -ENOSYS;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations fat_oper = {
	.init		= fat_init,
	.destroy	= fat_destroy,
	.getattr	= fat_getattr,
	.access		= fat_access,
	.readlink	= fat_readlink,
	.readdir	= fat_readdir,
	.mknod		= fat_mknod,
	.mkdir		= fat_mkdir,
	.symlink	= fat_symlink,
	.unlink		= fat_unlink,
	.rmdir		= fat_rmdir,
	.rename		= fat_rename,
	.link		= fat_link,
	.chmod		= fat_chmod,
	.chown		= fat_chown,
	.truncate	= fat_truncate,
	.utimens	= fat_utimens,
	.open		= fat_open,
	.read		= fat_read,
	.write		= fat_write,
	.statfs		= fat_statfs,
	.release	= fat_release,
	.fsync		= fat_fsync,
	.create		= fat_create,
#ifdef HAVE_SETXATTR
	.setxattr	= fat_setxattr,
	.getxattr	= fat_getxattr,
	.listxattr	= fat_listxattr,
	.removexattr	= fat_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	current_path = get_current_dir_name();
	return fuse_main(argc, argv, &fat_oper, NULL);
}

