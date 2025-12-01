/*
 * blocktypes.h, project 4, header file that contains the 4 block types
 * eahc 256 bytes and in standard C typedef struct format
 * to make it modular and easy to digest and debug.
*/

#include <stdint.h>

#define MAGIC 0x44
#define SB_E (256 - 1 - 1 - 4 - 4) 

typedef enum {
	SUPERBLOCK = 1,
	INODE = 2,
	FILEEXTENT = 3,
	FREE = 4
} blocktype;

typedef struct {
	uint8_t blocktype;	// byte	0	: SUPERBLOCK (1)
	uint8_t magic;		// byte 1	: MAGIC (0x44)
	int32_t root_inode;	// byte 2:5	: root inode of fs
	int32_t free_list;	// byte 6:9	: block # of free list
	uint8_t empty[SB_E];	// byte 10:255 : reserved
} superblock_disk;

