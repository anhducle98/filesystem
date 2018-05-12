#ifndef __CONSTANT_H__
#define __CONSTANT_H__

#include <stdint.h>

namespace FILE_SYSTEM_INFO {
    const uint32_t BLOCK_SIZE = 512;
    const uint32_t INODE_SIZE = 32;

    const uint8_t NUM_BLOCK_BITMAP = 8;
    const uint32_t NUM_INTS_BITMAP = 1 << 10;

    const uint32_t NUM_BYTE_INODE_TABLE = NUM_BLOCK_BITMAP * BLOCK_SIZE * 8 * 32;
    const uint32_t IMAP_START_BLOCK = 1;
    const uint32_t DMAP_START_BLOCK = IMAP_START_BLOCK + NUM_BLOCK_BITMAP;

    const uint32_t NUM_INODE_EACH_BLOCK = BLOCK_SIZE / INODE_SIZE;
    const uint32_t NUM_BLOCK_INODE_TABLE = NUM_BYTE_INODE_TABLE / BLOCK_SIZE;
    const uint8_t POINTER_SIZE = 2;
    const uint16_t ROOT_INUM = 0;
    const uint32_t NUM_DIRECT_POINTERS = 11;

    const uint32_t START_BLOCK_OF_DATA_REGION = 1 + NUM_BLOCK_BITMAP * 2 + NUM_BLOCK_INODE_TABLE;
    const uint32_t START_BYTE_OF_DATA_REGION = START_BLOCK_OF_DATA_REGION * BLOCK_SIZE;
}

namespace DISK_INFO {
    const char *DEFAULT_DISK = "HD.dat";
    const uint32_t DISK_SIZE = 16 * 1024 * 1024;
    const uint32_t NUM_BLOCK_DISK = DISK_SIZE / FILE_SYSTEM_INFO::BLOCK_SIZE;
}


namespace SPECIAL_CONSTANTS {
    const uint16_t NON_EXIST_CONSTANT = (uint16_t)(-1);
}

using namespace DISK_INFO;
using namespace FILE_SYSTEM_INFO;
using namespace SPECIAL_CONSTANTS;

using namespace std;

#endif