/* 
*
* TinyFS errno header 
*
*/ 

#ifndef TINYFS_ERRNO_H
#define TINYFS_ERRNO_H

#define TFS_SUCCESS 0
#define ERR_DISK_OPEN -1
#define ERR_DISK_READ -2
#define ERR_DISK_WRITE -3
#define ERR_DISK_CLOSE -4
#define ERR_DISK_INVALID -5
#define ERR_BLOCK_INVALID -6
#define ERR_NOT_MOUNTED -7
#define ERR_ALREADY_MOUNTED -8
#define ERR_FS_INVALID -9
#define ERR_FILE_NOT_FOUND -10
#define ERR_FILE_EXISTS -11
#define ERR_FILE_NAME -12
#define ERR_FD_INVALID -13
#define ERR_EOF -14
#define ERR_SEEK -15
#define ERR_DISK_FULL -16
#define ERR_FS_NAME -17
#endif
