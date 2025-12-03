#include <stdio.h>
#include <string.h>

#include "libTinyFS.h"      // tfs_mkfs, tfs_mount, tfs_unmount
#include "TinyFS_errno.h"   // TFS_SUCCESS + ERR_ codes
#include "libDisk.h"        // BLOCKSIZE, openDisk, closeDisk
#include "blocktypes.h"     // MAGIC, SUPERBLOCK, etc.

static void fail(const char *msg, int rc, int expected)
{
    printf("[FAIL] %s: got %d, expected %d\n", msg, rc, expected);
}

int main(void)
{
    const char *good_disk = "m_t.fs";
    const char *bad_disk  = "n_fs.fs";
    int bytes = 8 * BLOCKSIZE;

    printf("\n--- TinyFS mount test ---\n");

    // -------------------------------------------------------------------------
    // Create valid FS
    // -------------------------------------------------------------------------
    printf("[TEST] tfs_mkfs(\"%s\", %d)\n", good_disk, bytes);
    int rc = tfs_mkfs((char *)good_disk, bytes);
    if (rc != TFS_SUCCESS) {
        fail("tfs_mkfs(valid)", rc, TFS_SUCCESS);
        return 1;
    }
    printf("[PASS] mkfs succeeded\n");

    // -------------------------------------------------------------------------
    // Mount valid FS
    // -------------------------------------------------------------------------
    printf("[TEST] tfs_mount(valid FS)\n");
    rc = tfs_mount((char *)good_disk);
    if (rc != TFS_SUCCESS) {
        fail("tfs_mount(valid)", rc, TFS_SUCCESS);
        return 1;
    }
    printf("[PASS] mount succeeded\n");

    // -------------------------------------------------------------------------
    // Double mount should fail
    // -------------------------------------------------------------------------
    printf("[TEST] tfs_mount(again) — should fail\n");
    rc = tfs_mount((char *)good_disk);
    if (rc == TFS_SUCCESS) {
        fail("tfs_mount(double mount allowed!)", rc, /* expected */ -999);
        return 1;
    }
    printf("[PASS] second mount rejected (rc=%d)\n", rc);

    // -------------------------------------------------------------------------
    // Unmount
    // -------------------------------------------------------------------------
    printf("[TEST] tfs_unmount()\n");
    rc = tfs_unmount();
    if (rc != TFS_SUCCESS) {
        fail("tfs_unmount(first)", rc, TFS_SUCCESS);
        return 1;
    }
    printf("[PASS] unmount succeeded\n");

    // -------------------------------------------------------------------------
    // Double unmount should fail
    // -------------------------------------------------------------------------
    printf("[TEST] tfs_unmount(again) — should fail\n");
    rc = tfs_unmount();
    if (rc == TFS_SUCCESS) {
        fail("tfs_unmount(double unmount allowed!)", rc, /* expected */ -999);
        return 1;
    }
    printf("[PASS] second unmount rejected (rc=%d)\n", rc);

    // -------------------------------------------------------------------------
    // Create an invalid disk file (no mkfs -> no superblock)
    // -------------------------------------------------------------------------
    printf("[TEST] create invalid disk file \"%s\"\n", bad_disk);
    int fd = openDisk((char *)bad_disk, bytes);
    if (fd < 0) {
        fail("openDisk(invalid fs file)", fd, /* expected any success */ 0);
        return 1;
    }
    closeDisk(fd);

    // -------------------------------------------------------------------------
    // Mount invalid disk (should fail)
    // -------------------------------------------------------------------------
    printf("[TEST] tfs_mount(invalid FS) — should fail\n");
    rc = tfs_mount((char *)bad_disk);
    if (rc == TFS_SUCCESS) {
        fail("tfs_mount(invalid fs accepted!)", rc, /* expected */ -999);
        return 1;
    }
    printf("[PASS] invalid disk correctly rejected (rc=%d)\n", rc);

    printf("\nALL MOUNT TESTS PASSED ✅\n");
    return 0;
}

