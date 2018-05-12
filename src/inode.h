#ifndef __INODE_H__
#define __INODE_H__

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "bitmap.h"

struct inode_t {
    static const uint8_t TYPE_DIRECTORY = 1;

    uint32_t size;
    uint16_t block_count;
    uint8_t type;
    uint16_t pointers[NUM_DIRECT_POINTERS + 1]; // count from start of data region

    uint16_t inum;

    vector< uint16_t > data_blocks_ids;

    bitmap_t *imap, *dmap;

    inode_t(bitmap_t *imap = nullptr, bitmap_t *dmap = nullptr) {
        this->imap = imap;
        this->dmap = dmap;
        size = 0;
        block_count = 0;
        type = 0;
        inum = NON_EXIST_CONSTANT;
        memset(pointers, NON_EXIST_CONSTANT, sizeof(pointers));
    }

    void seek(FILE *fp) {
        uint32_t block_id = 1 + NUM_BLOCK_BITMAP * 2 + inum / NUM_INODE_EACH_BLOCK;
        uint32_t id_in_block = inum % NUM_INODE_EACH_BLOCK;
        fseek(fp, block_id * BLOCK_SIZE + id_in_block * INODE_SIZE, SEEK_SET);
    }

    void read_from_disk(FILE *fp, uint16_t inum) { // inum start from beginning of inode table
        memset(pointers, NON_EXIST_CONSTANT, sizeof(pointers));
        for (int i = 0; i <= NUM_DIRECT_POINTERS; i++) {
            pointers[i] = NON_EXIST_CONSTANT;
        }

        this->inum = inum;
        seek(fp);

        fread(&size, sizeof size, 1, fp);
        fread(&block_count, sizeof block_count, 1, fp);
        fread(&type, sizeof type, 1, fp);
        fread(pointers, sizeof(uint16_t), NUM_DIRECT_POINTERS + 1, fp);

        data_blocks_ids.clear();

        for (int i = 0; i < block_count; ++i) {
            if (i < NUM_DIRECT_POINTERS) {
                data_blocks_ids.push_back(pointers[i]);
            } else { 
                fseek(fp, START_BYTE_OF_DATA_REGION + pointers[NUM_DIRECT_POINTERS] * BLOCK_SIZE, SEEK_SET);
                uint16_t buffer[BLOCK_SIZE / POINTER_SIZE];
                fread(buffer, sizeof(uint16_t), BLOCK_SIZE / POINTER_SIZE, fp);
                uint32_t remain = block_count - NUM_DIRECT_POINTERS;
                for (int j = 0; j < remain; ++j) {
                    data_blocks_ids.push_back(buffer[j]);
                }
            }
        }
    }

    void write_to_disk(FILE *fp) {
        seek(fp);
        fwrite(&size, sizeof size, 1, fp);
        fwrite(&block_count, sizeof block_count, 1, fp);
        fwrite(&type, sizeof type, 1, fp);
        fwrite(pointers, sizeof(uint16_t), NUM_DIRECT_POINTERS + 1, fp);

        if (block_count > NUM_DIRECT_POINTERS) {
            fseek(fp, START_BYTE_OF_DATA_REGION + pointers[NUM_DIRECT_POINTERS] * BLOCK_SIZE, SEEK_SET);
            fwrite(data_blocks_ids.data() + NUM_DIRECT_POINTERS * sizeof(uint16_t), sizeof(uint16_t), data_blocks_ids.size() - NUM_DIRECT_POINTERS, fp);
        }

        imap->write_to_disk(fp);
        dmap->write_to_disk(fp);
    }

    void push_more_data(int more) { // more = num_bytes more
        size += more;
        int nblock = size / BLOCK_SIZE + (size % BLOCK_SIZE != 0);
        while (block_count < nblock) {
            block_count += 1;
            uint16_t new_block = dmap->find_free();
            data_blocks_ids.push_back(new_block);
        }

        for (int i = 0; i < data_blocks_ids.size() && i < NUM_DIRECT_POINTERS; ++i) {
            pointers[i] = data_blocks_ids[i];
        }

        if (data_blocks_ids.size() > NUM_DIRECT_POINTERS) {
            uint16_t indirect_block = pointers[NUM_DIRECT_POINTERS];
            if (indirect_block == NON_EXIST_CONSTANT) {
                indirect_block = dmap->find_free();    
                pointers[NUM_DIRECT_POINTERS] = indirect_block;
            }
        }
    }
};

#endif