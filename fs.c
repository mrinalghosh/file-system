#include "fs.h"

#include <stdio.h>
#include <string.h>

#include "disk.h"

/* global variables */
static struct superblock fs;
static struct fd_t fd[MAX_FILDES];  // renamed to prevent collision with argument fildes
static int *FAT;
static struct dir_entry *DIR;
static bool mounted = false;

/* EOF already defined in stdio.h as -1 */
const int FREE = -2;  // block not allocated to file

static int directory_index(int fildes) {
    int idx;
    for (idx = 0; idx < MAX_FILES; ++idx) {
        if (DIR[idx].used == true && DIR[idx].head == fd[fildes].file) {
            return idx;
        }
    }
    return -1;
}

int make_fs(char *disk_name) {
    if (make_disk(disk_name) == -1) { /* make disk */
        perror("make_fs: make_disk");
        return -1;
    }

    if (open_disk(disk_name) == -1) { /* open disk */
        perror("make_fs: open_disk");
        return -1;
    }

    /* initialize meta-information */
    superblock fs_;
    fs_.dir_idx = 1;
    fs_.dir_len = (sizeof(dir_entry) * MAX_FILES) / BLOCK_SIZE + 1;  // 32 * 64 / 4096 + 1 = 1
    fs_.fat_idx = fs_.dir_idx + fs_.dir_len;                         // 1 + 1 = 2
    fs_.fat_len = (sizeof(int) * DISK_BLOCKS) / BLOCK_SIZE;          // 4 * 8192 / 4096 = 8
    fs_.data_idx = fs_.fat_idx + fs_.fat_len;                        // 2 + 8 = 10 | SAVE SPACE BY BEGINNING DATA SEGMENT AFTER FAT AND DIR = fs_.dir_idx+fs_.dir_len

    /* copy meta-information to disk blocks */
    char *buffer = calloc(1, BLOCK_SIZE);

    /* block write super block */
    memcpy((void *)buffer, (void *)&fs_, sizeof(superblock));
    if (block_write(0, buffer) == -1) {
        perror("mount_fs: block_read()");
        return -1;
    }

    /* block write DIR */
    dir_entry empty = {.used = false, .name[0] = '\0', .size = 0, .head = -1, .ref_count = 0}; /* placeholder empty entry */

    int i;
    for (i = 0; i < MAX_FILES; ++i) {
        memcpy((void *)(buffer + i * sizeof(dir_entry)), (void *)&empty, sizeof(dir_entry)); /* populate single directory block buffer with empty entries */
    }

    if (block_write(fs_.dir_idx, buffer) == -1) {
        perror("mount_fs: block_read()");
        return -1;
    }

    /* block write FAT */
    for (i = 0; i < fs_.fat_len; ++i) {
        int j;
        for (j = 0; j < BLOCK_SIZE / sizeof(int); ++j) {                             // iterate over # of FAT entries in one block = 4096 / 4 = 1024
            memcpy((void *)(buffer + j * sizeof(int)), (void *)&FREE, sizeof(int));  // mark FAT elements as all free (never touch meta < 10 tho)
        }

        if (block_write(fs_.fat_idx + i, buffer) == -1) {
            perror("mount_fs: block_read()");
            return -1;
        }
    }

    if (close_disk(disk_name) == -1) {
        perror("mount_fs: block_read()");
        return -1;
    }

    return 0;
}

int mount_fs(char *disk_name) {
    if (open_disk(disk_name) == -1) {
        perror("mount_fs: open_disk()");
        return -1;
    }

    if (mounted == true) {
        perror("mount_fs: disk already mounted");
        return -1;
    }

    DIR = calloc(MAX_FILES, sizeof(dir_entry));  // 64 x 32
    FAT = calloc(DISK_BLOCKS, sizeof(int));      // 8192 x 4

    char *buffer = calloc(1, BLOCK_SIZE);

    /* mount super block */
    if (block_read(0, buffer) == -1) {
        perror("mount_fs: block_read()");
        return -1;
    }
    memcpy((void *)&fs, (void *)buffer, sizeof(superblock));

    /* mount DIR */
    if (block_read(fs.dir_idx, buffer) == -1) {
        perror("mount_fs: block_read()");
        return -1;
    }
    memcpy((void *)(DIR), (void *)(buffer), BLOCK_SIZE);

    /* mount FAT */
    int i;
    for (i = 0; i < fs.fat_len; ++i) {  // iter over FAT blocks
        if (block_read(fs.fat_idx + i, buffer) == -1) {
            perror("mount_fs: block_read()");
            return -1;
        }
        memcpy((void *)(FAT + i * BLOCK_SIZE / sizeof(int)), (void *)buffer, BLOCK_SIZE);  // can just copy entire block since it is a scalar multiple
    }

    /* initialize file descriptor array for local use */
    for (i = 0; i < MAX_FILDES; ++i) {  // iter over file descriptors
        fd[i] = (fd_t){.used = false, .file = -1, .offset = 0};
    }

    mounted = true;
    return 0;
}

int umount_fs(char *disk_name) {
    char *buffer = calloc(1, BLOCK_SIZE);

    /* write back superblock */
    memcpy((void *)buffer, (void *)&fs, sizeof(superblock));
    if (block_write(0, buffer) == -1) return -1;

    /* write back DIR */
    memcpy((void *)buffer, (void *)DIR, BLOCK_SIZE);
    if (block_write(fs.dir_idx, buffer) == -1) return -1;

    /* write back FAT */
    int i;
    for (i = 0; i < fs.fat_len; ++i) {
        memcpy((void *)buffer, (void *)(FAT + i * BLOCK_SIZE / sizeof(int)), BLOCK_SIZE);
        if (block_write(fs.fat_idx + i, buffer) == -1) {
            perror("umount_fs: block_write()");
            return -1;
        }
    }

    /* fildes not written to since not persistent across mounts */

    if (close_disk(disk_name) == -1) {
        perror("umount_fs: close_disk");
        return -1;
    }

    return 0;
}

int fs_open(char *name) {
    /* find first unused fd */
    int fd_idx;
    for (fd_idx = 0; fd_idx < MAX_FILDES; ++fd_idx) {  // iter over descriptors in fildes array
        if (fd[fd_idx].used == false) {                // unused fd_idx found
            break;
        } else if (fd_idx == MAX_FILDES - 1) {  // if not broken out of and no free fds
            perror("fs_open: maximum open file descriptors");
            return -1;
        }
    }

    /* find in directory and populate fd */
    int i;
    for (i = 0; i < MAX_FILES; ++i) {          // iter over files in directory
        if (strcmp(name, DIR[i].name) == 0) {  // file DIR entry found
            fd[fd_idx] = (fd_t){.used = true, .file = DIR[i].head, .offset = 0};
            ++DIR[i].ref_count;
            break;
        } else if (i == MAX_FILES - 1) {  // last iter and not broken out
            perror("fs_open: filename not found");
            return -1;
        }
    }

    return fd_idx;  // return file desciptor (idx)
}

int fs_close(int fildes) {
    int idx = directory_index(fildes);
    if (idx == -1 || fd[fildes].used == false) {
        perror("fs_close: invalid file descriptor");
        return -1;
    }

    fd[fildes] = (fd_t){.used = false, .file = -1, .offset = 0};  // free file descriptor
    --DIR[idx].ref_count;                                         // decrement references

    return 0;
}

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
    for (head = fs.data_idx; head < DISK_BLOCKS; ++head) {  // iter over number of blocks
        if (FAT[head] == FREE) {
            FAT[head] = EOF;
            break;
        } else if (head == DISK_BLOCKS - 1) {  // no free FAT blocks - hit when not broken out and last block not free
            perror("fs_create: no free blocks");
            return -1;
        }
    }

    /* populate DIR entry for file */
    for (i = 0; i < MAX_FILES; ++i) {
        if (DIR[i].used == false) {  // use first unused entry
            DIR[i].used = true;
            strcpy(DIR[i].name, name);
            DIR[i].size = 0;
            DIR[i].head = head;
            DIR[i].ref_count = 0;

            break;
        }
    }

    return 0;
}

int fs_delete(char *name) {
    /* find DIR entry index and check for open file descriptors */
    int idx;
    for (idx = 0; idx < MAX_FILES; ++idx) {
        if (strcmp(DIR[idx].name, name) == 0) {
            if (DIR[idx].ref_count > 0) {
                perror("fs_delete: open handles for file");
                return -1;
            }
            break;
        } else if (idx == MAX_FILES - 1) {
            perror("fs_delete: filename does not exist");
            return -1;
        }
    }

    /* free block allocation in FAT */
    int head = DIR[idx].head, ante;
    while (FAT[head] != EOF) {
        ante = head;
        head = FAT[head];
        FAT[ante] = FREE;
    }

    /* clear DIR entry */
    DIR[idx].used = false;
    memset(DIR[idx].name, '\0', MAX_FILENAME);
    DIR[idx].size = 0;
    DIR[idx].head = -1;
    DIR[idx].ref_count = 0;

    return 0;
}

int fs_read(int fildes, void *buf, size_t nbyte) {
    if (fildes < 0 || fildes > MAX_FILDES || fd[fildes].used == false) {  // file descriptor invalid
        perror("fs_read: file descriptor invalid");
        return -1;
    }

    if (nbyte <= 0) {
        perror("fs_read: nbyte invalid");
        return -1;
    }

    int idx = directory_index(fildes);  // index of directory referred to by fildes
    // int bytes = 0;                      // bytes actually read

    int head = DIR[idx].head;

    char *buffer = calloc(1, BLOCK_SIZE * ((DIR[idx].size - 1) / BLOCK_SIZE + 1));  // allocate in multiples of blocks

    int i;
    for (i = 0; i < (DIR[idx].size - 1) / BLOCK_SIZE + 1; ++i) {  // iter over number of blocks in buffer
        if (block_read(head, buffer + i * BLOCK_SIZE) == -1) {
            perror("fs_read: block_read()");
            return -1;
        }
        head = FAT[head];
    }

    memcpy(buf, (void *)(buffer + fd[fildes].offset), nbyte);

    if (fd[fildes].offset + nbyte > DIR[idx].size) { /* read does not out of bounds of filesize */
        int offset = fd[fildes].offset;
        fd[fildes].offset = DIR[idx].size;
        return DIR[idx].size - offset;
    }

    fd[fildes].offset += nbyte;
    return nbyte;
}

int fs_write(int fildes, void *buf, size_t nbyte) {
    if (fildes < 0 || fildes > MAX_FILDES || fd[fildes].used == false) { /* file descriptor invalid */
        perror("fs_write: file descriptor invalid");
        return -1;
    }

    int idx = directory_index(fildes);
    int head = DIR[idx].head;  // current

    /* check if nbyte increases file size above 16MB cap */
    if (fd[fildes].offset + nbyte > MAX_FILESIZE) {
        nbyte = MAX_FILESIZE - fd[fildes].offset;
    }

    /* extend file with new blocks */
    int i;
    for (i = 0; i < (fd[fildes].offset + nbyte - 1) / BLOCK_SIZE; ++i) { /* iter over total blocks needed in new file */
        if (FAT[head] == EOF) {
            /* find first free FAT block and extend by one each time */
            int j;
            for (j = fs.data_idx; j < DISK_BLOCKS; ++j) {  // iter over number of blocks in data segment
                if (FAT[j] == FREE) {                      /* next free (-2) block in fat found */
                    FAT[head] = j;
                    FAT[j] = EOF; /* -1 */
                    break;
                } else if (j == DISK_BLOCKS - 1) {  // no free FAT blocks - hit when not broken out and last block not free
                    perror("fs_write: no free blocks");
                    return -1;
                }
            }
        }
        head = FAT[head];  // next
    }

    /* new file size and calloc the buffer based on this size */
    if (fd[fildes].offset + nbyte > DIR[idx].size) {
        DIR[idx].size = fd[fildes].offset + nbyte;
    }

    char *buffer = calloc(1, BLOCK_SIZE * ((DIR[idx].size - 1) / BLOCK_SIZE + 1));  // allocate as multiples of blocks

    /* copy existing blocks over */
    for (i = 0; i < (DIR[idx].size - 1) / BLOCK_SIZE + 1; ++i) {  // doesn't matter if we look at empty new blocks too
        if (block_read(head, buffer + i * BLOCK_SIZE) == -1) {    // write directly from blocks to buffer
            perror("fs_write: block_read()");
            return -1;
        }
    }

    /* memcpy new bytes to write into buffer */
    memcpy((void *)(buffer + fd[fildes].offset), buf, nbyte);

    /* write entire buffer back to respective blocks */
    for (i = 0; i < (DIR[idx].size - 1) / BLOCK_SIZE + 1; ++i) {  // doesn't matter if we look at empty new blocks too
        if (block_write(head, buffer + i * BLOCK_SIZE) == -1) {   // write directly from blocks to buffer
            perror("fs_write: block_write()");
            return -1;
        }
    }

    /* increment offset to start next operation at this point */
    fd[fildes].offset += nbyte;

    return nbyte;  // corrected nbyte to maximum -- see above
}

int fs_get_filesize(int fildes) {
    int idx = directory_index(fildes);
    if (idx == -1) {
        perror("fs_get_filesize");
        return -1;
    }
    return DIR[idx].size;
}

int fs_listfiles(char ***files) {
    char **flist = calloc(MAX_FILES, sizeof(char *));
    int i, idx = 0;

    for (i = 0; i < MAX_FILES; ++i) {
        if (DIR[i].used == true) {
            flist[idx] = calloc(MAX_FILENAME, sizeof(char));
            strcpy(flist[idx], DIR[i].name);
            ++idx;
        }
    }

    flist[idx] = NULL;
    *files = flist;

    return 0;
}

int fs_lseek(int fildes, off_t offset) {
    if (fildes < 0 || fildes > MAX_FILDES || fd[fildes].used == false) { /* file descriptor invalid */
        perror("fs_lseek: file descriptor invalid");
        return -1;
    }

    int idx = directory_index(fildes);

    if (offset < 0 || offset > DIR[idx].size) { /* offset invalid */
        perror("fs_lseek: offset invalid");
        return -1;
    }

    fd[fildes].offset = offset;

    return 0;
}

int fs_truncate(int fildes, off_t length) {
    if (fildes < 0 || fildes > MAX_FILDES || fd[fildes].used == false) { /* file descriptor invalid */
        perror("fs_truncate: file descriptor invalid");
        return -1;
    }

    int idx = directory_index(fildes);

    if (length > DIR[idx].size) { /* invalid length */
        perror("fs_truncate: length > filesize");
        return -1;
    }

    /* keep blocks till length */
    int i;
    int head = DIR[idx].head;
    for (i = 0; i < (length - 1) / BLOCK_SIZE; ++i) {
        head = FAT[head];
    }

    /* set first block to be truncated to EOF and free blocks after */
    int temp;
    int trunc = head; /* last block before truncation */
    while (head != EOF) {
        temp = head;
        head = FAT[head];
        FAT[temp] = FREE;
    }

    FAT[trunc] = EOF;

    DIR[idx].size = length;

    /* truncate offsets in all open fds ? */

    return 0;
}