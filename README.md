<<<<<<< HEAD
# 453 project 4
Blake: Phase I
	- Creating "disk driver" which should just be a .c, .h file
	- Exposes API/functions that 'just work'
Everyone: Phase I
	- Blake: Making structs that make each block type digestible (blocktypes.h)
	- mkfs 
	- tfs_mount, unmount
	- tfs_open, close, write, deleteFile
	- tfs_readByte 
	- tfs_seek

-> A file is a "disk"
-> A disk contains a filesystem (tinyFS)
-> the filesystem is mountable, and contains files
-> opening these files gives us file descriptors
=======
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
>>>>>>> 4c14f664b95dc33e018685790b351acaffb83196
