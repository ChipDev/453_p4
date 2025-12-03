/*
 * blocktypes.h, project 4, header file that contains the 4 block types
 * eahc 256 bytes and in standard C typedef struct format
 * to make it modular and easy to digest and debug.
*/
#ifndef BLOCKTYPES_H
#define BLOCKTYPES_H
#include <stdint.h>

#define INODE_INITIAL_FLAGS 1 //Maybe something like USED
#define SUPERBLOCK_BLOCK 0
#define ROOT_INODE_BLOCK 1
#define MAGIC 0x44
#define SB_E (256 - 1 - 1 - 4 - 4) 
#define IN_E (256 - 1 - 1 - 9 - 4 -4 - 1)
#define EX_E (256 - 1 - 1 - 4)
#define FR_E (256 - 1 - 1 - 4)
typedef enum {
	SUPERBLOCK = 1,
	INODE = 2,
	FILEEXTENT = 3,
	FREE = 4
} blocktype;

typedef struct superblock_disk {
	uint8_t blocktype;	// byte	0	: SUPERBLOCK (1)
	uint8_t magic;		// byte 1	: MAGIC (0x44)
	int32_t root_inode;	// byte 2:5	: root inode of fs
	int32_t free_block;	// byte 6:9	: block # of first free index
	uint8_t empty[SB_E];	// byte 10:255 : reserved
} superblock_disk;

typedef struct inode_disk{
	uint8_t blocktype;	//byte 0	: INODE (2)
	uint8_t magic;		//byte 1	: MAGIC (0x44)
	char	 name[9];	//byte 2-10	: NAME [8 chars + guaranteed \0] 
	int32_t size_B;		//byte 11-14	: SIZE bytes
	int32_t blk_start;	//byte 15-18	: BLOCK of initial data (can be -1)
	uint8_t metaflags;	//byte 19	: FLAGS (extra metdata tbd)
	uint8_t empty[IN_E];
} inode_disk; 

typedef struct fileextent_disk {
	uint8_t blocktype;	//byte 0 	: FILEEXTENT (3)
	uint8_t magic;		//byte 1	: MAGIC 0x44
	uint32_t blk_next; 	//byte 2-5	: BLOCK of next data (0 = nothing)
	uint8_t data[EX_E];	//byte 6-255	: DATA 
} fileextent_disk;

typedef struct free_disk {
	uint8_t blocktype; 	//byte 0	: FREE (4)
	uint8_t magic;		//byte 1 	: MAGIC
	int32_t blk_next; 	//byte 2-5 	: BLOCK of next free data (can be -1)
	uint8_t empty[FR_E];	//byte 6-255 	: EMPT
} free_disk;	

#endif
