/*
*
* libTinyFS.c : actual tinyfs implementation
*
*/

#include "libTinyFS.h"
#include "libDisk.h"
#include "TinyFS_errno.h"
#include <string.h>
#include "blocktypes.h"

//maximum open files at a time
#define MAX_OPEN_FILES 20

typedef struct open_file {
	int inUse;	        //1 if this entry is in use, 0 otherwise
	int inodeBlock;		//block number where the inode is stored
	int in_use;		    // current read/write position in file
	char name[9];       //name of the file
} Open FileEntry;

//static resource table
static OpenFileEntry openFiles[MAX_OPEN_FILES];

//Only a single disk may be mounted at a time.
static int disk_no = -1;

int tfs_mkfs(char *filename, int nBytes) {
	if(!filename) return ERR_FS_NAME;
	if(strlen(filename) == 0 || strlen(filename) > 8) return ERR_FS_NAME;
	// block 0: superblock
	// block 1: root inode
	int nb = nBytes;
	nb -= (nb % BLOCKSIZE);
	//nb is now a multiple of BLOCKSIZE
	//now use the libDisk
	int disk = openDisk(filename, nBytes);
	if(disk < 0) { return ERR_DISK_OPEN; }
	//disk is now open, write to it. Remember that a disk is just a black box (file)
	
	//write block 0 (superblck) and 1 (root inode)
	struct superblock_disk sb = {0};
	sb.blocktype = SUPERBLOCK;
	sb.magic = 0x44;
	sb.root_inode = ROOT_INODE_BLOCK;	// block 1 = root inode
	sb.free_block = 2;			// block 2 = free (first).

	if (writeBlock(disk, SUPERBLOCK_BLOCK, &sb) != TFS_SUCCESS) {
        	closeDisk(disk);
		return ERR_DISK_WRITE;
	}
	
	struct inode_disk rin = {0};
	rin.blocktype = INODE;
	rin.magic = MAGIC;
	strcpy(rin.name, "/");
	rin.size_B = 0;
	rin.blk_start = 0;	//we're using 0 now instead of -1
	rin.metaflags = INODE_INITIAL_FLAGS; 

	if(writeBlock(disk, ROOT_INODE_BLOCK, &rin) != TFS_SUCCESS) {
		closeDisk(disk); 
		return ERR_DISK_WRITE;
	}
	//fill the rest with free blocks
	int blocks = nb / BLOCKSIZE;
	if(blocks < 3) { printf("What do do in this situation??\n"); closeDisk(disk); return -1; }

	for(int i = 2; i < blocks; i++) {
		printf("setting block %d to free\n", i);
		struct free_disk free = {0};
		free.blocktype = FREE;
		free.magic = MAGIC;
		free.blk_next = (i != blocks - 1) ? i + 1 : 0;	//quickly sets up the rest as free, last -> 0
		if(writeBlock(disk, i, &free) != TFS_SUCCESS) {
			closeDisk(disk);
			return ERR_DISK_WRITE;
		}		
	} 
	return TFS_SUCCESS;
}

int tfs_mount(char *diskname){
	if(disk_no != -1) return ERR_ALREADY_MOUNTED;
	int disk_attempt_open = openDisk(diskname, 0); //dont overwrite.
	if(disk_attempt_open < 0) { return ERR_DISK_OPEN; }
	disk_no = disk_attempt_open;
	superblock_disk sb = {0};
	int validate = readBlock(disk_no, 0, &sb);
	if(validate < 0) { 
		closeDisk(disk_no);
		disk_no = -1; 
		return ERR_DISK_READ; 	
	}
	if(sb.magic != MAGIC || sb.blocktype != SUPERBLOCK) {
		closeDisk(disk_no);
		disk_no = -1;
		return ERR_FS_INVALID;
	}
	return TFS_SUCCESS;
}

int tfs_unmount(void) {
	if(disk_no == -1) return ERR_NOT_MOUNTED;
	if(closeDisk(disk_no) != TFS_SUCCESS) return ERR_DISK_CLOSE; 
	disk_no = -1;
	return TFS_SUCCESS;
}


fileDescriptor tfs_openFile(char *name) {
    if (disk_no == -1) return ERR_NOT_MOUNTED;
    if (!name) return ERR_FILE_NAME;
    if (strlen(name) == 0 || strlen(name) > 8) return ERR_FILE_NAME;
    // check if file is already open in the resource table 
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (openFiles[i].inUse && strncmp(openFiles[i].name, name, 8) == 0) {
            //if file open return existing fd
            return i;
        }
    }// find free slot in the resource table 
    int fd = findFreeFileSlot();
    if (fd < 0) {
        return ERR_FD_INVALID; //too many open files
    }//fine or create the inode for the file
    int inodeBlock = findOrCreateInode(name);
    if (inodeBlock < 0) {
        return ERR_DISK_FULL;
    } //set up the resource table entry
    openFiles[fd].inUse = 1;
    openFiles[fd].inodeBlock = inodeBlock;
    openFiles[fd].filePointer = 0;
    strncpy(openFiles[fd].name, name, 8);
    openFiles[fd].name[8] = '\0';
    return fd;
}

// helper that initialize the open files table 
static void initOpenFilesTable(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        openFiles[i].inUse = 0;
        openFiles[i].inodeBlock = -1;
        openFiles[i].filePointer = 0;
        openFiles[i].name[0] = '\0';
    }
}

// helper that find a free slot in the open files table */
static int findFreeFileSlot(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!openFiles[i].inUse) {
            return i;
        }
    }
    return -1;
}

// helper that checks if a fd is valid
static int isValidFD(fileDescriptor FD) {
    if (FD < 0 || FD >= MAX_OPEN_FILES) return 0;
    if (!openFiles[FD].inUse) return 0;
    return 1;
}


int tfs_closeFile(fileDescriptor FD) {
    if (disk_no == -1) return ERR_NOT_MOUNTED;
    if (!isValidFD(FD)) return ERR_FD_INVALID;//clear resource table entry 
    openFiles[FD].inUse = 0;
    openFiles[FD].inodeBlock = -1;
    openFiles[FD].filePointer = 0;
    openFiles[FD].name[0] = '\0';
    return TFS_SUCCESS;
}