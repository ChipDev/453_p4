// test_mkfs.c
#include <stdio.h>
#include <string.h>

#include "libDisk.h"       // openDisk, readBlock, closeDisk, BLOCKSIZE
#include "libTinyFS.h"     // tfs_mkfs prototype
#include "blocktypes.h"    // superblock_disk, inode_disk, free_disk, MAGIC, SUPERBLOCK, FREE, etc.
#include "TinyFS_errno.h"  // TFS_SUCCESS, error codes

int main(void)
{
    const char *filename = "test.fs";

    // Pick a size that gives us several blocks
    int nBytes = 16 * BLOCKSIZE;   // 16 blocks worth of space

    printf("[TEST] tfs_mkfs(\"%s\", %d)\n", filename, nBytes);

    int rc = tfs_mkfs((char *)filename, nBytes);
    if (rc != TFS_SUCCESS) {
        printf("[FAIL] tfs_mkfs returned %d (expected %d)\n", rc, TFS_SUCCESS);
        return 1;
    }

    // Mirror mkfs rounding logic
    int nb     = nBytes - (nBytes % BLOCKSIZE);
    int blocks = nb / BLOCKSIZE;

    if (blocks < 3) {
        printf("[FAIL] too few blocks after rounding: %d (need >= 3)\n", blocks);
        return 1;
    }

    int disk = openDisk((char *)filename, nb);
    if (disk < 0) {
        printf("[FAIL] openDisk on created image returned %d\n", disk);
        return 1;
    }

    // ----- Check superblock at block 0 -----
    superblock_disk sb;
    rc = readBlock(disk, 0, &sb);
    if (rc != TFS_SUCCESS) {
        printf("[FAIL] readBlock(superblock) returned %d\n", rc);
        closeDisk(disk);
        return 1;
    }

    printf("[INFO] Superblock:\n");
    printf("       blocktype = %u\n", sb.blocktype);
    printf("       magic     = 0x%x\n", sb.magic);
    printf("       root_inode= %d\n", sb.root_inode);
    printf("       free_block= %d\n", sb.free_block);

    if (sb.blocktype != SUPERBLOCK) {
        printf("[FAIL] superblock blocktype = %u, expected %u (SUPERBLOCK)\n",
               sb.blocktype, (unsigned)SUPERBLOCK);
        closeDisk(disk);
        return 1;
    }

    if (sb.magic != MAGIC) {
        printf("[FAIL] superblock magic = 0x%x, expected 0x%x\n",
               sb.magic, MAGIC);
        closeDisk(disk);
        return 1;
    }

    if (sb.free_block != 2 && blocks > 2) {
        printf("[FAIL] superblock free_block = %d, expected 2\n", sb.free_block);
        closeDisk(disk);
        return 1;
    }

    // ----- Check free blocks from 2..blocks-1 -----
    printf("[INFO] Checking free blocks and next pointers...\n");

    for (int i = 2; i < blocks; i++) {
        free_disk fb;
        rc = readBlock(disk, i, &fb);
        if (rc != TFS_SUCCESS) {
            printf("[FAIL] readBlock(free block %d) returned %d\n", i, rc);
            closeDisk(disk);
            return 1;
        }

        if (fb.blocktype != FREE) {
            printf("[FAIL] block %d blocktype = %u, expected %u (FREE)\n",
                   i, fb.blocktype, (unsigned)FREE);
            closeDisk(disk);
            return 1;
        }

        if (fb.magic != MAGIC) {
            printf("[FAIL] free block %d magic = 0x%x, expected 0x%x\n",
                   i, fb.magic, MAGIC);
            closeDisk(disk);
            return 1;
        }

        int expectedNext = (i != blocks - 1) ? (i + 1) : 0;
        if (fb.blk_next != expectedNext) {
            printf("[FAIL] free block %d blk_next = %d, expected %d\n",
                   i, fb.blk_next, expectedNext);
            closeDisk(disk);
            return 1;
        }
    }

    printf("[PASS] superblock magic and free-block chain look good.\n");
    closeDisk(disk);
    return 0;
}

