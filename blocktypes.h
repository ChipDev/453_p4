/*
 * blocktypes.h, project 4, header file that contains the 4 block types
 * eahc 256 bytes and in standard C typedef struct format
 * to make it modular and easy to digest and debug.
*/

typedef enum {
	SUPERBLOCK = 1,
	INODE = 2,
	FILEEXTENT = 3,
	FREE = 4
} blocktype;
