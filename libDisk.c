#include "libDisk.h"

int openDisk(char *filename, int nBytes)
{
    // file descriptor for disk
    int fd = -1;

    // error if less than 3 blocks, byte size not multiple of 256, byte size larger than max disk size
    // SLIGHT ISSUE HERE IF SETTING nBytes < BLOCKSIZE * 3
    if ((nBytes < BLOCKSIZE * 3 && nBytes != 0) || nBytes % BLOCKSIZE != 0 || \
        nBytes > MAX_DISK_SIZE)
        return INVALID_OP; // invalid nBytes size

    // open file
    if (nBytes == 0) // disk already exists
    {
        fd = open(filename, O_RDWR);
        if (fd < 0)
        {
            return OPEN_ERR; // open error
        }
    }
    else // create new disk
    {
        // open disk
        fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, \
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd < 0)
        {
            return OPEN_ERR; // open error
        }
        // fill in disk space with null characters
        uint8_t *buf = (uint8_t *) calloc(nBytes, 1);
        if (buf == NULL)
            return MALLOC_ERR; // calloc error
        if (write(fd, buf, nBytes) < 0)
        {
            free(buf);
            return WRITE_ERR; // write error
        }

        // free buffer
        free(buf);
    }

    // return disk file descriptor
    return fd;
}

int readBlock(int disk, int bNum, void *block)
{
    // find disk size
    int disk_size = get_disk_size(disk);
    if (disk_size < 0)
        return LSEEK_ERR; // lseek error

    // error check number of blocks
    if (bNum < 0 || bNum * BLOCKSIZE >= disk_size)
        return INVALID_OP; // invalid number of blocks

    // read bNum bytes from disk and copy to block buffer
    if (lseek(disk, bNum * BLOCKSIZE, SEEK_SET) < 0)
        return LSEEK_ERR; // lseek error

    // read block into buffer
    if (read(disk, block, BLOCKSIZE) < 0)
        return READ_ERR; // read error

    // successful return
    return 0;
}

int writeBlock(int disk, int bNum, void *block)
{
    // find disk size
    int disk_size = get_disk_size(disk);
    if (disk_size < 0)
        return LSEEK_ERR; // lseek error

    // error check number of blocks
    if (bNum < 0 || bNum * BLOCKSIZE >= disk_size)
        return INVALID_OP; // invalid number of blocks

    // read bNum bytes from disk and copy to block buffer
    if (lseek(disk, bNum * BLOCKSIZE, SEEK_SET) < 0)
        return LSEEK_ERR; // lseek error

    // write block to disk
    if (write(disk, block, BLOCKSIZE) < 0)
        return WRITE_ERR; // write error

    // successful return
    return 0;
}

int closeDisk(int disk)
{
    // close disk (returns 0 if successful, -1 if error)
    if (close(disk) < 0){
        return CLOSE_ERR;
    }
    return 0;
}

int get_disk_size(int disk)
{
    // return disk size, or -1 if error
    return lseek(disk, 0, SEEK_END);
}