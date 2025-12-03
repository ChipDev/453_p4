#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tinyFS.h"   /* includes libTinyFS.h, TinyFS_errno.h, blocktypes.h */

//print fatal error message and exit
static void tfs_fatal(const char *msg, int rc) {
    fprintf(stderr, "[FATAL] %s: rc=%d\n", msg, rc);
    exit(EXIT_FAILURE);
}

//fill buffer with repeated phrase up to size
static int fillBufferWithPhrase(const char *phrase, char *buf, int size) {
    if (!phrase || !buf || size <= 0) return -1;
    int phraseLen = (int)strlen(phrase);
    if (phraseLen == 0) return -1;
    int index = 0;
    while (index < size) {
        for (int i = 0; i < phraseLen && (index + i) < size; i++) {
            buf[index + i] = phrase[i];
        }
        index += phraseLen;
    }
    buf[size - 1] = '\0';
    return 0;
}

static void demo_readBytes(fileDescriptor fd, int maxBytes) {
    int rc;
    char ch;
    int count = 0;
    printf("    Reading up to %d bytes:\n    \"", maxBytes);
    while (count < maxBytes) {
        rc = tfs_readByte(fd, &ch);
        if (rc < 0) {
            if (rc == ERR_EOF) {
                break;  /* EOF: stop cleanly */
            } else {
                tfs_fatal("tfs_readByte", rc);
            }
        }
        putchar(ch);
        count++;
    }
    printf("\"\n");
}

int main(void) {
    int rc;
    printf("=== TinyFS Demo ===\n");
    printf("[1] Mounting TinyFS disk '%s'...\n", DEFAULT_DISK_NAME);
    rc = tfs_mount(DEFAULT_DISK_NAME);
    if (rc < 0) {
        printf("    Mount failed (rc=%d). Creating new FS...\n", rc);
        rc = tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE);
        if (rc < 0) {
            tfs_fatal("tfs_mkfs", rc);
        }
        rc = tfs_mount(DEFAULT_DISK_NAME);
        if (rc < 0) {
            tfs_fatal("tfs_mount (after mkfs)", rc);
        }
    }
    printf("    Mounted successfully.\n\n");

    const int sizeA = 128;
    const int sizeB = 300;
    char *bufA = malloc(sizeA);
    char *bufB = malloc(sizeB);
    if (!bufA || !bufB) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }

    fillBufferWithPhrase("FileA-", bufA, sizeA);
    fillBufferWithPhrase("FileB-", bufB, sizeB);

    printf("[2] Opening files 'fileA' and 'fileB'...\n");
    fileDescriptor fdA = tfs_openFile("fileA");
    if (fdA < 0) tfs_fatal("tfs_openFile(fileA)", fdA);
    fileDescriptor fdB = tfs_openFile("fileB");
    if (fdB < 0) tfs_fatal("tfs_openFile(fileB)", fdB);
    printf("    fileA FD=%d, fileB FD=%d\n\n", fdA, fdB);

    printf("[3] Writing data to fileA (%d bytes) and fileB (%d bytes)...\n",
           sizeA, sizeB);

    rc = tfs_writeFile(fdA, bufA, sizeA);
    if (rc < 0) tfs_fatal("tfs_writeFile(fileA)", rc);

    rc = tfs_writeFile(fdB, bufB, sizeB);
    if (rc < 0) tfs_fatal("tfs_writeFile(fileB)", rc);

    printf("    Writes complete.\n\n");

    printf("[4] Directory listing after writes:\n");
    rc = tfs_readdir();
    if (rc < 0) tfs_fatal("tfs_readdir", rc);
    printf("\n");

    printf("[5] Demonstrating tfs_seek + tfs_readByte on 'fileA':\n");

    rc = tfs_seek(fdA, 0);
    if (rc < 0) tfs_fatal("tfs_seek(fileA,0)", rc);
    printf("    From beginning of fileA:\n");
    demo_readBytes(fdA, 64);

    int midOffset = sizeA / 2;
    rc = tfs_seek(fdA, midOffset);
    if (rc < 0) tfs_fatal("tfs_seek(fileA,mid)", rc);
    printf("    From offset %d of fileA:\n", midOffset);
    demo_readBytes(fdA, 64);
    printf("\n");

    printf("[6] Renaming 'fileB' -> 'fileC'...\n");
    rc = tfs_rename(fdB, "fileC");
    if (rc < 0) tfs_fatal("tfs_rename(fileB->fileC)", rc);

    printf("    Directory listing after rename:\n");
    rc = tfs_readdir();
    if (rc < 0) tfs_fatal("tfs_readdir", rc);
    printf("\n");

    printf("[7] Deleting 'fileA'...\n");
    rc = tfs_deleteFile(fdA);
    if (rc < 0) tfs_fatal("tfs_deleteFile(fileA)", rc);

    printf("    Directory listing after delete:\n");
    rc = tfs_readdir();
    if (rc < 0) tfs_fatal("tfs_readdir", rc);
    printf("\n");

    printf("[8] Closing remaining open files...\n");
    rc = tfs_closeFile(fdB);
    if (rc < 0) tfs_fatal("tfs_closeFile(fileC)", rc);

    free(bufA);
    free(bufB);

    printf("[9] Unmounting filesystem...\n");
    rc = tfs_unmount();
    if (rc < 0) tfs_fatal("tfs_unmount", rc);

    printf("=== TinyFS Demo complete ===\n");
    return 0;
}
