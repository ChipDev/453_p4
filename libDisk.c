/*
*
* libDisk.c implementation
*
*/

#include "libDisk.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

#define ALLOC_DISKS 10

typedef struct {
	uint8_t flags; // (literally for now this can just be a 1 if used
	int nBytes;
	int fd;
	int nBlocks; //should map cleanly but from what I read good practice 
} disk_entry;

static disk_entry disks[ALLOC_DISKS] = {0}

static int isOpen(int disk) {
	if(disk >= 0 && disk < ALLOC_DISKS && disks[disk].flags) {
		return 1;
	}else{
		return 0;
	}
}

// not part of the api, used interally
static int next_free_disk() {
	for(int i = 0; i < ALLOC_DISKS; i++) {
		//loop thru all the structs, its ok that they arent initialized.
		if( disks[i].flags == 0 ) { 
			//flags == 0 guarantees not in use, this might change.
			return i;			
		}	
	}
	//no disk available
	return DISK_ALLOC_ERROR;
} 

int openDisk(char *filename, int nBytes) {
	int bs = nBytes;
	if(bs < 0) return OPEN_DISK_PARAM_ERR;
	if(bs == 0) {
		//open disk (happens after if elif)
	}
	else if(bs > 0 && bs < BLOCKSIZE) {
		return OPEN_DISK_PARAM_ERR;
	}
	else if(bs > BLOCKSIZE && (bs % BLOCKSIZE) != 0) {
		bs -= (bs % BLOCKSIZE);
	}	
	//blocksize is now either 0 or a multiple of blocksize 
	// if 0 : open the disk
	// if nonzero : overwrite 
	int fd = -1;
	int diskn = next_free_disk();
	if(diskn < 0) return DISK_ALLOC_ERROR;
	if(bs == 0) {
		fd = open(filename, O_RDWR);
		struct stat st;
		if(fd < 0 ) {
			return OPEN_DISK_FILE_ERR;	
		}
		if(fstat(fd, &st) < 0){
			close(fd);
			return OPEN_DISK_FILE_ERR;
		}
		disks[diskn].flags = 1;
		disks[diskn].fd = fd; 
		disks[diskn].nBytes = st.st_size - (st.st_size % BLOCKSIZE);
		disks[diskn].nBlocks = disks[diskn].nBytes / BLOCKSIZE;
	}
	if(bs != 0) {
		fd = open(filename, O_RDWR | O_CREAT, 0666); //0 666 octal is default for files
	//	struct stat st;
		if(fd < 0) {
			return OPEN_DISK_FILE_ERR;
		}
		//ftruncate changes the file size, doens't just decrease
		if(ftruncate(fd, bs) < 0) {
			close(fd);
			return OPEN_DISK_FILE_ERR;
		}
		disks[diskn].flags = 1;
		disks[diskn].fd = fd;
		disks[diskn].nBytes = bs;
		disks[diskn].nBlocks = bs / BLOCKSIZE;
	}
	return diskn;
}

int closeDisk(int diskn) {
	if(disks[diskn].flags) {
		//disks[diskn] = {0};
		//disks[diskn].fd = -1;
		if(close(disk[diskn].fd) != 0) {
			return DISK_CLOSE_ERR;
		}
		disks[diskn] = {0};
		disks[diskn].fd = -1;
		return 0;
	}else{
		return DISK_NOT_OPEN;
	}
}

int readBlock(int disk, int bNum, void *block) {
	if(!isOpen(disk)) return DISK_NOT_OPEN;
	if(!block) return BUF_NULL;
	//lseek uses off_t, is this just a uint64? idk
	off_t offset = bNum * BLOCKSIZE;
	if(lseek(disks[disk].fd, offset, SEEK_SET) < 0) return DISK_IO_ERR;
	int read_b = 0;
	if((read_b = read(disks[disk].fd, block, BLOCKSIZE)) != BLOCKSIZE) {
		printf("DEBUG !! Read different number than BLOCKSIZE: %d\n", read_b);
		return DISK_IO_ERR;
	}
	//good
	return 0;
}

int writeBlock(int disk, int bNum, void *block) {
	if(!isOpen(disk)) return DISK_NOT_OPEN;
	if(!block) return BUF_NULL;
	off_t offset = bNum * BLOCKSIZE;
	if(lseek(disks[disk].fd, offset, SEEK_SET) < 0) return DISK_IO_ERR;
	//can do a repeat write at different offsets, should be good for now
	// writeBlock only writes 1 block !!
	int wrote = write(disks[disk].fd, block, BLOCKSIZE);
	if(wrote < 0) { return DISK_IO_ERR; }
	if(wrote != BLOCKSIZE) { printf("DEBUG !! Wrote %d not %d blocksize", wrote, BLOCKSIZE); }
	return 0; 
}
