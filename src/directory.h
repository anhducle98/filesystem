#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

#include <stdio.h>
#include <vector>
#include <string>

#include "constants.h"
#include "inode.h"

struct directory_t {
    inode_t inode;
    vector< pair<uint16_t, string> > a;

    directory_t(bitmap_t *imap, bitmap_t *dmap) {
        inode.imap = imap;
        inode.dmap = dmap;
    }

    void read_children_list(FILE *fp) {
        vector<uint8_t> buffer;
        uint8_t block_buffer[BLOCK_SIZE];
        for (uint16_t data_block_id : inode.data_blocks_ids) {
            fseek(fp, START_BYTE_OF_DATA_REGION + data_block_id * BLOCK_SIZE, SEEK_SET);
            fread(block_buffer, sizeof(uint8_t), BLOCK_SIZE, fp);
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                buffer.push_back(block_buffer[i]);
            }
        }
  
        a.clear();
        int cur_pos = 0;
        while (cur_pos < buffer.size()) {
            uint16_t inum_child = buffer[cur_pos + 1] << 8 | buffer[cur_pos];
            cur_pos += 2;
            string file_name;
            while (buffer[cur_pos] != 0) {
                file_name += (char)buffer[cur_pos];
                cur_pos += 1;
            }
            a.push_back(make_pair(inum_child, file_name));
            cur_pos += 1;
            if (cur_pos == buffer.size() || buffer[cur_pos] == 0) break;
        }
    }

    void read_from_disk(FILE *fp, uint16_t inum) {
        inode.read_from_disk(fp, inum);
        assert(inode.type == inode_t::TYPE_DIRECTORY);
        read_children_list(fp);
    }

    uint32_t serialize(uint8_t *&res) {
        int num_bytes = get_byte_array_size();

        res = new uint8_t[num_bytes];
        int cur_pos = 0;
        for (auto it : a) {
            res[cur_pos] = it.first & 0xFF;
            res[cur_pos + 1] = it.first >> 8;
            cur_pos += 2;
            for (int i = 0; i < it.second.size(); ++i) {
                res[cur_pos++] = (uint8_t)it.second[i];
            }
            res[cur_pos++] = 0;
        }
        res[cur_pos++] = 0;
        assert(cur_pos == num_bytes);
        return num_bytes;
    }

    void write_to_disk(FILE *fp) {
        uint8_t *byte_array = nullptr;
        uint32_t total_size = serialize(byte_array);

        inode.push_more_data(total_size - inode.size);
        inode.write_to_disk(fp);

        uint32_t cur_pos = 0;

        for (uint16_t data_block_id : inode.data_blocks_ids) {
            fseek(fp, START_BYTE_OF_DATA_REGION + data_block_id * BLOCK_SIZE, SEEK_SET);
            fwrite(byte_array + cur_pos, sizeof(uint8_t), min(BLOCK_SIZE, total_size - cur_pos), fp);
            cur_pos += BLOCK_SIZE;
        }
        delete byte_array;
    }

    bool contains(string name) {
        for (const auto &it : a) {
            if (it.second == name) {
                return true;
            }
        }
        return false;
    }

    uint16_t get_inum_of_child(string child) {
   		for (int i = 0; i < a.size(); i++) {
   			if (a[i].second == child) {
   				return a[i].first;
   			}
   		}
   		return NON_EXIST_CONSTANT;
   	}

    uint32_t get_byte_array_size() {
        int num_bytes = 0;
        for (auto &it : a) {
            num_bytes += 2;
            num_bytes += it.second.size() + 1; // +1 for ending byte '\0'
        }
        num_bytes += 1;
        return num_bytes;
    }
};

#endif