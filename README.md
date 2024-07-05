# tinyFS
Name: Jimmy Chen, Sean Du

Main Functionality:
    Our TinyFS works pretty well, first create a new directory by running tfs_mkfs which creates a valid file system with the magic number 0x5A. You can mount any created directories with tfs_mount using the directory name, and since only one directory can be mounted at a time, tfs_unmount the directory that is currently mounted on the file system. For our implementation, our max disk size is set to 256 * 254 * 8, which is the capacity of our bit array of free block. We used a bit array for our disk because it is faster to search for a free block and it is more space efficient. Our max file name length is set to 8 so file names greater than 8 are invalid upon attempting to create a new file. For our inodes and dynamic resource table we decided to keep track of them using linked lists, this allows us to create an arbitary number of files or an arbitary long file only limited by disk space.

Additonal Functionality:
    For our additional functionality we choose tfs_readdir(), tfs_rename(), and a time stamp system. Our readdir() directly prints out all the file in the root directory, and our rename() changes the name of an open file (using FD). The time stamp system is managed in our file inode, and includes creation, modification and access time stamp.

Limitations and Bugs:
    - If we had more time we could have created a struct based system for the time stamp, making it easier to keep track of all the different time stamps for each file.
