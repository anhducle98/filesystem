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


#include <bits/stdc++.h>

using namespace std;

typedef uint16_t pointer_t;

const char *DEFAULT_DISK = "HD.dat";

const uint32_t DISK_SIZE = 16 * 1024 * 1024;
const uint32_t BLOCK_SIZE = 512;
const uint32_t INODE_SIZE = 32;
const uint32_t NUM_BLOCK_DISK = DISK_SIZE / BLOCK_SIZE;
const uint8_t NUM_BLOCK_BITMAP = 8;
const uint32_t NUM_BYTE_INODE_TABLE = NUM_BLOCK_BITMAP * BLOCK_SIZE * 8 * 32;
const uint32_t NUM_INODE_EACH_BLOCK = BLOCK_SIZE / INODE_SIZE;
const uint32_t NUM_BLOCK_INODE_TABLE = NUM_BYTE_INODE_TABLE / BLOCK_SIZE;
const uint32_t START_BLOCK_OF_DATA_REGION = 1 + NUM_BLOCK_BITMAP * 2 + NUM_BLOCK_INODE_TABLE;
const uint32_t START_BYTE_OF_DATA_REGION = START_BLOCK_OF_DATA_REGION * BLOCK_SIZE;

const uint8_t POINTER_SIZE = 2;
const uint16_t ROOT_INUM = 0;



const uint32_t NUM_DIRECT_POINTERS = 11;
const uint32_t NUM_INTS_BITMAP = 1 << 10;

const uint32_t BUFFER_SIZE = 1 << 9; // TODO, remove this

struct inode_t {
    static const uint8_t TYPE_DIRECTORY = 1;

    uint32_t size;
    uint16_t block_count;
    uint8_t type;
    pointer_t pointers[NUM_DIRECT_POINTERS + 1]; // count from start of disk

    vector< pointer_t > data_blocks_ids;

    void read_from_disk(FILE *fp, uint32_t inum) { // inum start from beginning of disk
        uint32_t block_id = 1 + NUM_BLOCK_BITMAP * 2 + inum / NUM_INODE_EACH_BLOCK;
        uint32_t id_in_block = inum % NUM_INODE_EACH_BLOCK;
        fseek(fp, block_id * BLOCK_SIZE + id_in_block * INODE_SIZE, SEEK_SET);
        fread(&size, sizeof size, 1, fp);
        fread(&block_count, sizeof block_count, 1, fp);
        fread(&type, sizeof type, 1, fp);
        fread(pointers, sizeof(pointer_t), NUM_DIRECT_POINTERS + 1, fp);

        for (int i = 0; i < block_count; ++i) {
            if (i < NUM_DIRECT_POINTERS) {
                data_blocks_ids.push_back(pointers[i]);
            } else {
                fseek(fp, pointers[NUM_DIRECT_POINTERS] * BLOCK_SIZE, SEEK_SET);
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
        
    }
};

struct directory_t {
    vector< pair<uint16_t, string> > a;

    void read_from_disk(FILE *fp, uint16_t inum) {
        inode_t inode;
        inode.read_from_disk(fp, inum);
        assert(inode.type == inode_t::TYPE_DIRECTORY);

        vector<uint8_t> buffer;
        uint8_t block_buffer[BLOCK_SIZE];
        for (pointer_t data_block_id : inode.data_blocks_ids) {
            fseek(fp, data_block_id * BLOCK_SIZE, SEEK_SET);
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
            if (cur_pos + 1 == buffer.size() || buffer[cur_pos + 1] == 0) break;
        }
    }
};

struct bitmap_t {
    uint32_t element[NUM_INTS_BITMAP];

    void read_from_disk(FILE *fp, int start_block) { // imap starts from 1, dmap starts from 2
        fseek(fp, start_block * BLOCK_SIZE, SEEK_SET);
        fread(element, sizeof(uint32_t), NUM_INTS_BITMAP, fp);
    }

    bool get(int i) {
        return element[i >> 5] >> (i & 0x1F) & 1;
    }

    bool set(int i) {
        return (element[i >> 5] |= (1 << (i & 0x1F)));
    }

    bool toggle(int i) {
        return (element[i >> 5] ^= (1 << (i & 0x1F)));
    }

    int count() {
        int count = 0;
        for (int i = 0; i < NUM_INTS_BITMAP; ++i) {
            count += __builtin_popcount(element[i]);
        }
        return count;
    }
} imap, dmap;

void create_empty_directory(FILE *fp, const char *name, uint16_t parent_inum) {
    if (parent_inum == -1) {
        // create root
        assert(imap.get(ROOT_INUM) == 0);
        imap.set(ROOT_INUM);
        inode_t inode;
        inode.size = 0;
        inode.block_count = 1;
        inode.type = inode_t::TYPE_DIRECTORY;
        inode.pointers[0] = START_BLOCK_OF_DATA_REGION; // hardcode this number;
        inode.write_to_disk(fp);
    } else {
        // TODO
    }
}

void create_empty_disk(const char *file_name) {
    FILE *fp = fopen(file_name, "wb");

    fwrite(&DISK_SIZE, sizeof DISK_SIZE, 1, fp);
    fwrite(&BLOCK_SIZE, sizeof BLOCK_SIZE, 1, fp);
    fwrite(&NUM_BLOCK_BITMAP, 1, 1, fp);
    fwrite(&NUM_BLOCK_BITMAP, 1, 1, fp);
    fwrite(&POINTER_SIZE, 1, 1, fp);

    // jump to second block to write inode table and data region
    // everything is zeros initialy
    fseek(fp, BLOCK_SIZE, SEEK_SET);
    uint8_t buffer[BLOCK_SIZE] = {0};
    for (int i = 1; i < NUM_BLOCK_DISK; ++i) {
        fwrite(buffer, 1, BLOCK_SIZE, fp);
    }
    fclose(fp);
}

void read_disk_info(const char *file_name = DEFAULT_DISK) {
    FILE *fp = fopen(file_name, "rb");

    uint32_t disk_size;
    uint32_t block_size;
    uint8_t num_block_imap;
    uint8_t num_block_dmap;
    uint8_t pointer_size; 

    fread(&disk_size, sizeof disk_size, 1, fp);
    fread(&block_size, sizeof block_size, 1, fp);
    fread(&num_block_imap, sizeof num_block_imap, 1, fp);
    fread(&num_block_dmap, sizeof num_block_dmap, 1, fp);
    fread(&pointer_size, sizeof pointer_size, 1, fp);

    printf("DISK_SIZE = %d\n", disk_size);
    printf("BLOCK_SIZE = %d\n", block_size);
    printf("NUM_BLOCK_IMAP = %d\n", num_block_imap);
    printf("NUM_BLOCK_DMAP = %d\n", num_block_dmap);
    printf("POINTER_SIZE = %d\n", pointer_size);

    imap.read_from_disk(fp, 1);
    dmap.read_from_disk(fp, 2);

    printf("NUM_INODE = %d\n", imap.count());
    printf("NUM_DATA_BLOCKS = %d\n", dmap.count());

    fclose(fp);
}

void create_empty_file(const ) {

}

void create_zeros_file(const char *file_name, uint32_t num_bytes) {
    FILE *fp = fopen(file_name, "wb");
    unsigned char zeros_buffer[BUFFER_SIZE] = {0};

    for (int i = 0; i < num_bytes; i += BUFFER_SIZE) {
        fwrite(zeros_buffer, 1, min(num_bytes - i, BUFFER_SIZE), fp);
    }

    fclose(fp);
}

void create_random_file(const char *file_name, uint32_t num_bytes) {
    FILE *fp = fopen(file_name, "wb");
    unsigned char buffer[BUFFER_SIZE] = {0};

    for (int i = 0; i < num_bytes; i += BUFFER_SIZE) {
        // create random buffer
        for (int j = 0; j < BUFFER_SIZE; ++j) {
            buffer[j] = rand() % (1 << 8); // 1 byte = 8 bit
        }
        fwrite(buffer, 1, min(num_bytes - i, BUFFER_SIZE), fp);
    }

    fclose(fp);
}

void read_all_bytes(const char *file_name) {
    FILE *fp = fopen(file_name, "rb");
    unsigned char buffer[BUFFER_SIZE];

    while (!feof(fp)) {
        int num_read = fread(buffer, 1, BUFFER_SIZE, fp);
        for (int i = 0; i < num_read; ++i) {
            printf("%02x ", buffer[i]);
        }
        printf("\n");
    }

    fclose(fp);
}



int main() {
    //create_empty_disk("HD.dat");
    read_disk_info();
    //read_all_bytes("HD.dat");

    return 0;
}