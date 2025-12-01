# 453 Project 4

## Phase I

### Blake
- Creating "disk driver" (`.c` and `.h`)
- Exposes API / functions that "just work"

### Everyone
- `mkfs`
- `tfs_mount`, `tfs_unmount`
- `tfs_open`, `tfs_close`, `tfs_write`, `deleteFile`
- `tfs_readByte`
- `tfs_seek`

## Notes
- A file is a "disk"
- A disk contains a filesystem (tinyFS)
- The filesystem is mountable and contains files
- Opening files gives file descriptors
