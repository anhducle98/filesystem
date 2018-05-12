/*
DISK_SIZE = 16MB
BLOCK_SIZE = 512 bytes
NBLOCKS = 2^15
POINTER_SIZE = 2 bytes = 8 bit
I-bitmap = 8 blocks = 8 * 512 bytes = 4 * 1024 bytes = 2^15 bit
D-bitmap = 8 blocks = 8 * 512 bytes = 4 * 1024 bytes = 2^15 bit

Inode {
    11 direct pointers = 11 * (2 bytes) = 22 bytes
    1 single indirect pointer = 2 bytes
    size = 4 bytes
    nblock = 2 bytes
    type = 1 byte //0 - file, 1 - directory, 2 - direct pointers
    remain: 1
} = 32 bytes each

=> MAX_FILE_SIZE = (11 + 512 / 2) * 512 = 136704 bytes
InodeTable = 1MB

Super block {
    DISK_SIZE: 4 bytes
    BLOCK_SIZE: 4 bytes
    
    I-bitmap block count: 1 bytes
    D-bitmap block count: 1 bytes

    Pointer size: 1 bytes
}
*/

#include <stdio.h>
#include <vector>
#include <cassert>

#include "constants.h"
#include "bitmap.h"
#include "inode.h"
#include "directory.h"
#include "file_system.h"


void run_test() {
    file_system_t FS;
    FS.create_empty_disk("HD.dat");

    FS.create_empty_folder("/", "code");
    FS.copy_file_from_outside("/code/", "main.cpp");
    FS.copy_file_from_outside("/code/", "shit.cpp");
    FS.copy_file_from_outside("/", "big.txt");

    FS.read_disk_info();
    FS.list_disk();

    FS.view_text_file("/code/shit.cpp");
}

int main() {
    run_test();    
    return 0;
}