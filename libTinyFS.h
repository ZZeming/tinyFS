#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "libDisk.h"
#include "linkedList.h"


#define DEFAULT_DISK_SIZE 10240
#define DEFAULT_DISK_NAME "tinyFSDisk"
#define MAGIC 90 // 0x5A
#define MAX_FILENAME_LEN 8

#define ONE 0b00000001
#define BYTE 8

#define SUPERBLOCK 0
#define ROOT_INODE 1

#define MAGIC_INDEX 0
#define ROOT_INODE_INDEX 1
#define FREE_LIST_INDEX 2

typedef int fileDescriptor;

typedef struct Root_inode_entry
{
    char filename[MAX_FILENAME_LEN];
    int addr;
    int size;
} Root_inode_entry;

typedef struct File_inode_entry
{
    int addr;
} File_inode_entry;

typedef struct Resource_table_entry
{
    char filename[MAX_FILENAME_LEN];
    int fd;
    int fp;
} Resource_table_entry;

int tfs_mkfs(char *filename, int nBytes);

int tfs_mount(char *filename);

int tfs_unmount(void);

fileDescriptor tfs_open(char *name);

int tfs_close(fileDescriptor FD);

int tfs_write(fileDescriptor FD, char *buffer, int size);

int tfs_delete(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset);

int tfs_rename(fileDescriptor FD, char *new_name);

int tfs_readdir();

int tfs_stat(fileDescriptor FD, struct tm *creation_time, \
    struct tm *access_time, struct tm *modification_time);

int get_filename(fileDescriptor FD, char *filename);

void free_block(uint8_t *superblock, int index);

void unfree_block(uint8_t *superblock, int index);

int unfree_first_free_block(uint8_t *superblock);

void free_all();