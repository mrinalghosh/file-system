#include "fs.h"

#include "disk.h"
#include "string.h"

/* global variables */
struct superblock_t fs;
struct fd_t fildes[MAX_FILDES];
short *FAT;
struct dir_entry_t *DIR;
bool mounted = false;

int make_fs(char *disk_name) {
    if (make_disk(disk_name) == -1 || open_disk(disk_name) == -1) {
        return -1;
    }

    /* initialize meta-information */
    superblock_t fs_;
    fs_.fat_idx = 1;                                                   // start after super block
    fs_.fat_len = (sizeof(short) * DISK_BLOCKS) / BLOCK_SIZE;          // 2*8192/4096 = 4
    fs_.dir_idx = fs_.fat_idx + fs_.fat_len;                           // 1 + 4 = 5
    fs_.dir_len = (sizeof(dir_entry_t) * MAX_FILES) / BLOCK_SIZE + 1;  // 32*64/4096 + 1 = 1
    fs_.data_idx = DISK_BLOCKS / 2;                                    // TODO: SAVE ALL THIS SPACE BY BEGINNING DATA SEGMENT AFTER FAT AND DIR = fs_.dir_idx+fs_.dir_len

    /* copy meta-information to disk blocks */
    char *buffer = calloc(1, BLOCK_SIZE);  // temporary buffer

    /* block write super block */
    memcpy((void *)buffer, (void *)&fs_, sizeof(superblock_t));
    block_write(0, buffer);

    /* block write FAT */
    int i;
    for (i = 0; i < fs_.fat_len; ++i) {
        memset((void *)buffer, FREE, BLOCK_SIZE);  // sets all to 0xf...f (CHARACTERWISE - check whether this works)

        if (i == 0) {  // reserve meta-segment
            int j;
            for (j = 0; j < fs_.data_idx; ++j) {
                memcpy((void *)(buffer + j * sizeof(short)), (void *)RESERVED, sizeof(short));
            }
        }

        block_write(fs_.fat_idx + i, buffer);
    }

    /* block write DIR */
    dir_entry_t empty = {.used = false, .name[0] = '\0', .size = 0, .head = -1, .ref_cnt = 0}; /* placeholder empty directory entry*/

    for (i = 0; i < MAX_FILES; ++i) {
        memcpy((void *)(buffer + i * sizeof(dir_entry_t)), (void *)&empty, sizeof(dir_entry_t)); /* populate directory block buffer with empty entries */
    }

    block_write(fs_.dir_idx, buffer);

    free(buffer);
    close_disk(disk_name);  // TODO: need to close disk after writing?

    return 0;
}

int mount_fs(char *disk_name) {
    if (open_disk(disk_name) == -1) {
        return -1;
    }

    // fs = calloc(1, sizeof(superblock_t)); // don't need to dynamically allocate - should super block be static then?
    DIR = calloc(MAX_FILES, sizeof(dir_entry_t));
    FAT = calloc(DISK_BLOCKS, sizeof(short));

    char *buffer = calloc(1, BLOCK_SIZE);

    /* mount super block */
    block_read(0, buffer);
    memcpy((void *)&fs, (void *)buffer, sizeof(superblock_t));

    /* mount DIR */
    // int i;
    // for (i = 0; i < MAX_FILES; ++i) {  // # DIR blocks is 1 - don't need to iterate
    memcpy((void *)(DIR), (void *)(buffer), BLOCK_SIZE);
    // }

    /* mount FAT */
    int i;
    for (i = 0; i < fs.fat_len; ++i) {
        block_read(fs.fat_idx + i, buffer);
        memcpy((void *)(FAT + i * BLOCK_SIZE), (void *)buffer, BLOCK_SIZE);  // can just copy entire block since it is a scalar multiple
    }

    free(buffer);
    mounted = true;

    // TODO: SHOULD RETURN ZERO IF DOESN'T CONTAIN A VALID FILE SYSTEM - what to check for?

    return 0;
}

int umount_fs(char *disk_name) {
    char *buffer = calloc(1, BLOCK_SIZE);

    /* write back superblock */
    memcpy((void *)buffer, (void *)&fs, sizeof(superblock_t));
    block_write(0, buffer);

    /* write back DIR */
    memcpy((void *)buffer, (void *)DIR, BLOCK_SIZE);
    block_write(fs.dir_idx, buffer);

    /* write back FAT */
    int i;
    for (i = 0; i < fs.fat_len; ++i) {
        memcpy((void *)buffer, (void *)(FAT + i * BLOCK_SIZE), BLOCK_SIZE);
        block_write(fs.fat_idx + i, buffer);
    }

    if (close_disk(disk_name) == -1) {
        return -1;
    }

    return 0;
}

// int fs_open(char *name);
// int fs_close(int fildes);
// int fs_create(char *name);
// int fs_delete(char *name);

// int fs_read(int fildes, void *buf, size_t nbyte);
// int fs_write(int fildes, void *buf, size_t nbyte);

// int fs_get_filesize(int fildes);
// int fs_listfiles(char ***files);
// int fs_lseek(int fildes, off_t offset);
// int fs_truncate(int fildes, off_t length);