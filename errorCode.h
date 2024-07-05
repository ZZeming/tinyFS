#define MALLOC_ERR -1
#define INVALID_OP -2
#define LSEEK_ERR -3
#define OPEN_ERR -4
#define WRITE_ERR -5
#define READ_ERR -6
#define CLOSE_ERR -7
#define INVALID_DISK -8
#define DISK_FULL -9
#define NO_FD -10
#define EOF_ERR -11


#define MALLOC_MESS "Memory allocation error"
#define INVALID_OP_MESS "Invalid operation"
#define LSEEK_MESS "Invalid file descriptor"
#define OPEN_MESS "Failed to open file"
#define WRITE_MESS "Failed to write to file"
#define READ_MESS "Failed to read file"
#define CLOSE_MESS "Failed to close file"
#define INVALID_DISK_MESS "Disk format is invalid"
#define DISK_FULL_MESS "Disk is full"
#define NO_FD_MESS "File not open or file does not exist"
#define EOF_MESS "Can't read beyond EOF"

static const int errorCodes[11] =
{
    MALLOC_ERR,     //index 0
    INVALID_OP,     //index 1
    LSEEK_ERR,      //index 2
    OPEN_ERR,       //index 3
    WRITE_ERR,      //index 4
    READ_ERR,       //index 5
    CLOSE_ERR,      //index 6
    INVALID_DISK,   //index 7
    DISK_FULL,      //index 8
    NO_FD,          //index 9
    EOF_ERR         //index 10
};

static char* errorMessage[11] =
{
    MALLOC_MESS,        //index 0
    INVALID_OP_MESS,    //index 1
    LSEEK_MESS,         //index 2
    OPEN_MESS,          //index 3
    WRITE_MESS,         //index 4
    READ_MESS,          //index 5
    CLOSE_MESS,         //index 6
    INVALID_DISK_MESS,  //index 7
    DISK_FULL_MESS,     //index 8
    NO_FD_MESS,         //index 9
    EOF_MESS            //index 10
};

void print_error(int errorCode);