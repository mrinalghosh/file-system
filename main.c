#include <stdio.h>

#include "fs.h"

int main(int argc, char** argv) {
    char* filename = "test";

    make_fs(filename);

    mount_fs(filename);
    
    umount_fs(filename);

    return 0;
}