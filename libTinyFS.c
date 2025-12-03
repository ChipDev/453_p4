/*
*
* libTinyFS.c : actual tinyfs implementation
*
*/

#include "libTinyFS.h"
#include "libDisk.h"
#include "TinyFS_errno.h"
#include <string.h>

int tfs_mkfs(char *filename, int nBytes) {
	if(!filename) return ERR_FS_INVALID
	if(strlen(filename) == 0 || strlen(filename) > 8) return ERR_FS_INVALID
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
	sb.blocktype = blocktype.SUPERBLOCK;
	sb.magic = 0x44;
	sb.root_inode = ROOT_INODE_BLOCK;	// block 1 = root inode
	sb.free_block = 2;			// block 2 = free (first).

	if (writeBlock(disk, SUPERBLOCK_BLOCK, &sb) != TFS_SUCCESS) {
        	closeDisk(disk);
		return ERR_DISK_WRITE;
	}
	
	struct inode_disk rin = {0};
	rin.blocktype = blocktype.INODE;
	rin.magic = MAGIC;
	strcpy(in.name, "/");
	rin.size_B = nb;
	rin.blk_start = -1;	//initially - 1 if no data
	rin.metaflags = INODE_INITIAL_FLAGS; 

	if(writeBlock(disk, ROOT_INODE_BLOCK, &rin) != TFS_SUCCESS) {
		closeDisk(disk); 
		return ERR_DISK_WRITE;
	}
	//fill the rest with free blocks
	int blocks = nb / BLOCKSIZE;
	if(blocks < 3) { printf("What do do in this situation??\n"); return -1; }

	for(int i = 2; i < blocks; i++) {
		printf("setting block %d to free\n", i);
		struct free_disk free = {0};
		free.blocktype = blocktype.FREE;
		free.magic = MAGIC;
		free.blk_next = (i != blocks - 1) ? i + 1 : 0;	//quickly sets up the rest as free, last -> 0
		if(writeBlock(disk, i, &free) != TFS_SUCCESS) {
			closeDisk(disk);
			return ERR_DISK_WRITE;
		}		
	} 
}
