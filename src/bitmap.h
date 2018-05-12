#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdio.h>
#include <vector>
#include <string.h>

#include "constants.h"

struct bitmap_t {
    uint32_t element[NUM_INTS_BITMAP];
    vector<uint32_t> need_update;
    int start_block; // imap starts from 1, dmap starts from 9

    bitmap_t(int start_block): start_block(start_block) {}

    void read_from_disk(FILE *fp) {
        fseek(fp, start_block * BLOCK_SIZE, SEEK_SET);
        fread(element, sizeof(uint32_t), NUM_INTS_BITMAP, fp);
    }

    void write_to_disk(FILE *fp) {
        while (!need_update.empty()) {
            uint32_t int_id = need_update.back();
            need_update.pop_back();
            fseek(fp, start_block * BLOCK_SIZE + sizeof(uint32_t) * int_id, SEEK_SET);
            fwrite(element + int_id, sizeof(uint32_t), 1, fp);
        }
    }

    void clear() {
        memset(element, 0, sizeof element);
        need_update.clear();
    }

    bool get(int i) {
        return element[i >> 5] >> (i & 0x1F) & 1;
    }

    bool set(int i) {
        need_update.push_back(i >> 5);
        return (element[i >> 5] |= (1 << (i & 0x1F)));
    }

    bool toggle(int i) {
        need_update.push_back(i >> 5);
        return (element[i >> 5] ^= (1 << (i & 0x1F)));
    }

    int count() {
        int count = 0;
        for (int i = 0; i < NUM_INTS_BITMAP; ++i) {
            count += __builtin_popcount(element[i]);
        }
        return count;
    }

    uint16_t find_free() {
        for (int i = 0; i < 32 * NUM_INTS_BITMAP; i++) {
            if (get(i) == 0) {
                set(i);
                return i;
            }
        }

        return NON_EXIST_CONSTANT;
    }
};

struct imap_t : bitmap_t {
    imap_t(): bitmap_t(IMAP_START_BLOCK) {}
};

struct dmap_t : bitmap_t {
    dmap_t(): bitmap_t(DMAP_START_BLOCK) {}
};

#endif