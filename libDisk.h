#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#include "errorCode.h"

#define BLOCKSIZE 256
#define MAX_DISK_SIZE 520192 //  256 * 254 * 8

int openDisk(char *filename, int nBytes);

int readBlock(int disk, int bNum, void *block);

int writeBlock(int disk, int bNum, void *block);

int closeDisk(int disk);

int get_disk_size(int disk);