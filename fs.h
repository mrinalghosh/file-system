#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

#define error(message)      \
    {                       \
        perror(message);    \
        exit(EXIT_FAILURE); \
    }

#define MAX_FILES 64            // maximum files at any given time
#define MAX_FILDES 32           // maximum file descriptors per file
#define MAX_FILENAME 15         // maximum 15 character filenames
#define MAX_FILESIZE (1 << 24)  // maximum file size 16 MB = 16777216 B
#define FREE -1                 // not allocated to a file
#define EOF -2                  // end of file code
#define RESERVED -3             // FAT, DIR, superblock not to be overwritten

/* defined in disk.h - DISK_BLOCKS 8192 - BLOCK_SIZE 4096 */

typedef struct superblock_t {  // hardcoded to block 0
    int dir_idx;               // head of directory
    int dir_len;               // number of blocks of directory
    int fat_idx;               // head of FAT
    int fat_len;               // number of blocks of FAT
    int data_idx;              // first block of file data
} superblock_t;

typedef struct dir_entry_t {
    bool used;                    // is this file-"slot" in use
    char name[MAX_FILENAME + 1];  // mmmm.. donuts
    int size;                     // file size (bytes)
    int head;                     // first data block of file
    int ref_cnt;                  // how many open fds (ref_cnt > 0) -> cannot delete file
} dir_entry_t;

typedef struct fd_t {  // file descriptor
    bool used;         // fd in use
    int file;          // first block of the file (f) to which fd refers to
    int offset;        // position of fd within f - dynamically updated
} fd_t;

int make_fs(char *disk_name);
int mount_fs(char *disk_name);
int umount_fs(char *disk_name);

int fs_open(char *name);
int fs_close(int fildes);
int fs_create(char *name);
int fs_delete(char *name);

int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);

int fs_get_filesize(int fildes);
int fs_listfiles(char ***files);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);

#endif /* FS_H */