#include "fs.h"

#include <stdio.h>
#include <string.h>

#include "disk.h"

/* global variables */
struct superblock_t fs;
struct fd_t fildes[MAX_FILDES];
int *FAT;
struct dir_entry_t *DIR;
bool mounted = false;  // TODO: is this even needed as a sanity check?

/* EOF already defined in stdio.h as -1 */
const int FREE = -2;      // not allocated to a file
const int RESERVED = -3;  // FAT, DIR, superblock not to be overwritten // DO I NEED THIS OR CAN I JUSST USE EOF AGAIN

static void DEBUG_DIR(int n) { printf("DIR entry %d\n---------8<--------\nused:%d\nfilename:%s\nsize:%d\nhead:%d\nref_count:%d\n---------8<--------\n", n, DIR[n].used, DIR[n].name, DIR[n].size, DIR[n].head, DIR[n].ref_count); }

int make_fs(char *disk_name) {
    if (make_disk(disk_name) == -1 || open_disk(disk_name) == -1) {
        perror("make_disk");
        return -1;
    }

    /* initialize meta-information */
    superblock_t fs_;
    fs_.dir_idx = 1;
    fs_.dir_len = (sizeof(dir_entry_t) * MAX_FILES) / BLOCK_SIZE + 1;  // 32 * 64 / 4096 + 1 = 1
    fs_.fat_idx = fs_.dir_idx + fs_.dir_len;                           // 1 + 1 = 2
    fs_.fat_len = (sizeof(int) * DISK_BLOCKS) / BLOCK_SIZE;            // 4 * 8192 / 4096 = 8
    fs_.data_idx = fs_.fat_idx + fs_.fat_len;                          // 2 + 8 = 10 | SAVE SPACE BY BEGINNING DATA SEGMENT AFTER FAT AND DIR = fs_.dir_idx+fs_.dir_len

    /* copy meta-information to disk blocks */
    char *buffer = calloc(1, BLOCK_SIZE);  // temporary buffer

    /* block write super block */
    memcpy((void *)buffer, (void *)&fs_, sizeof(superblock_t));
    if (block_write(0, buffer) == -1) return -1;

    /* block write DIR */
    dir_entry_t empty = {.used = false, .name[0] = '\0', .size = 0, .head = -1, .ref_count = 0}; /* placeholder empty entry */

    int i;
    for (i = 0; i < MAX_FILES; ++i) {
        memcpy((void *)(buffer + i * sizeof(dir_entry_t)), (void *)&empty, sizeof(dir_entry_t)); /* populate single directory block buffer with empty entries */
    }

    if (block_write(fs_.dir_idx, buffer) == -1) return -1;

    /* block write FAT */
    for (i = 0; i < fs_.fat_len; ++i) {
        // printf("-----------------BUFFER START------------------\n");
        int j;
        for (j = 0; j < BLOCK_SIZE / sizeof(int); ++j) {  // iterate over # of FAT entries in one block = 4096 / 4 = 1024
            if (j + i * BLOCK_SIZE / sizeof(int) < fs_.data_idx) {
                // printf("Marking element %d as RESERVED in buffer", j + i * BLOCK_SIZE / 4);
                memcpy((void *)(buffer + j * sizeof(int)), (void *)&RESERVED, sizeof(int));
                // printf("value = %d\n", *(buffer + j * sizeof(int)));
            } else {
                // printf("Marking element %d as FREE in buffer", j + i * BLOCK_SIZE / 4);
                memcpy((void *)(buffer + j * sizeof(int)), (void *)&FREE, sizeof(int));
                // printf("value = %d\n", *(buffer + j * sizeof(int)));
            }
        }

        if (block_write(fs_.fat_idx + i, buffer) == -1) return -1;
    }

    free(buffer);
    if (close_disk(disk_name) == -1) return -1;  // TODO: need to close disk after writing?

    return 0;
}

int mount_fs(char *disk_name) {
    if (open_disk(disk_name) == -1) {
        perror("mount_fs: open_disk");
        return -1;
    }

    // fs = calloc(1, sizeof(superblock_t)); // don't need to dynamically allocate - should super block be a static  variable then?
    DIR = calloc(MAX_FILES, sizeof(dir_entry_t));
    FAT = calloc(DISK_BLOCKS, sizeof(int));

    char *buffer = calloc(1, BLOCK_SIZE);

    /* mount super block */
    if (block_read(0, buffer) == -1) return -1;
    memcpy((void *)&fs, (void *)buffer, sizeof(superblock_t));

    /* mount DIR */
    if (block_read(fs.dir_idx, buffer) == -1) return -1;
    memcpy((void *)(DIR), (void *)(buffer), BLOCK_SIZE);  // only 1 DIR block - don't need to iterate

    /* mount FAT */
    int i;
    for (i = 0; i < fs.fat_len; ++i) {
        if (block_read(fs.fat_idx + i, buffer) == -1) return -1;
        memcpy((void *)(FAT + i * BLOCK_SIZE), (void *)buffer, BLOCK_SIZE);  // can just copy entire block since it is a scalar multiple
    }

    /* initialize file descriptor array for local use */
    for (i = 0; i < MAX_FILDES; ++i) {  // iter over file descriptors
        fildes[i] = (fd_t){.used = false, .file = -1, .offset = 0};
    }

    // TODO: THIS KEEPS GIVING MUNMAP_CHUNK(): INVALID POINTER WHAT DO
    // free(buffer);
    mounted = true;

    // TODO: SHOULD RETURN ZERO IF DOESN'T CONTAIN A VALID FILE SYSTEM - what to check for?

    return 0;
}

int umount_fs(char *disk_name) {
    char *buffer = calloc(1, BLOCK_SIZE);

    /* write back superblock */
    memcpy((void *)buffer, (void *)&fs, sizeof(superblock_t));
    if (block_write(0, buffer) == -1) return -1;

    /* write back DIR */
    memcpy((void *)buffer, (void *)DIR, BLOCK_SIZE);
    if (block_write(fs.dir_idx, buffer) == -1) return -1;

    /* write back FAT */
    int i;
    for (i = 0; i < fs.fat_len; ++i) {
        memcpy((void *)buffer, (void *)(FAT + i * BLOCK_SIZE), BLOCK_SIZE);
        if (block_write(fs.fat_idx + i, buffer) == -1) return -1;
    }

    /* fildes not written since not persistent across mounts */

    if (close_disk(disk_name) == -1) {
        perror("umount_fs: close_disk");
        return -1;
    }

    return 0;
}

int fs_open(char *name) {
    /* find first unused fd */
    int fd_idx, fd_cnt = 0;
    for (fd_idx = 0; fd_idx < MAX_FILDES; ++fd_idx) {  // iter over fd in fildes array
        if (fildes[fd_idx].used == false) {            // unused fd_idx found
            break;
        }
        ++fd_cnt;
    }

    if (fd_cnt == MAX_FILDES) {
        perror("fs_open: maximum open file descriptors");
        return -1;
    }

    /* find file in directory and populate fd */
    int i;
    for (i = 0; i < MAX_FILES; ++i) {          // iter over files in directory
        if (strcmp(name, DIR[i].name) == 0) {  // file DIR entry found
            fildes[fd_idx] = (fd_t){.used = true, .file = DIR[i].head, .offset = 0};
            ++DIR[i].ref_count;
            break;
        } else if (i == MAX_FILES - 1) {  // last iter and not broken out
            perror("fs_open: filename not found");
            return -1;
        }
    }

    return fd_idx;  // return file desciptor (index)
}

int fs_close(int fildes) { return 0; }

int fs_create(char *name) {
    if (strlen(name) > MAX_FILENAME) {
        perror("fs_create: filename too long");
        return -1;
    }

    int i, fc = 0;  // iter over DIR, file counter
    for (i = 0; i < MAX_FILES; ++i) {
        if (DIR[i].used == true) {
            if (strcmp(name, DIR[i].name) == 0) {  // filename already exists
                perror("fs_create: filename already exists");
                return -1;
            }
            ++fc;
        }
    }

    if (fc == MAX_FILES) {
        perror("fs_create: maximum files created");
        return -1;
    }

    /* find first free FAT block and mark as EOF - needed in DIR entry below */
    int head;
    for (head = fs.data_idx; head < DISK_BLOCKS; ++head) {  // iter over data segment
        if (FAT[head] == FREE) {
            FAT[head] = EOF;
            break;
        } else if (head == DISK_BLOCKS - 1) {  // no free FAT blocks - hit when not broken out and last block not free
            perror("fs_create: no free blocks");
            return -1;
        }
    }

    /* populate DIR */
    for (i = 0; i < MAX_FILES; ++i) {
        if (DIR[i].used == false) {  // first unused entry
            DIR[i] = (dir_entry_t){.used = true, .name = name, .size = 0, .head = head, .ref_count = 0};
            DEBUG_DIR(i);
            break;
        }
    }

    return 0;
}

int fs_delete(char *name) { return 0; }

// int fs_read(int fildes, void *buf, size_t nbyte);
// int fs_write(int fildes, void *buf, size_t nbyte);

// int fs_get_filesize(int fildes);
// int fs_listfiles(char ***files);
// int fs_lseek(int fildes, off_t offset);
// int fs_truncate(int fildes, off_t length);