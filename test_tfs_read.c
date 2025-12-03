// test_tfs_read.c
#include <stdio.h>
#include <string.h>
#include "libTinyFS.h"
#include "TinyFS_errno.h"

int main(void) {
    const char *fsname = "test_fs.img";
    int rc;

    // 1) Create filesystem
    rc = tfs_mkfs((char *)fsname, 10 * BLOCKSIZE);
    if (rc != TFS_SUCCESS) {
        printf("tfs_mkfs failed: %d\n", rc);
        return 1;
    }

    // 2) Mount
    rc = tfs_mount((char *)fsname);
    if (rc != TFS_SUCCESS) {
        printf("tfs_mount failed: %d\n", rc);
        return 1;
    }

    // 3) Open a file
    fileDescriptor fd = tfs_openFile("foo");
    if (fd < 0) {
        printf("tfs_openFile failed: %d\n", fd);
        return 1;
    }

    // 4) Write known data
    const char *msg = "HelloTinyFS";
    int len = (int)strlen(msg);

    rc = tfs_writeFile(fd, (char *)msg, len);
    if (rc != TFS_SUCCESS) {
        printf("tfs_writeFile failed: %d\n", rc);
        return 1;
    }

    // 5) Seek back to start
    rc = tfs_seek(fd, 0);
    if (rc != TFS_SUCCESS) {
        printf("tfs_seek failed: %d\n", rc);
        return 1;
    }

    // 6) Read back one byte at a time
    char buf[64] = {0};
    for (int i = 0; i < len; i++) {
        char c;
        rc = tfs_readByte(fd, &c);
        if (rc != TFS_SUCCESS) {
            printf("tfs_readByte failed at i=%d: %d\n", i, rc);
            return 1;
        }
        buf[i] = c;
    }

    // 7) Verify EOF behavior: one extra read should fail
    char extra;
    rc = tfs_readByte(fd, &extra);
    if (rc == TFS_SUCCESS) {
        printf("ERROR: expected EOF error on extra read, got success\n");
        return 1;
    }

    // 8) Compare
    if (strncmp(buf, msg, len) != 0) {
        printf("Mismatch! wrote '%s', read back '%s'\n", msg, buf);
        return 1;
    }

    printf("PASS: tfs_readByte + tfs_seek worked, read back '%s'\n", buf);

    tfs_closeFile(fd);
    tfs_unmount();
    return 0;
}

