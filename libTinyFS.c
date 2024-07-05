#include "libTinyFS.h"

// file descriptor of mounted file system
fileDescriptor mounted_disk = -1;

// make dynamic resource table
LinkedList *resource_table = NULL;

// make a new file system
int tfs_mkfs(char *filename, int nBytes)
{
    if (mounted_disk >= 0)
    {
        int err = tfs_unmount();
        if (err < 0)
            return err; // unmount error
    }

    if (resource_table == NULL)
        resource_table = create_linked_list();

    // create disk
    int disk = openDisk(filename, nBytes);
    if (disk < 0)
    {
        return disk; // open disk error
    }
    
    // make block buffer
    uint8_t *block = (uint8_t *) malloc(BLOCKSIZE);
    if (block == NULL)
        return MALLOC_ERR; // malloc error

    // make superblock
    block[MAGIC_INDEX] = MAGIC; // magic number
    block[ROOT_INODE_INDEX] = ROOT_INODE; // block of root directory inode
    
    // block[2] to block[255]: bit array used to track free blocks
    // each byte in this bit array is little endian
    // 0 bit = free, 1 bit == not free
    memset(&block[FREE_LIST_INDEX], 0, 254);
    unfree_block(block, SUPERBLOCK);
    unfree_block(block, ROOT_INODE);

    // write superblock to disk
    if (writeBlock(disk, SUPERBLOCK, block) < 0)
    {
        return WRITE_ERR; // write error
    }
    // empty block for root directory inode
    memset(block, 0, BLOCKSIZE);

    // make root directory inode
    LinkedList *root_inode = create_linked_list();
    memcpy(block, &root_inode, sizeof(LinkedList *));
    
    // write root directory inode to disk
    if (writeBlock(disk, ROOT_INODE, block) < 0)
    {
        free_linked_list(root_inode);
        return WRITE_ERR; // write error
    }

    if (closeDisk(disk) < 0)
    {
        free_linked_list(root_inode);
        return CLOSE_ERR; // close error
    }

    // free stuff
    free(block);

    // successful return
    return 0;
}

// mount a file system
int tfs_mount(char *filename)
{
    int err;

    // unmount a disk if one is already mounted
    if (mounted_disk >= 0)
    {
        err = tfs_unmount();
        if (err < 0)
            return err; // unmount error
    }

    // open mounted file
    mounted_disk = openDisk(filename, 0);
    if (mounted_disk < 0)
        return mounted_disk; // open disk error

    // make block buffer
    uint8_t *block = (uint8_t *) malloc(BLOCKSIZE);
    if (block == NULL)
        return MALLOC_ERR; // malloc error

    // check the file system is valid
    err = readBlock(mounted_disk, SUPERBLOCK, block);
    
    if (err < 0)
    {
        free(block);
        return err; // read error
    }
    if ((block[MAGIC_INDEX] != MAGIC) || (block[ROOT_INODE_INDEX] != ROOT_INODE))
    {
        free(block);
        return INVALID_DISK; // Disk is invalid
    }

    // free block buffer
    free(block);

    // successful return
    return 0;
}

int tfs_unmount(void)
{
    if (mounted_disk >= 0)
    {
        // close mounted file
        int err = closeDisk(mounted_disk);
        if (err < 0)
            return err; // close error
        mounted_disk = -1;
        
    }
    // successful return
    return 0;
}

fileDescriptor tfs_open(char *name)
{
    // check if filename is valid
    if (strlen(name) > MAX_FILENAME_LEN)
        return INVALID_OP; // invalid filename length

    // check if the file is already open
    Node *cur = resource_table->front;
    while (cur->next != NULL)
    {
        // if file is already open
        if (!strncmp(name, ((Resource_table_entry *) cur->next->data)->filename, MAX_FILENAME_LEN))
            return ((Resource_table_entry *) cur->next->data)->fd; // return open file descriptor
        cur = cur->next;
    }

    // check if the file exists on the mounted disk
    // make block buffer
    uint8_t *root_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (root_inode == NULL)
        return MALLOC_ERR; // malloc error
    
    // read the block containing the root directory inode
    int err = readBlock(mounted_disk, ROOT_INODE, root_inode);
    if (err < 0)
    {
        free(root_inode);
        return err; // read error
    }
    // iterate through root directory inode to see if file exists
    cur = (*((LinkedList **) root_inode))->front;
    while (cur->next != NULL)
    {
        if (!strncmp(name, ((Root_inode_entry *) cur->next->data)->filename, MAX_FILENAME_LEN))
        {
            free(root_inode);
            break;
        }
        cur = cur->next;
    }

    // if file does not exist, create file
    if (cur->next == NULL)
    {   
        // create buffer for superblock
        uint8_t *superblock = (uint8_t *) malloc(BLOCKSIZE);
        if (superblock == NULL)
            return MALLOC_ERR; // malloc error

        // read superblock
        err = readBlock(mounted_disk, SUPERBLOCK, superblock);
        if (err < 0)
        {
            free(root_inode);
            free(superblock);
            return err; // read error
        }

        // find a free block and unfree it
        int new_addr = unfree_first_free_block(superblock);
        if (new_addr < 0)
        {
            free(root_inode);
            free(superblock);
            return new_addr; // no free blocks
        }

        // write superblock back to disk
        err = writeBlock(mounted_disk, SUPERBLOCK, superblock);
        if (err < 0)
        {
            free(root_inode);
            free(superblock);
            return err; // write error
        }

        // free superblock
        free(superblock);

        // create new root directory inode entry
        Root_inode_entry *root_inode_entry = (Root_inode_entry *) \
            malloc(sizeof(Root_inode_entry));
        if (root_inode_entry == NULL)
        {
            free(root_inode);
            return MALLOC_ERR; // malloc error
        }
        strncpy(root_inode_entry->filename, name, MAX_FILENAME_LEN);
        root_inode_entry->addr = new_addr;
        root_inode_entry->size = 0;

        // add new root inode entry to linked list
        if (append(*((LinkedList **) root_inode), root_inode_entry) < 0)
        {
            free(root_inode);
            return MALLOC_ERR; // linked list malloc error
        }
        
        // free root inode
        free(root_inode);

        // add file inode
        uint8_t *file_inode = (uint8_t *) malloc(BLOCKSIZE);
        if (file_inode == NULL)
            return MALLOC_ERR; // malloc error
        
        memset(file_inode, 0, BLOCKSIZE);
        LinkedList *file_inode_entry = create_linked_list();
        memcpy(file_inode, &file_inode_entry, sizeof(LinkedList *));

        // add creation time
        time((time_t *)&file_inode[sizeof(LinkedList *)]);

        // add access time
        time((time_t *)&file_inode[sizeof(LinkedList *) + sizeof(time_t *)]);

        // add modification time
        time((time_t *)&file_inode[sizeof(LinkedList *) + 2 * sizeof(time_t *)]);

        // write file inode entry to disk
        err = writeBlock(mounted_disk, new_addr, file_inode);
        if (err < 0)
        {
            free(file_inode);
            return err; // write error
        }

        // free block
        free(file_inode);
    }
    
    // create new entry for resource table
    Resource_table_entry *resource_table_entry = (Resource_table_entry *) \
        malloc(sizeof(Resource_table_entry));
    if (resource_table_entry == NULL)
        return MALLOC_ERR; // malloc error
    strncpy(resource_table_entry->filename, name, MAX_FILENAME_LEN);

    if (resource_table->size == 0)
        resource_table_entry->fd = 0;
    else
    {
        resource_table_entry->fd = \
            (((Resource_table_entry *) resource_table->back->data)->fd) + 1;
    }
    resource_table_entry->fp = 0;
    

    // append new entry to resource table linked list
    if (append(resource_table, resource_table_entry) < 0)
        return MALLOC_ERR; // linked list malloc error

    // successful return
    return resource_table_entry->fd;
}

int tfs_close(fileDescriptor FD)
{
    // find entry in resource table
    Node *cur = resource_table->front;
    while (cur != NULL && cur->next != NULL)
    {
        if (((Resource_table_entry *) cur->next->data)->fd == FD)
        {
            delete(resource_table, cur);
            return 0;
        }
        cur = cur->next;
    }
    return NO_FD; // file not open or doesn't exist
}

int tfs_write(fileDescriptor FD, char *buffer, int size)
{
    int err;
    // create buffer for file name
    char *filename = (char *) malloc(MAX_FILENAME_LEN);
    if (filename == NULL)
        return MALLOC_ERR; // malloc error

    // check if file exists and is open
    err = get_filename(FD, filename);
    if (err < 0)
    {
        free(filename);
        return err; // file not open or doesn't exist
    }

    // create superblock buffer
    uint8_t *superblock = (uint8_t *) malloc(BLOCKSIZE);
    if (superblock == NULL)
    {
        free(filename);
        return MALLOC_ERR; // malloc error
    }

    // read superblock
    err = readBlock(mounted_disk, SUPERBLOCK, superblock);
    if (err < 0)
    {
        free(filename);
        free(superblock);
        return err; // read error
    }

    // create root directory inode buffer
    uint8_t *root_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (root_inode == NULL)
    {
        free(filename);
        free(superblock);
        return MALLOC_ERR; // malloc error
    }
    
    // read the block containing the root directory inode
    err = readBlock(mounted_disk, ROOT_INODE, root_inode);
    if (err < 0)
    {
        free(filename);
        free(superblock);
        free(root_inode);
        return err; // read error
    }

    // find block with file inode
    int file_inode_addr = 0;
    Node *root_inode_entry = (*((LinkedList **) root_inode))->front;
    while (root_inode_entry->next != NULL)
    {
        if (!strncmp(((Root_inode_entry *) \
            root_inode_entry->next->data)->filename, filename, \
            MAX_FILENAME_LEN))
        {
            file_inode_addr = \
                ((Root_inode_entry *) root_inode_entry->next->data)->addr;
            root_inode_entry = root_inode_entry->next;
            break;
        }
        root_inode_entry = root_inode_entry->next;
    }

    // free stuff
    free(filename);
    free(root_inode);

    // create file inode buffer
    uint8_t *file_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (file_inode == NULL)
        return MALLOC_ERR; // malloc error
    
    // read the block containing the file inode
    err = readBlock(mounted_disk, file_inode_addr, file_inode);
    if (err < 0)
    {
        free(superblock);
        free(file_inode);
        return err; // read error
    }

    // update modification time
    time((time_t *)&file_inode[sizeof(LinkedList *) + 2 * sizeof(time_t *)]);

    // compare new size to old size
    int bytes_written = 0;
    int new_free_block_addr = 0;
    Node *cur = (*((LinkedList **) file_inode))->front;
    while (1)
    {
        // if there are no more blocks allocated for the file
        if (cur->next == NULL)
        {
            // allocate a new free block to the file
            new_free_block_addr = unfree_first_free_block(superblock);
            if (new_free_block_addr < 0)
            {
                free(superblock);
                free(file_inode);
                return DISK_FULL; // no more disk space
            }
            // add a file inode entry
            File_inode_entry *file_inode_entry = (File_inode_entry *) \
                malloc(sizeof(File_inode_entry));
            if (file_inode_entry == NULL)
            {
                free(superblock);
                free(file_inode);
                return MALLOC_ERR; // malloc error
            }
            file_inode_entry->addr = new_free_block_addr;
            if (append(*((LinkedList **) file_inode), file_inode_entry) < 0)
            {
                free(superblock);
                free(file_inode);
                return MALLOC_ERR; // linked list malloc error
            }
        }

        // write a full block
        if (size - bytes_written >= BLOCKSIZE)
        {
            err = writeBlock(mounted_disk, ((File_inode_entry *) \
                cur->next->data)->addr, &buffer[bytes_written]);
            if (err < 0)
            {
                free(superblock);
                free(file_inode);
                return err; // write error
            }
            bytes_written += BLOCKSIZE;
            cur = cur->next;
        }
        // write a partial block, then fill in rest of block with null
        else if (size - bytes_written > 0)
        {
            uint8_t *temp = (uint8_t *) calloc(BLOCKSIZE, 1);
            if (temp == NULL)
            {
                free(superblock);
                free(file_inode);
                return MALLOC_ERR; // calloc error
            }
            memcpy(temp, &buffer[bytes_written], size - bytes_written);
            err = writeBlock(mounted_disk, ((File_inode_entry *) \
                cur->next->data)->addr, &buffer[bytes_written]);
            if (err < 0)
            {
                free(temp);
                free(superblock);
                free(file_inode);
                return err; // write error
            }
            free(temp);
            bytes_written = size;
            cur = cur->next;
        }

        // done writing
        if (bytes_written == size)
        {
            while (cur->next != NULL)
            {
                free_block(superblock, \
                    ((File_inode_entry *) cur->next->data)->addr);
                delete(*((LinkedList **) file_inode), cur);
            }
            break;
        }
    }

    free(file_inode);
    err = writeBlock(mounted_disk, SUPERBLOCK, superblock);
    if (err < 0)
    {
        free(superblock);
        return err; // write error
    }

    // free superblock
    free(superblock);

    // update number of bytes written to 
    ((Root_inode_entry *) root_inode_entry->data)->size = bytes_written;
    
    // move file pointer to front
    cur = resource_table->front;
    while (cur->next != NULL)
    {
        if (((Resource_table_entry *) cur->next->data)->fd == FD)
        {
            ((Resource_table_entry *) cur->next->data)->fp = 0;
            break;
        }
        cur = cur->next;
    }

    // successful return
    return 0;
}

int tfs_delete(fileDescriptor FD)
{
    int err;
    // create buffer for file name
    char *filename = (char *) malloc(MAX_FILENAME_LEN);
    if (filename == NULL)
        return MALLOC_ERR; // malloc error

    // check if file exists and is open
    err = get_filename(FD, filename);
    if (err < 0)
    {
        free(filename);
        return err; // file not open or doesn't exist
    }

    // create superblock buffer
    uint8_t *superblock = (uint8_t *) malloc(BLOCKSIZE);
    if (superblock == NULL)
    {
        free(filename);
        return MALLOC_ERR; // malloc error
    }

    // read superblock
    err = readBlock(mounted_disk, SUPERBLOCK, superblock);
    if (err < 0)
    {
        free(filename);
        free(superblock);
        return err; // read error
    }

    // create root directory inode buffer
    uint8_t *root_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (root_inode == NULL)
    {
        free(filename);
        free(superblock);
        return MALLOC_ERR; // malloc error
    }
    
    // read the block containing the root directory inode
    err = readBlock(mounted_disk, ROOT_INODE, root_inode);
    if (err < 0)
    {
        free(filename);
        free(superblock);
        free(root_inode);
        return err; // read error
    }

    // find block with file inode
    int file_inode_addr = 0;
    Node *cur = (*((LinkedList **) root_inode))->front;
    while (cur->next != NULL)
    {
        if (!strncmp(((Root_inode_entry *) cur->next->data)->filename, \
            filename, MAX_FILENAME_LEN))
        {
            // set address of file inode
            file_inode_addr = ((Root_inode_entry *) cur->next->data)->addr;

            // delete file inode
            delete(*((LinkedList **) root_inode), cur);
            break;
        }
        cur = cur->next;
    }

    // free stuff
    free(filename);
    free(root_inode);

    // create file inode buffer
    uint8_t *file_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (file_inode == NULL)
        return MALLOC_ERR; // malloc error
    
    // read the block containing the file inode
    err = readBlock(mounted_disk, file_inode_addr, file_inode);
    if (err < 0)
    {
        free(superblock);
        free(file_inode);
        return err; // read error
    }

    // free the block containing the file inode
    free_block(superblock, file_inode_addr);

    // set data blocks free
    cur = (*((LinkedList **) file_inode))->front;
    while (cur->next != NULL)
    {
        free_block(superblock, ((File_inode_entry *) cur->next->data)->addr);
        cur = cur->next;
    }

    // write superblock back to disk
    err = writeBlock(mounted_disk, SUPERBLOCK, superblock);
    if (err < 0)
    {
        free_linked_list(*((LinkedList **) file_inode));
        free(superblock);
        free(file_inode);
        return err; // write error
    }

    // free stuff
    free_linked_list(*((LinkedList **) file_inode));
    free(superblock);
    free(file_inode);

    // remove file from resource table and return
    return tfs_close(FD);
}

int tfs_readByte(fileDescriptor FD, char *buffer)
{
    int err;
    // create buffer for file name
    char *filename = (char *) malloc(MAX_FILENAME_LEN);
    if (filename == NULL)
        return MALLOC_ERR; // malloc error

    // check if file exists and is open
    err = get_filename(FD, filename);
    if (err < 0)
    {
        free(filename);
        return err; // file not open or doesn't exist
    }

    // create root directory inode buffer
    uint8_t *root_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (root_inode == NULL)
    {
        free(filename);
        return MALLOC_ERR; // malloc error
    }
    
    // read the block containing the root directory inode
    err = readBlock(mounted_disk, ROOT_INODE, root_inode);
    if (err < 0)
    {
        free(filename);
        free(root_inode);
        return err; // read error
    }

    // find block with file inode
    int file_inode_addr = 0;
    int size = 0;
    Node *cur = (*((LinkedList **) root_inode))->front;
    while (cur->next != NULL)
    {
        if (!strncmp(((Root_inode_entry *) cur->next->data)->filename, \
            filename, MAX_FILENAME_LEN))
        {
            file_inode_addr = ((Root_inode_entry *) cur->next->data)->addr;
            size = ((Root_inode_entry *) cur->next->data)->size;
            break;
        }
        cur = cur->next;
    }

    // free filename
    free(filename);

    // create file inode buffer
    uint8_t *file_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (file_inode == NULL)
        return MALLOC_ERR; // malloc error
    
    // read the block containing the file inode
    err = readBlock(mounted_disk, file_inode_addr, file_inode);
    if (err < 0)
    {
        free(root_inode);
        free(file_inode);
        return err; // read error
    }

    // get offset
    cur = ((LinkedList *) resource_table)->front;
    int fp;
    while (cur->next != NULL)
    {
        if (((Resource_table_entry *) cur->next->data)->fd == FD)
        {
            fp = ((Resource_table_entry *) cur->next->data)->fp;
            break;
        }
        cur = cur->next;
    }

    // make sure file pointer isn't at EOF
    if (fp >= size)
    {
        free(root_inode);
        free(file_inode);
        return EOF_ERR; // no bytes left to read
    }
    else // increment file pointer
        ((Resource_table_entry *) cur->next->data)->fp += 1;

    // iterate to correct file inode entry
    cur = (*((LinkedList **) file_inode))->front;
    int i;
    for (i = 0; i < fp / BLOCKSIZE; i++)
        cur = cur->next;

    // get address of block
    int addr = ((File_inode_entry *) cur->next->data)->addr;

    // free stuff
    free(root_inode);
    free(file_inode);

    // get data block buffer
    uint8_t *data_block = (uint8_t *) malloc(BLOCKSIZE);
    if (data_block == NULL)
        return MALLOC_ERR; // malloc error
    
    // read the block containing the file inode
    err = readBlock(mounted_disk, addr, data_block);
    if (err < 0)
    {
        free(data_block);
        return err; // read error
    }

    // copy byte into buffer
    memcpy(buffer, &data_block[fp % BLOCKSIZE], 1);

    // free data block
    free(data_block);

    // successful return
    return 0;
}

int tfs_seek(fileDescriptor FD, int offset)
{
    int err;
    // create buffer for file name
    char *filename = (char *) malloc(MAX_FILENAME_LEN);
    if (filename == NULL)
        return MALLOC_ERR; // malloc error

    // check if file exists and is open
    err = get_filename(FD, filename);
    if (err < 0)
    {
        free(filename);
        return err; // file not open or doesn't exist
    }

    // create root directory inode buffer
    uint8_t *root_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (root_inode == NULL)
    {
        free(filename);
        return MALLOC_ERR; // malloc error
    }
    
    // read the block containing the root directory inode
    err = readBlock(mounted_disk, ROOT_INODE, root_inode);
    if (err < 0)
    {
        free(filename);
        free(root_inode);
        return err; // read error
    }

    // iterate through root directory inode to get size
    Node *cur = (*((LinkedList **) root_inode))->front;
    int size = 0;
    while (cur->next != NULL)
    {
        if (!strncmp(((Root_inode_entry *) cur->next->data)->filename, \
            filename, MAX_FILENAME_LEN))
        {
            size = ((Root_inode_entry *) cur->next->data)->size;
            break;
        }
        cur = cur->next;
    }

    // free stuff
    free(filename);
    free(root_inode);

    // check if offset is invalid
    if (offset < 0 || offset > size)
        return INVALID_OP; // invalid offset

    // seek file pointer
    cur = ((LinkedList *) resource_table)->front;
    while (cur->next != NULL)
    {

        if (((Resource_table_entry *) cur->next->data)->fd == FD)
        {       
            ((Resource_table_entry *) cur->next->data)->fp = offset;
            break;
        }
        cur = cur->next;
    }
    return 0;
}

int tfs_rename(fileDescriptor FD, char *new_name)
{
    int err;

    if (strlen(new_name) > MAX_FILENAME_LEN)
        return INVALID_OP; // invalid filename length

    // create buffer for file name
    char *filename = (char *) malloc(MAX_FILENAME_LEN);
    if (filename == NULL)
        return MALLOC_ERR; // malloc error

    // check if file exists and is open
    err = get_filename(FD, filename);
    if (err < 0)
    {
        free(filename);
        return err; // file not open or doesn't exist
    }

    // create root directory inode buffer
    uint8_t *root_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (root_inode == NULL)
    {
        free(filename);
        return MALLOC_ERR; // malloc error
    }
    
    // read the block containing the root directory inode
    err = readBlock(mounted_disk, ROOT_INODE, root_inode);
    if (err < 0)
    {
        free(filename);
        free(root_inode);
        return err; // read error
    }

    // iterate through root directory inode to get filename
    Node *cur = (*((LinkedList **) root_inode))->front;
    while (cur->next != NULL)
    {
        if (!strncmp(((Root_inode_entry *) cur->next->data)->filename, \
            filename, MAX_FILENAME_LEN))
        {
            strncpy(((Root_inode_entry *) cur->next->data)->filename, \
                new_name, MAX_FILENAME_LEN);
            break;
        }
        cur = cur->next;
    }

    // find block with file inode
    int file_inode_addr = 0;
    cur = (*((LinkedList **) root_inode))->front;
    while (cur->next != NULL)
    {
        if (!strncmp(((Root_inode_entry *) cur->next->data)->filename, \
            filename, MAX_FILENAME_LEN))
        {
            file_inode_addr = ((Root_inode_entry *) cur->next->data)->addr;
            break;
        }
        cur = cur->next;
    }

    // free filename
    free(filename);
    free(root_inode);

    // create file inode buffer
    uint8_t *file_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (file_inode == NULL)
        return MALLOC_ERR; // malloc error
    
    // read the block containing the file inode
    err = readBlock(mounted_disk, file_inode_addr, file_inode);
    if (err < 0)
    {
        free(file_inode);
        return err; // read error
    }

    cur = resource_table->front;
    while (cur->next != NULL)
    {
        if (((Resource_table_entry *) cur->next->data)->fd == FD)
        {
            strncpy(((Resource_table_entry *) cur->next->data)->filename, \
                new_name, MAX_FILENAME_LEN);
            break;
        }
        cur = cur->next;
    }

    // update modification time
    time((time_t *)&file_inode[sizeof(LinkedList *) + 2 * sizeof(time_t *)]);

    // write file inode entry to disk
    err = writeBlock(mounted_disk, file_inode_addr, file_inode);
    if (err < 0)
    {
        free(file_inode);
        return err; // write error
    }

    // free filename
    free(file_inode);

    // successful return
    return 0;
}

int tfs_readdir()
{
    int err;
    // create root directory inode buffer
    uint8_t *root_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (root_inode == NULL)
        return MALLOC_ERR; // malloc error
    
    // read the block containing the root directory inode
    err = readBlock(mounted_disk, ROOT_INODE, root_inode);
    if (err < 0)
    {
        free(root_inode);
        return err; // read error
    }

    // iterate through root directory inode to get filename
    Node *cur = (*((LinkedList **) root_inode))->front;
    while (cur->next != NULL)
    {
        printf("%s\n", ((Root_inode_entry *) cur->next->data)->filename);
        cur = cur->next;
    }

    // free stuff
    free(root_inode);

    // successful return
    return 0;
}

int tfs_stat(fileDescriptor FD, struct tm *creation_time, \
    struct tm *access_time, struct tm *modification_time)
{
    int err;
    // create buffer for file name
    char *filename = (char *) malloc(MAX_FILENAME_LEN);
    if (filename == NULL)
        return MALLOC_ERR; // malloc error

    // check if file exists and is open
    err = get_filename(FD, filename);
    if (err < 0)
    {
        free(filename);
        return err; // file not open or doesn't exist
    }

    // create root directory inode buffer
    uint8_t *root_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (root_inode == NULL)
    {
        free(filename);
        return MALLOC_ERR; // malloc error
    }
    
    // read the block containing the root directory inode
    err = readBlock(mounted_disk, ROOT_INODE, root_inode);
    if (err < 0)
    {
        free(filename);
        free(root_inode);
        return err; // read error
    }

    // find block with file inode
    int file_inode_addr = 0;
    Node *cur = (*((LinkedList **) root_inode))->front;
    while (cur->next != NULL)
    {
        if (!strncmp(((Root_inode_entry *) cur->next->data)->filename, \
            filename, MAX_FILENAME_LEN))
        {
            file_inode_addr = ((Root_inode_entry *) cur->next->data)->addr;
            break;
        }
        cur = cur->next;
    }

    // free filename
    free(filename);
    free(root_inode);

    // create file inode buffer
    uint8_t *file_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (file_inode == NULL)
        return MALLOC_ERR; // malloc error
    
    // read the block containing the file inode
    err = readBlock(mounted_disk, file_inode_addr, file_inode);
    if (err < 0)
    {
        free(file_inode);
        return err; // read error
    }

    // get creation time
    localtime_r((time_t *)&file_inode[sizeof(LinkedList *)], creation_time);

    // get access time
    time((time_t *)&file_inode[sizeof(LinkedList *) + sizeof(time_t *)]);
    localtime_r((time_t *)&file_inode[sizeof(LinkedList *) + sizeof(time_t *)], access_time);

    // get modification time
    localtime_r((time_t *)&file_inode[sizeof(LinkedList *) + 2 * sizeof(time_t *)], modification_time);

    // free file inode
    free(file_inode);

    // successful return
    return 0;
}



// if the file is open, puts the file described by FD in the filename buffer
// returns 0 if successful, -1 if the file isn;t open/doesn't exist.
int get_filename(fileDescriptor FD, char *filename)
{
    // check if file exists and is open
    Node *cur = resource_table->front;
    while (cur->next != NULL)
    {
        if (((Resource_table_entry *) cur->next->data)->fd == FD)
        {
            strncpy(filename, ((Resource_table_entry *) \
                cur->next->data)->filename, MAX_FILENAME_LEN);
            return 0;
        }
        cur = cur->next;
    }
    return NO_FD;
}

void free_all()
{
    int err;
    // create root directory inode buffer
    uint8_t *root_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (root_inode == NULL)
        return; // malloc error
    uint8_t *file_inode = (uint8_t *) malloc(BLOCKSIZE);
    if (file_inode == NULL)
    {
        free(root_inode);
        return; // malloc error
    }
    
    // read the block containing the root directory inode
    err = readBlock(mounted_disk, ROOT_INODE, root_inode);
    if (err < 0)
    {
        free(root_inode);
        free(file_inode);
        return;
    }

    // delete all file on disk
    Node* cur = (*((LinkedList **) root_inode))->front;
    while (cur->next != NULL)
    {
        err = readBlock(mounted_disk, ((Root_inode_entry *) \
            cur->next->data)->addr, file_inode);
        if (err < 0)
        {
            free(root_inode);
            free(file_inode);
            return;
        }
        free_linked_list(*((LinkedList **) file_inode));
        cur = cur->next;
    }

    free_linked_list(*((LinkedList **) root_inode));
    free_linked_list(resource_table);
    free(root_inode);
    free(file_inode);
}

// free block in free blocks list
void free_block(uint8_t *superblock, int index)
{
    superblock[FREE_LIST_INDEX + index / BYTE] &= ~(ONE << index % BYTE);
}

// unfree block in free blocks list
void unfree_block(uint8_t *superblock, int index)
{
    superblock[FREE_LIST_INDEX + index / BYTE] |= (ONE << index % BYTE);
}

// unfree first block in free blocks list
// returns the address of that block
// returns -1 if there are no free blocks
int unfree_first_free_block(uint8_t *superblock)
{
    int i = 0;
    int j = 0;
    int disk_blocks = get_disk_size(mounted_disk) / BLOCKSIZE;
    for (i = FREE_LIST_INDEX; i < FREE_LIST_INDEX + disk_blocks / BYTE; i++)
    {
        if (superblock[i] != 0b11111111)
        {
            for (j = 0; j < BYTE; j++)
            {
                if (!(superblock[i] & (ONE << j)))
                {
                    // unfree block
                    superblock[i] |= (ONE << j);
                    
                    // update root directory inode
                    return BYTE * (i - FREE_LIST_INDEX) + j; 
                }
            }
        }
    }
    for (j = 0; j < disk_blocks % BYTE; j++)
    {
        if (!(superblock[i] & (ONE << j)))
        {
            // unfree block
            superblock[i] |= (ONE << j);
            
            // update root directory inode
            return BYTE * (i - FREE_LIST_INDEX) + j; 
        }
    }
    return DISK_FULL; // no free blocks
}