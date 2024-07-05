#include "libTinyFS.h"

// 50 characters
#define SMALLSTR "3PF0CcEExCjTVrSFw9OWwg7Aa6FxZh7kBnTLCIEf4u6TQHUH8w"
// 256 characters
#define BIGSTR "mEPpiJeBCjO6tfBBSdHp0Y14zqAKbewmmsATWTKL9xUvMu20N1bq7WWIzDtqAuhuRndFpAt0jaswB9X9GG0xTZaKjWvE78eoTyUcLL0Q4u81Y6RYHT6RTdBhjSuw7cVzpRnqhU2IPHNVCyytznbO88EUuZLuxkkuCz50rEKIo0Y8vSS8VyzYSiI7hRKWsgamrRnT0QUxCfWCHXGEyPNe7RcG8d5SuI5DJJ1VakgjFRqf4ZFQjEO7qqvRrTAaOSNd"
// 512 characters
#define VERYBIGSTR "hlmBoSAOc7DUcQd9g1BsuXsGegVqHl8MQWOG77EEyM50GdjImL4PUqt0fRk9sJI5u3i7vWVfV29OUgtGtgKWx25n4wlubVyojIIEDZNzr36wYnKnAsXCvhdJRetGLcxMFEQCEjm21mlEvpw2migzsXHNYJJhxGR8s2OQHs9U5hFMm1ZUb747u4S2hQYICXAwhf0KwS8rpTkm66A7bbibJ0TxwJ02lsYFpRAX5T1a2Sq93n3NmSasAdlLx19dvyFDmyLlfvWxPCZlFKDghAZfdpPSkHT41EcmN4aH2jHkrECChjNHpGBXaX7tCBQQ3tlTZUULwPdubXYGikN8nqt09IO4qFmGa8yM2eDs83LEsSSup3fBh69s2129PlT0JgSCS83zos3lHqyoxJ050N4Sby1yqsOK5VD4Y6B4C7pptdAPF64LIScjIupU6zZULL6b9bKyqdlkBfQI4snQl7cPbc1S52DCuLTgfvS2CI6NMpFpwK0vnpun57lFhMbwaEaW"
// Max disk size character string

int main(int argc, char *argv[]){

    int fd1 = -1;
    int fd2 = -1;
    int fd3 = -1;
    int fd4 = -1;
    int i;
    int err;
    char buffer[51];
    char bigbuf[257];
    char biggerbuf[513];

    struct tm *creation_time = (struct tm *) malloc(sizeof(struct tm));
    struct tm *access_time = (struct tm *) malloc(sizeof(struct tm));
    struct tm *modification_time = (struct tm *) malloc(sizeof(struct tm));

    // run mkfs
    err = tfs_mkfs(DEFAULT_DISK_NAME, 3*BLOCKSIZE);
    if (err >= 0)
        printf("mkfs (new disk): success\n");
    else
        printf("mkfs (new disk): failure\n");

    // resize the mounted disk
    err = tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE);
    if (err >= 0)
        printf("mkfs resize (existing disk): success\n");
    else
        printf("mkfs resize (existing disk): failure\n");

    //test invalid input
    err = tfs_mkfs(DEFAULT_DISK_NAME, 2*BLOCKSIZE);
    if (err >= 0)
        printf("mkfs (invalid input): failure\n");
    else
    {
        printf("mkfs (invalid input): success\n");
        print_error(err);
    }

    // create (second disk)
    err = tfs_mkfs("SMALL_DISK", 5*BLOCKSIZE);
    if (err >= 0)
        printf("mkfs (second disk): success\n");
    else
        printf("mkfs (second disk): failure\n");
    

    // mount (existing disk)
    err = tfs_mount(DEFAULT_DISK_NAME);
    if (err >= 0)
        printf("mount (existing disk): success\n");
    else
        printf("mount (existing disk): failure\n");

    // mount (non-existant disk)
    err = tfs_mount("blah blah blah");
    if (err >= 0)
        printf("mount (non-existant disk): failure\n");
    else
    {
        printf("mount (non-existant disk): success\n");
        print_error(err);
    }

    // mount (invalid disk format)
    // make a new disk with wrong magic number
    openDisk("WRONG_NUMBER", DEFAULT_DISK_SIZE);
    err = tfs_mount("WRONG_NUMBER");
    if (err >= 0)
        printf("open (new file with wrong magic number): failure\n");
    else
    {
        printf("open (new file with wrong magic number): success\n");
        print_error(err);
    }

    // unmount
    err = tfs_unmount();
    if (err >= 0)
        printf("unmount: success\n");
    else
        printf("unmount: failure\n");
    
    // open (new file)
    tfs_mount(DEFAULT_DISK_NAME);
    fd1 = tfs_open("file123");
    if (fd1 >= 0)
        printf("open (new file): success\n");
    else
        print_error(fd1);
    //printf("FD: %d\n", fd1);

    // open (already open file)
    fd1 = tfs_open("file123");
    if (fd1 >= 0)
        printf("open (already open file): success\n");
    else
    {
        printf("open (already open file): failure\n");
        print_error(fd1);
    }

    // open (open second file)
    fd2 = tfs_open("file248");
    if (fd2 >= 0)
        printf("open (open second file): success\n");
    else
    {
        printf("open (open second file): failure\n");
        print_error(fd2);
    }

    // write (single block)
    err = tfs_write(fd1, SMALLSTR, 50);
    if (err >= 0)
        printf("write (single block): success\n");
    else
    {
        printf("write (single block): failure\n");
        print_error(err);
    }

    // write (multiple blocks + expansion)
    err = tfs_write(fd1, VERYBIGSTR, 512);
    if (err >= 0)
        printf("write (multiple blocks + expansion): success\n");
    else
    {
        printf("write (multiple blocks + expansion): failure\n");
        print_error(err);
    }

    // write (empty buffer + contraction)
    err = tfs_write(fd1, "", 0);
    if (err >= 0)
        printf("write (empty buffer + contraction): success\n");
    else
    {
        printf("write (empty buffer + contraction): failure\n");
        print_error(err);
    }

    // write (closed file)
    tfs_close(fd1);
    err = tfs_write(fd1, SMALLSTR, 50);
    if (err >= 0)
        printf("write (closed file): failure\n");
    else
    {
        printf("write (closed file): success\n");
        print_error(err);
    }

    // open (previously closed file)
    fd1 = tfs_open("file123");
    if (fd1 >= 0)
        printf("open (previously closed file): success\n");
    else
        printf("open (previously closed file): failure\n");


    // readByte (normal operation, single block)
    tfs_write(fd1, SMALLSTR, 50);

    for (i = 0; i < 50; i++)
    {
        err = tfs_readByte(fd1, &buffer[i]);
        if (err < 0)
            print_error(err);
    }
    buffer[50] = '\0';
    if (!strcmp(buffer, SMALLSTR))
        printf("readByte (normal operation, single block): success\n");
    else
        printf("readByte (normal operation, single block): failure\n");


    // readByte (normal operation, multilple blocks)
    tfs_write(fd1, VERYBIGSTR, 512);
    for (i = 0; i < 512; i++)
    {
        err = tfs_readByte(fd1, &biggerbuf[i]);
        if (err < 0)
            print_error(err);
    }
    biggerbuf[512] = '\0';
    if (!strcmp(biggerbuf, VERYBIGSTR))
        printf("readByte (normal operation, multiple blocks): success\n");
    else
        printf("readByte (normal operation, multiple blocks): failure\n");


    // readByte (EOF)
    err = tfs_readByte(fd1, &buffer[50]);
    if (err >= 0)
        printf("readByte (EOF): failure\n");
    else
    {
        printf("readByte (EOF): success\n");
        print_error(err);
    }


    // readByte (invalid file descriptor)
    err = tfs_readByte(-1, &buffer[50]);
    if (err >= 0)
        printf("readByte (invalid file descriptor): failure\n");
    else
    {
        printf("readByte (invalid file descriptor): success\n");
        print_error(err);
    }


    // seek (normal operation)
    for (i = 0; i < 50; i++)
    {
        err = tfs_seek(fd1, i);
        if (err < 0)
        {
            print_error(err);
            break;
        }
        else
            err = tfs_readByte(fd1, &buffer[i]);
    }
    if (!strcmp(buffer, SMALLSTR))
        printf("seek (normal operation): success\n");
    else
        printf("seek (normal operation): failure\n");


    // seek (invalid offset)
    err = tfs_seek(fd1, 51);
    if (err >= 0)
        printf("seek (invalid offset): failure\n");
    else
    {
        printf("seek (invalid offset): success\n");
        print_error(err);
    }


    // seek (invalid file descriptor)
    err = tfs_seek(-1, 50);
    if (err >= 0)
        printf("seek (invalid file descriptor): failure\n");
    else
    {
        printf("seek (invalid file descriptor): success\n");
        print_error(err);
    }


    // close (single file)
    err = tfs_close(fd1);
     if (err >= 0)
        printf("close (single file): success\n");
    else
    {
        printf("close (single file): failure\n");
        print_error(err);
    }


    // close (previously closed file)
    err = tfs_close(fd1);
    if (err >= 0)
        printf("close (previously closed file): failure\n");
    else
    {
        printf("close (previously closed file): success\n");
        print_error(err);
    }
    

    // delete (first file)
    fd1 = tfs_open("file123");
    err = tfs_delete(fd1);
    if (err >= 0)
        printf("delete (first file): success\n");
    else
        printf("delete (first file): failure\n");


    // delete (second file)
    err = tfs_delete(fd2);
    if (err >= 0)
        printf("delete (second file): success\n");
    else
        printf("delete (second file): failure\n");


    // unmount (first disk)
    err = tfs_unmount();

    // mount (second disk)
    err = tfs_mount("SMALL_DISK");
    
    // populate disk until full
    fd1 = tfs_open("file1");
    fd2 = tfs_open("file2");
    fd3 = tfs_open("file3");
    // open (new file in almost full disk)
    if (fd3 >= 0)
        printf("open (new file in almost full disk): success\n");
    else
        printf("open (new file in almost full disk): failure\n");
    

    // open (create new file in full disk)
    fd4 = tfs_open("file4");
    if (fd4 >= 0)
        printf("open (fail to create new file in full disk): failure\n");
    else
    {
        printf("open (fail to create new file in full disk): success\n");
        print_error(fd4);
    }


    // print out all the files in the directory
    err = tfs_readdir();
    if (err >= 0)
        printf("read directory: success\n");
    else
    {
        printf("read directory: failure\n");
        print_error(err);
    }
    

    // delete (first file)
    err = tfs_delete(fd1);
    if (err >= 0)
        printf("delete (first file): success\n");
    else
        printf("delete (first file): failure\n");


    // open (reopen first file)
    fd1 = tfs_open("file1");
    if (fd1 >= 0)
        printf("open (reopen first file): success\n");
    else
        printf("open (reopen first file): failure\n");


    // delete (second file)
    err = tfs_delete(fd2);
    if (err >= 0)
        printf("delete (second file): success\n");
    else
    {
        printf("delete (second file): failure\n");
        print_error(err);
    }


    // delete (third file)
    err = tfs_delete(fd3);
    if (err >= 0)
        printf("delete (third file): success\n");
    else
    {
        printf("delete (third file): failure\n");
        print_error(err);
    }


    // delete (first file)
    err = tfs_delete(fd1);
    if (err >= 0)
        printf("delete (first file): success\n");
    else
        printf("delete (first file): failure\n");


    // delete (already deleted files/invalid file descriptor)
    err = tfs_delete(fd1);
    if (err >= 0)
        printf("delete (already deleted files): failure\n");
    else
    {
        printf("delete (already deleted files): success\n");
        print_error(err);
    }


    // open (reopen first  and second file)
    fd1 = tfs_open("file1");
    fd2 = tfs_open("file2");

    // stat (show time stamp)
    err = tfs_stat(fd1, creation_time, access_time, modification_time);
    if (err >= 0)
    {
        printf("stat (time): success\n");
        printf("\tFile creation time: %s", asctime(creation_time));
        printf("\tFile access time: %s", asctime(access_time));
        printf("\tFile modification time: %s", asctime(modification_time));
    }
    else
    {
        printf("stat (time): failure\n");
        print_error(err);
    }


    // write (verybigstr to first file)
    err = tfs_write(fd1, VERYBIGSTR, 512);
    if (err >= 0)
        printf("write (verybigstr to first file): failure\n");
    else
    {
        printf("write (verybigstr to first file): success\n");
        print_error(err);
    }


    // write (bigstr to first file)
    err = tfs_write(fd1, BIGSTR, 256);
    if (err >= 0)
        printf("write (bigstr to first file): success\n");
    else
    {
        printf("write (bigstr to first file): failure\n");
        print_error(err);
    }


    // write (empty first file)
    err = tfs_write(fd1, "", 0);
    if (err >= 0)
        printf("write (empty first file): success\n");
    else
    {
        printf("write (empty first file): failure\n");
        print_error(err);
    }


    // write (bigstr to second file)
    err = tfs_write(fd2, BIGSTR, 256);
    if (err >= 0)
        printf("write (bigstr to second file after emptying first file): success\n");
    else
    {
        printf("write (bigstr to second file after emptying first file): failure\n");
        print_error(err);
    }


    // unmount (second disk)
    err = tfs_unmount();
    if (err >= 0)
        printf("unmount (second disk): success\n");
    else
        printf("unmount (second disk): failure\n");


    // mount (second disk with data in it)
    err = tfs_mount("SMALL_DISK");
    if (err >= 0)
        printf("mount (second disk with data in it): success\n");
    else
        printf("mount (second disk with data in it): failure\n");


    // open (reopen first and second file)
    fd1 = tfs_open("file1");
    if (fd1 >= 0)
        printf("open (reopen first file): success\n");
    else
        printf("open (reopen first file): failure\n");


    fd2 = tfs_open("file2");
    if (fd2 >= 0)
        printf("open (reopen second file): success\n");
    else
        printf("open (reopen second file): failure\n");

    
    // readByte (after reopening second file)
    for (i = 0; i < 256; i++)
    {
        err = tfs_readByte(fd2, &bigbuf[i]);
        if (err < 0)
            print_error(err);
    }
    bigbuf[256] = '\0';
    if (!strcmp(bigbuf, BIGSTR))
        printf("readByte (after reopening second file): success\n");
    else
        printf("readByte (after reopening second file): failure\n");


    // rename (first file)
    err = tfs_rename(fd1, "renamed");
    if (err >= 0)
        printf("rename (first file): success\n");
    else
    {
        printf("rename (first file): failure\n");
        print_error(err);
    }


    // rename (file name too long)
    err = tfs_rename(fd1, "renamed_file");
    if (err >= 0)
        printf("rename (file name too long): failure\n");
    else
    {
        printf("rename (file name too long): success\n");
        print_error(err);
    }


    // rename (file name invalid)
    err = tfs_rename(fd3, "renamed");
    if (err >= 0)
        printf("rename (file name invalid): failure\n");
    else
    {
        printf("rename (file name invalid): success\n");
        print_error(err);
    }
    

    // stat (time)
    err = tfs_stat(fd1, creation_time, access_time, modification_time);
    if (err >= 0)
    {
        printf("stat (time): success\n");
        printf("\tFile creation time: %s", asctime(creation_time));
        printf("\tFile access time: %s", asctime(access_time));
        printf("\tFile modification time: %s", asctime(modification_time));
    }
    else
    {
        printf("stat (time): failure\n");
        print_error(err);
    }

    // delete (renamed file)
    err = tfs_delete(fd1);

    // delete (file 2)
    err = tfs_delete(fd2);

    // unmount (second disk)
    err = tfs_unmount();


    // // mkfs (resize to max disk size)
    // err = tfs_mkfs("SMALL_DISK", MAX_DISK_SIZE);
    // if (err >= 0)
    //     printf("mkfs (resize to max disk size): success\n");
    // else
    //     printf("mkfs (resize to max disk size): failure\n");
    
    // // mount "small disk"
    // err = tfs_mount("SMALL_DISK");

    // // populate the entire disk with new empty files
    // char filenames[254][8];
    // for (i = 0; i < 254; i++)
    // {
    //     memcpy(filenames[i], &VERYBIGSTR[i], 8);
    //     fd1 = tfs_open(filenames[i]);
    //     if (fd1 < 0)
    //         print_error(fd1);
    // }

    // // mkfs (resize to max disk size again)
    // err = tfs_mkfs("SMALL_DISK", MAX_DISK_SIZE);
    // if (err >= 0)
    //     printf("mkfs (resize to max disk size again): success\n");
    // else
    //     printf("mkfs (resize to max disk size again): failure\n");

    // char massiveFile[256 * BLOCKSIZE * 253]
    // // open one file and populate the entire disk with that file
    // fd1 = tfs_open("file1");


    // free all the stuff
    free(creation_time);
    free(access_time);
    free(modification_time);
    free_all();
    
    return 0;
}

void print_error(int errorCode)  {
    char* message = errorMessage[(-1 * errorCode) - 1];
    printf("\t%s\n", message);
}