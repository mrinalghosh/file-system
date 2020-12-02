#include "fs.h"

#include <stdio.h>
#include <string.h>

#include "disk.h"

#define DEBUG 0

/* global variables */
struct superblock fs;
struct fd_t fd[MAX_FILDES];  // renamed to prevent collision with argument fildes
int *FAT;
struct dir_entry *DIR;
bool mounted = false;  // TODO: is this even needed as a sanity check?

/* EOF already defined in stdio.h as -1 */
const int FREE = -2;  // not allocated to a file
// const int RESERVED = -3;  // FAT, DIR, superblock not to be overwritten // DO I NEED THIS OR CAN I JUSST USE EOF AGAIN

static void debug_dir(int n) {
    if (DEBUG) {
        printf("\nDIR entry %d\n---------8<--------\nused:%d\nfilename:%s\nsize:%d\nhead:%d\nref_count:%d\n===================\n",
               n, DIR[n].used, DIR[n].name, DIR[n].size, DIR[n].head, DIR[n].ref_count);
    }
}

static void debug_fat(int *fat) {
    if (DEBUG) {
        printf("FAT\n------------------------8<----------------------------------------------------------------------------------------------------\n");
        int i, j, n = 32;
        for (i = 0; i < DISK_BLOCKS / n; ++i) {
            for (j = 0; j < n; ++j)
                printf("%d  ", fat[i * n + j]);
            printf("\n");
        }
        printf("=============================================================================================================================\n");
    }
}

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
    if (make_disk(disk_name) == -1) {
        perror("make_fs: make_disk");
        return -1;
    }

    if (open_disk(disk_name) == -1) {
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
    char *buffer = calloc(1, BLOCK_SIZE);  // temporary buffer

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
        // printf("-----------------BUFFER START------------------\n");
        int j;
        for (j = 0; j < BLOCK_SIZE / sizeof(int); ++j) {  // iterate over # of FAT entries in one block = 4096 / 4 = 1024

            // if (j + i * BLOCK_SIZE / sizeof(int) < fs_.data_idx) {
            //     // printf("Marking element %d as RESERVED in buffer", j + i * BLOCK_SIZE / 4);
            //     memcpy((void *)(buffer + j * sizeof(int)), (void *)&RESERVED, sizeof(int));
            //     // printf("value = %d\n", *(buffer + j * sizeof(int)));
            // } else {
            // printf("Marking element %d as FREE in buffer", j + i * BLOCK_SIZE / 4);
            memcpy((void *)(buffer + j * sizeof(int)), (void *)&FREE, sizeof(int));
            // printf("value = %d\n", *(buffer + j * sizeof(int)));
        }

        if (block_write(fs_.fat_idx + i, buffer) == -1) {
            perror("mount_fs: block_read()");
            return -1;
        }
    }

    if (close_disk(disk_name) == -1) {  // TODO: do I need to close disk?
        perror("mount_fs: block_read()");
        return -1;
    }

    return 0;
}

int mount_fs(char *disk_name) {
    if (open_disk(disk_name) == -1) {
        perror("mount_fs: open_disk");
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
    memcpy((void *)(DIR), (void *)(buffer), BLOCK_SIZE);  // only 1 DIR block

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
        memcpy((void *)buffer, (void *)(FAT + i * BLOCK_SIZE), BLOCK_SIZE);
        if (block_write(fs.fat_idx + i, buffer) == -1) return -1;
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
    int fd_idx, fd_cnt = 0;
    for (fd_idx = 0; fd_idx < MAX_FILDES; ++fd_idx) {  // iter over descriptors in fildes array
        if (fd[fd_idx].used == false) {                // unused fd_idx found
            break;
        }
        ++fd_cnt;
    }

    if (fd_cnt == MAX_FILDES) {
        perror("fs_open: maximum open file descriptors");
        return -1;
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
    int i;
    for (i = 0; i < MAX_FILES; ++i) {  // iter over directory entries
        if (DIR[i].head == fd[fildes].file) {
            fd[fildes] = (fd_t){.used = false, .file = -1, .offset = 0};  // unuse file descriptor
            --DIR[i].ref_count;                                           // decrement references
        }
    }

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

    debug_fat(FAT);

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

    /* populate DIR */
    for (i = 0; i < MAX_FILES; ++i) {
        if (DIR[i].used == false) {  // first unused entry
            // DIR[i] = (dir_entry){.used = true, .name = name, .size = 0, .head = head, .ref_count = 0}; // this does not work

            DIR[i].used = true;
            memcpy((void *)DIR[i].name, (void *)name, MAX_FILENAME);
            DIR[i].size = 0;
            DIR[i].head = head;
            DIR[i].ref_count = 0;

            debug_dir(i);
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
                debug_dir(idx);
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

    debug_dir(idx);

    return 0;
}

int fs_read(int fildes, void *buf, size_t nbyte) {
    if (fildes < 0 || fildes > MAX_FILDES || fd[fildes].used == false) {  // file descriptor invalid
        perror("fs_read: file descriptor invalid");
        return -1;
    }

    int idx = directory_index(fildes);  // index of directory referred to by fildes
    int bytes = 0;                      // bytes actually read

    int head = DIR[idx].head;

    char *buffer = calloc(1, BLOCK_SIZE); /* allocate buffer */

    // if (fd[fildes].offset % BLOCK_SIZE == 0) {  // TODO: this is different - MAYBE? - try this case when debugging
    // }

    /* traverse till starting block number given offset */
    int i;
    for (i = 0; i < (fd[fildes].offset - 1) / BLOCK_SIZE + 1; ++i) {  // iter over blocks till block containing end of offset
        head = FAT[head];                                             // TODO: check that this works
    }

    /* read data starting from offset */
    for (i = 0; i < ((nbyte - 1) / BLOCK_SIZE + 1); ++i) {  // eg. (4096-1)/4096 + 1 = 1 -> edge cases with byte multiples of blocks are fixed
        block_read(head, buffer);

        /* CASES 1.first - 3.middle - 2.last */
        if (i == 0) {
            bytes += BLOCK_SIZE - fd[fildes].offset % BLOCK_SIZE;
            memcpy((void *)buf, (void *)buffer, BLOCK_SIZE - fd[fildes].offset % BLOCK_SIZE);  // last part of first block
        } else if (i == nbyte / BLOCK_SIZE - 1) {
            bytes += (nbyte + fd[fildes].offset) % BLOCK_SIZE;
            memcpy((void *)buf, (void *)buffer, (nbyte + fd[fildes].offset) % BLOCK_SIZE);  // first part of last block
        } else {
            bytes += BLOCK_SIZE;
            memcpy((void *)buf, (void *)buffer, BLOCK_SIZE);
        }

        head = FAT[head];  // iter to next block

        if (head == EOF) {
            break;
        }
    }

    fd[fildes].offset += bytes; /* implicitly increment 'file pointer' */
    return bytes;               // return number of bytes actually read
}

int fs_write(int fildes, void *buf, size_t nbyte) {
    if (fildes < 0 || fildes > MAX_FILDES || fd[fildes].used == false) { /* file descriptor invalid */
        perror("fs_write: file descriptor invalid");
        return -1;
    }

    int idx = directory_index(fildes);
    int head = DIR[idx].head;  // current

    char *buffer = calloc(1, BLOCK_SIZE);

    /* extend file with # new blocks = (offset + nbyte - size) / BLOCK_SIZE + 1 */

    int i;
    for (i = 0; i < (fd[fildes].offset + nbyte - 1) / BLOCK_SIZE + 1; ++i) {  // iterate over total # of blocks needed in written file

        if (FAT[head] == EOF) {  // if next block is end of file - extend
            /* find first free FAT block and extend by one each time */
            int j;
            for (j = fs.data_idx; j < DISK_BLOCKS; ++j) {  // iter over number of blocks in data segment
                if (FAT[j] == FREE) {                      /* next free block in fat found */
                    // if (i == (fd[fildes].offset + nbyte - 1) / BLOCK_SIZE) /* last iteration - need to set EOF */
                    FAT[head] = j;
                    FAT[j] = EOF;
                    break;
                } else if (j == DISK_BLOCKS - 1) {  // no free FAT blocks - hit when not broken out and last block not free
                    perror("fs_write: no free blocks");
                    return -1;
                }
            }
        }
        head = FAT[head];  // next
    }

    /* traverse till starting block number given offset */
    head = DIR[idx].head;  // reset head
    for (i = 0; i < (fd[fildes].offset - 1) / BLOCK_SIZE; ++i) {
        head = FAT[head];
    }

    /* write from buffer into file block by block beginning at offset */
    int bytes = 0;
    char *buf__ = buf;  // to be able to modify this pointer when iterating

    for (i = 0; i < (nbyte - 1) / BLOCK_SIZE + 1; ++i) {  // iter over number of blocks to be written to
        /* CASES:   
        1. (last of) first - starting from offset 
        2. (first of) last
        3. middle */
        if (i == 0) {
            if (block_read(head, buffer) == -1) { /* preserve contents of block above offset */
                perror("fs_write: block_read()");
                return -1;
            }
            memcpy((void *)(buffer + fd[fildes].offset % BLOCK_SIZE), (void *)(buf__ + bytes), BLOCK_SIZE - (fd[fildes].offset % BLOCK_SIZE)); /* copy buffer and write to block */
            bytes += BLOCK_SIZE - (fd[fildes].offset % BLOCK_SIZE);
        } else if (i == (nbyte - 1) / BLOCK_SIZE) {
            if (block_read(head, buffer) == -1) { /* preserve contents of last block after last written */
                perror("fs_write: block_read()");
                return -1;
            }
            memcpy((void *)buffer, (void *)(buf__ + bytes), (nbyte + fd[fildes].offset) % BLOCK_SIZE); /* copy buffer<-buf */
            bytes += (nbyte + fd[fildes].offset) % BLOCK_SIZE;
        } else {
            memcpy((void *)buffer, (void *)(buf__ + bytes), BLOCK_SIZE);
            bytes += BLOCK_SIZE;
        }

        if (block_write(head, buffer) == -1) {  // write block
            perror("fs_write: block_write()");
            return -1;
        }

        head = FAT[head];  // TODO: sanity check this during debug - INCREMENT TO NEXT BLOCK
    }

    fd[fildes].offset += bytes;

    // TODO: bytes should be the same as nbyte if no failures, right?

    /* update file size (could be writing in the middle or end) */
    if (fd[fildes].offset + bytes > DIR[idx].size) {
        DIR[idx].size = fd[fildes].offset + bytes;
    }

    return bytes;
}

int fs_get_filesize(int fildes) {
    int idx = directory_index(fildes);
    if (idx == -1) {
        perror("fs_get_filesize");
        return -1;
    }
    return DIR[idx].size;
}

int fs_listfiles(char ***files) { /* what possible error could this need to return -1 from? */
    char **files__ = calloc(MAX_FILES, sizeof(char *));
    int i, idx = 0;

    for (i = 0; i < MAX_FILES; ++i) {
        if (DIR[i].used == true) {
            files__[idx] = calloc(MAX_FILENAME, sizeof(char));
            memcpy((void *)files__[idx], (void *)DIR[i].name, MAX_FILENAME);
            ++idx;
        }
    }

    files[idx] == NULL;
    *files = files__;

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
    }  // on last iter head is LAST block before truncation = trunc (below)

    /* set first block to be truncated to EOF and free blocks after */
    int temp = head;
    int trunc = head;
    while (head != EOF) {
        temp = head;
        head = FAT[head];
        temp = FREE;
    }

    FAT[trunc] = EOF;

    DIR[idx].size = length;

    return 0;
}