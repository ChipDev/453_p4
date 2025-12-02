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

#define ALLOC_DISKS 10

typedef struct {
	uint8_t flags; // (literally for now this can just be a 1 if used
	int nBytes;
	int fd;
	int nBlocks; //should map cleanly but from what I read good practice 
} disk_entry;

static disk_entry disks[ALLOC_DISKS] = {NULL}


