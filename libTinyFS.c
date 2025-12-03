/*
*
* libTinyFS.c : actual tinyfs implementation
*
*/

#include "libTinyFS.h"
#include "libDisk.h"
#include "TinyFS_errno.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "blocktypes.h"

//maximum open files at a time
#define MAX_OPEN_FILES 20

typedef struct open_file {
	int inUse;	        //1 if this entry is in use, 0 otherwise
	int inodeBlock;		//block number where the inode is stored
	int filePointer; 	// current read/write position in file
	char name[9];       //name of the file
} OpenFileEntry;

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

	// new: initialize root inode timestamps
	time_t now = time(NULL);
	rin.ctime = (uint32_t)now;
	rin.mtime = (uint32_t)now;
	rin.atime = (uint32_t)now;

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
	//initialize the open files table
	initOpenFilesTable();
	return TFS_SUCCESS;
}

int tfs_unmount(void) {
	if(disk_no == -1) return ERR_NOT_MOUNTED;
	if(closeDisk(disk_no) != TFS_SUCCESS) return ERR_DISK_CLOSE; 
	disk_no = -1;
	return TFS_SUCCESS;
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

static int findInodeByName(const char *name) {
    if (disk_no < 0 || !name) return -1;
    inode_disk inode;
    int blockNum = 2; // start searching from block 2
    while (1) {
        if (readBlock(disk_no, blockNum, &inode) != TFS_SUCCESS) {
            break;
        }
        if (inode.blocktype == INODE && inode.magic == MAGIC) {
            if (strncmp(inode.name, name, 8) == 0) {
                return blockNum;
            }
        }
        blockNum++;
        if (blockNum > 1000) break;
    }
    return -1;
}

static int findOrCreateInode(const char *name) {
    // first try and find existing
    int existing = findInodeByName(name);
    if (existing > 0) {
        return existing;
    }
    // if not found allocate new block
    int newBlock = allocate_free_block();
    if (newBlock < 0) {
        return -1;
    }
    //initialize new Inode
    inode_disk newInode = {0};
    newInode.blocktype = INODE;
    newInode.magic = MAGIC;
    strncpy(newInode.name, name, 8);
    newInode.name[8] = '\0';
    newInode.size_B = 0;
    newInode.blk_start = 0;
    newInode.metaflags = INODE_INITIAL_FLAGS;
    if (writeBlock(disk_no, newBlock, &newInode) != TFS_SUCCESS) {
        free_block(newBlock);
        return -1;
    }
    return newBlock;
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



int tfs_closeFile(fileDescriptor FD) {
    if (disk_no == -1) return ERR_NOT_MOUNTED;
    if (!isValidFD(FD)) return ERR_FD_INVALID;//clear resource table entry 
    openFiles[FD].inUse = 0;
    openFiles[FD].inodeBlock = -1;
    openFiles[FD].filePointer = 0;
    openFiles[FD].name[0] = '\0';
    return TFS_SUCCESS;
}

//returns a block number that you can do anything with (removes it from the free list)
int allocate_free_block() {
	if(disk_no == -1) return ERR_NOT_MOUNTED;
	superblock_disk sb = {0};
	if (readBlock(disk_no, SUPERBLOCK_BLOCK, &sb) != TFS_SUCCESS) return ERR_DISK_READ;
	if (sb.free_block == 0) return ERR_FS_FULL; //use 0 not -1 

	int block = sb.free_block; //first in pointer
	free_disk freedisk = {0};
	if(readBlock(disk_no, block, &freedisk) != TFS_SUCCESS) return ERR_DISK_READ;
	sb.free_block = freedisk.blk_next;

	//writeback
	if (writeBlock(disk_no, SUPERBLOCK_BLOCK, &sb) != TFS_SUCCESS) return ERR_DISK_WRITE;	
	return block;
}

//once done with a block free it and it goes back onto the free linkedlist
int free_block(int block) {
	if(disk_no == -1) return ERR_NOT_MOUNTED;
	superblock_disk sb = {0};
	if (readBlock(disk_no, SUPERBLOCK_BLOCK, &sb) != TFS_SUCCESS) return ERR_DISK_READ;
	
	free_disk fb = {0};
	fb.blocktype = FREE;
  	fb.magic = MAGIC;
 	fb.blk_next = sb.free_block;	
	//FIRST write the free block, if successful then change the superblock (it isn't like we're allowing errors)

	if(writeBlock(disk_no, block, &fb) != TFS_SUCCESS) return ERR_DISK_WRITE;
	//written free block.
	
	sb.free_block = block; 
	if(writeBlock(disk_no, SUPERBLOCK_BLOCK, &sb) != TFS_SUCCESS) return ERR_DISK_WRITE;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
    if (disk_no == -1) return ERR_NOT_MOUNTED;
    if (!isValidFD(FD)) return ERR_FD_INVALID;
    if (!buffer && size > 0) return ERR_DISK_WRITE;
    int inodeBlock = openFiles[FD].inodeBlock;
    // free existing data blocks
    inode_disk inode;
    if (readBlock(disk_no, inodeBlock, &inode) != TFS_SUCCESS) {
        return ERR_DISK_READ;
    }
    // free all data blocks
    int currentBlock = inode.blk_start;
    while (currentBlock != 0) {
        fileextent_disk extent;
        if (readBlock(disk_no, currentBlock, &extent) != TFS_SUCCESS) {
            break;
        }
        int nextBlock = extent.blk_next;
        free_block(currentBlock);
        currentBlock = nextBlock;
	}
    // if size is 0, just update inode and return
    if (size == 0) {
        inode.blk_start = 0;
        inode.size_B = 0;
        writeBlock(disk_no, inodeBlock, &inode);
        openFiles[FD].filePointer = 0;
        return TFS_SUCCESS;
    }
    // calculate number of blocks needed
    int dataPerBlock = EX_E;
    int blocksNeeded = (size + dataPerBlock - 1) / dataPerBlock;
    
    // allocate required blocks
    int *blocks = malloc(blocksNeeded * sizeof(int));
    if (!blocks) return ERR_DISK_WRITE;
    for (int i = 0; i < blocksNeeded; i++) {
        blocks[i] = allocate_free_block();
        if (blocks[i] < 0) {
            // allocation failed, free already allocated blocks
            for (int j = 0; j < i; j++) {
                free_block(blocks[j]);
            }
            free(blocks);
            return ERR_DISK_FULL;
        }
    }
    // write data into allocated blocks
    int bytesWritten = 0;
    for (int i = 0; i < blocksNeeded; i++) {
        fileextent_disk extent = {0};
        extent.blocktype = FILEEXTENT;
        extent.magic = MAGIC;
        extent.blk_next = (i < blocksNeeded - 1) ? blocks[i + 1] : 0;
        int bytesToWrite = size - bytesWritten;
        if (bytesToWrite > dataPerBlock) {
            bytesToWrite = dataPerBlock;
        }
        memcpy(extent.data, buffer + bytesWritten, bytesToWrite);
        bytesWritten += bytesToWrite;
        if (writeBlock(disk_no, blocks[i], &extent) != TFS_SUCCESS) {
            free(blocks);
            return ERR_DISK_WRITE;
        }
    }
	// Update inode
    inode.blk_start = blocks[0];
    inode.size_B = size;
    if (writeBlock(disk_no, inodeBlock, &inode) != TFS_SUCCESS) {
        free(blocks);
        return ERR_DISK_WRITE;
    }
    free(blocks);
    openFiles[FD].filePointer = 0;
    return TFS_SUCCESS;
}

int tfs_deleteFile(fileDescriptor FD) {
    if (disk_no == -1) return ERR_NOT_MOUNTED;
    if (!isValidFD(FD)) return ERR_FD_INVALID;

    int inodeBlock = openFiles[FD].inodeBlock;

    // read the inode to get the data block chain
    inode_disk inode;
    if (readBlock(disk_no, inodeBlock, &inode) != TFS_SUCCESS) {
        return ERR_DISK_READ;
    }

    // free all data blocks
    int currentBlock = inode.blk_start;
    while (currentBlock != 0) {
        fileextent_disk extent;
        if (readBlock(disk_no, currentBlock, &extent) != TFS_SUCCESS) {
            break;  // don't leak blocks because of a read error, but bail
        }
        int nextBlock = extent.blk_next;
        free_block(currentBlock);
        currentBlock = nextBlock;
    }

    // free the inode block itself
    free_block(inodeBlock);

    // clear resource table entry
    openFiles[FD].inUse = 0;
    openFiles[FD].inodeBlock = -1;
    openFiles[FD].filePointer = 0;
    openFiles[FD].name[0] = '\0';

    return TFS_SUCCESS;
}

int tfs_seek(fileDescriptor FD, int offset) {
    if (disk_no == -1) return ERR_NOT_MOUNTED;
    if (!isValidFD(FD)) return ERR_FD_INVALID;
    if (offset < 0) return ERR_SEEK;

    // read inode to get file size
    inode_disk inode;
    if (readBlock(disk_no, openFiles[FD].inodeBlock, &inode) != TFS_SUCCESS) {
        return ERR_DISK_READ;
    }

    // check if offset is within file size
    if (offset > (int)inode.size_B) {
        return ERR_SEEK;
    }

    // set file pointer to new offset
    openFiles[FD].filePointer = offset;
    return TFS_SUCCESS;
}

/* Helper: load inode given a fileDescriptor (uses openFiles table) */
static int load_inode_from_fd(fileDescriptor FD, inode_disk *inodeOut, int *blkNumOut) {
    if (disk_no == -1) return ERR_NOT_MOUNTED;
    if (!isValidFD(FD)) return ERR_FD_INVALID;
    if (!inodeOut) return ERR_FS_INVALID;

    int inodeBlock = openFiles[FD].inodeBlock;

    if (readBlock(disk_no, inodeBlock, inodeOut) != TFS_SUCCESS)
        return ERR_DISK_READ;

    if (inodeOut->blocktype != INODE || inodeOut->magic != MAGIC)
        return ERR_FS_INVALID;

    if (blkNumOut) *blkNumOut = inodeBlock;
    return TFS_SUCCESS;
}

int tfs_rename(fileDescriptor FD, char *newName) {
    if (disk_no == -1) return ERR_NOT_MOUNTED;
    if (!isValidFD(FD)) return ERR_FD_INVALID;
    if (!newName) return ERR_FILE_NAME;

    size_t len = strlen(newName);
    if (len == 0 || len > 8) return ERR_FILE_NAME; // keep same limit as tfs_openFile

    inode_disk inode;
    int inodeBlock;
    int rc = load_inode_from_fd(FD, &inode, &inodeBlock);
    if (rc < 0) return rc;

    // copy new name into fixed array
    memset(inode.name, 0, sizeof(inode.name));
    memcpy(inode.name, newName, len);

    // update timestamps: metadata change
    time_t now = time(NULL);
    inode.mtime = (uint32_t)now;
    inode.atime = (uint32_t)now;

    if (writeBlock(disk_no, inodeBlock, &inode) != TFS_SUCCESS)
        return ERR_DISK_WRITE;

    // keep resource table in sync with the inode
    strncpy(openFiles[FD].name, newName, 8);
    openFiles[FD].name[8] = '\0';

    return TFS_SUCCESS;
}

int tfs_readdir(void) {
    if (disk_no == -1) return ERR_NOT_MOUNTED;
    
    inode_disk inode;
    int blk = 0;
    int first = 1;

    printf("TinyFS directory listing:\n");

    while (1) {
	int rc = readBlock(disk_no, blk, &inode);
	if (rc != TFS_SUCCESS) break; // assume this means "no more blocks"

	if (inode.blocktype == INODE && inode.magic == MAGIC) {

	    // skip completely / unused inode slots if you mark them that way
	    if (inode.name[0] == '\0' && inode.size_B == 0 && blk != ROOT_INODE_BLOCK) {
		blk++;
		continue;
	    }

	    char nameBuf[10] = {0};
	    strncpy(nameBuf, inode.name, 9);
	    nameBuf[9] = '\0';

	    if (first) {
		first = 0;
	    }

	    printf("  block %2d  %-9s  %u bytes\n",
		   blk, nameBuf, (unsigned)inode.size_B);

	}

	blk++;


    if (first) {
        printf("  [no files]\n");
    }

    return TFS_SUCCESS;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
	if(!disk_no) return ERR_NOT_MOUNTED;
	if(!isValidFD(FD)) return ERR_FD_INVALID;
	if(!buffer) return ERR_BUF;

	int in_block = openFiles[FD].inodeBlock;
	struct inode_disk in = {0};
	if (readBlock(disk_no, in_block, &in) != TFS_SUCCESS) {
		return ERR_DISK_READ;
	}
	//now we have the inode block; get pointer
	int fp = openFiles[FD].filePointer;
	if(fp >= in.size_B) {
		//at or beyond the eof
		return ERR_EOF;	
	}
	//figure out where to read
	// byte --> disk read and offset
	// Remember that each file extent only holds EX_E bytes (250?)
	int extent_i = fp / EX_E;
	int extent_off = fp % EX_E;
	struct fileextent_disk fext = {0};
	int node_block = in.blk_start;
	//start at inode head; walk linkedlist.
	for(int i = 0; i < extent_i; i++){
		if(node_block <= 0) { return ERR_FS_INVALID; }
		if(readBlock(disk_no, node_block, &fext) != TFS_SUCCESS) {
			return ERR_DISK_READ;
		}
		node_block = fext.blk_next; 
	}	
	//node_block is now the extent_ith block
	if(readBlock(disk_no, node_block, &fext) != TFS_SUCCESS) {
		return ERR_DISK_READ;
	}
	*buffer = ext.data[extent_off];
	//already compensatas for the struct offset.
	///as per pdf
	openFiles[FD].filePointer = fp + 1;
	return TFS_SUCCEESS;
}

int readFileInfo(fielDescripto FD, tfsFileInfo *out) {
    if (disk_no == -1) return ERR_NOT_MOUNTED;
    if (!isValidFD(FD)) return ERR_FD_INVALID;
    if (!out) return ERR_FS_INVALID;

    inode_disk inode;
    int inodeBlock;
    int rc = load_inode_from_fd(FD, &inode, &inodeBlock);
    if (rc < 0) return rc;

    memset(out, 0, sizeof(*out));

    // copy name
    strncpy(out->name, inode.name, 8);
    out->name[8] = '\0';

    out->size_B     = (int)inode.size_B;
    out->ctime      = (time_t)inode.ctime;
    out->mtime      = (time_t)inode.mtime;
    out->atime      = (time_t)inode.atime;
    out->inodeBlock = inodeBlock;

    return TFS_SUCCESS;
}
