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
const uint32_t IMAP_START_BLOCK = 1;
const uint32_t DMAP_START_BLOCK = IMAP_START_BLOCK + NUM_BLOCK_BITMAP;
const uint8_t POINTER_SIZE = 2;
const uint16_t ROOT_INUM = 0;

const uint16_t NON_EXIST_CONSTANT = (uint16_t)(-1);

const uint32_t NUM_DIRECT_POINTERS = 11;
const uint32_t NUM_INTS_BITMAP = 1 << 10;

const uint32_t BUFFER_SIZE = 1 << 9; // TODO, remove this

struct bitmap_t {
    uint32_t element[NUM_INTS_BITMAP];
    vector<uint32_t> need_update;
    int start_block; // imap starts from 1, dmap starts from 9

    bitmap_t(int start_block): start_block(start_block) {}

    void read_from_disk(FILE *fp) {
        fseek(fp, start_block * BLOCK_SIZE, SEEK_SET);
        fread(element, sizeof(uint32_t), NUM_INTS_BITMAP, fp);
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

    void write_to_disk(FILE *fp) {
        while (!need_update.empty()) {
            uint32_t int_id = need_update.back();
            need_update.pop_back();
            fseek(fp, start_block * BLOCK_SIZE + sizeof(uint32_t) * int_id, SEEK_SET);
            //printf("bit_map_t::write_to_disk(), start_block=%d, int_id=%d\n", start_block, int_id);
            fwrite(element + int_id, sizeof(uint32_t), 1, fp);
        }
    }
} imap(IMAP_START_BLOCK), dmap(DMAP_START_BLOCK);

struct inode_t {
    static const uint8_t TYPE_DIRECTORY = 1;

    uint32_t size;
    uint16_t block_count;
    uint8_t type;
    pointer_t pointers[NUM_DIRECT_POINTERS + 1]; // count from start of data region

    uint32_t inum;

    vector< pointer_t > data_blocks_ids;

    void seek(FILE *fp) {
        uint32_t block_id = 1 + NUM_BLOCK_BITMAP * 2 + inum / NUM_INODE_EACH_BLOCK;
        uint32_t id_in_block = inum % NUM_INODE_EACH_BLOCK;
        fseek(fp, block_id * BLOCK_SIZE + id_in_block * INODE_SIZE, SEEK_SET);
        //printf("seek(), block_id=%d, id_in_block=%d\n", block_id, id_in_block);
    }

    void read_from_disk(FILE *fp, uint32_t inum) { // inum start from beginning of inode table
        //printf("start reading inode, inum=%d\n", inum);
        //memset(pointers, 1, sizeof(pointers));
        //printf("inum : %d\n", inum);
        for (int i = 0; i <= NUM_DIRECT_POINTERS; i++) {
            pointers[i] = NON_EXIST_CONSTANT;
        }

        this->inum = inum;
        seek(fp);

        fread(&size, sizeof size, 1, fp);
        fread(&block_count, sizeof block_count, 1, fp);
        fread(&type, sizeof type, 1, fp);
        fread(pointers, sizeof(pointer_t), NUM_DIRECT_POINTERS + 1, fp);

        printf("pointer : ");
        for (int i = 0; i <= NUM_DIRECT_POINTERS; i++){
            printf("%d ", pointers[i]);
        }
        printf("\n");
        printf("size = %d | block_count = %d | type = %d\n", size, block_count, type);

        //printf("size=%u, type=%d\n", size, type);

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
        //printf("inode_t::write_to_distk(), inum=%d\n", inum);
        imap.set(inum);

        seek(fp);

        fwrite(&size, sizeof size, 1, fp);
        fwrite(&block_count, sizeof block_count, 1, fp);
        fwrite(&type, sizeof type, 1, fp);
        fwrite(pointers, sizeof(pointer_t), NUM_DIRECT_POINTERS + 1, fp);
        printf("BLOCK_COUNT = %d \n", block_count);

        if (block_count > NUM_DIRECT_POINTERS) {
            fseek(fp, START_BYTE_OF_DATA_REGION + pointers[NUM_DIRECT_POINTERS] * BLOCK_SIZE, SEEK_SET);
            fwrite(data_blocks_ids.data() + NUM_DIRECT_POINTERS * sizeof(pointer_t), sizeof(pointer_t), data_blocks_ids.size() - NUM_DIRECT_POINTERS, fp);
        }

        for (uint16_t block_id : data_blocks_ids) {
            dmap.set(block_id);
        }

        imap.write_to_disk(fp);
        dmap.write_to_disk(fp);
    }

    void push_more_data(uint32_t more) { // more = num_bytes more
        
        

        size += more;
        int nblock = size / BLOCK_SIZE + (size % BLOCK_SIZE != 0);
        while (block_count < nblock) {
            block_count += 1;
            uint16_t new_block = dmap.find_free();
            data_blocks_ids.push_back(new_block);
        }

        for (int i = 0; i < data_blocks_ids.size() && i < NUM_DIRECT_POINTERS; ++i) {
            pointers[i] = data_blocks_ids[i];
        }

        

        
        if (data_blocks_ids.size() > NUM_DIRECT_POINTERS) {
            uint16_t indirect_block = pointers[NUM_DIRECT_POINTERS];

            
           // if (indirect_block == NON_EXIST_CONSTANT) {
                indirect_block = dmap.find_free();    
                pointers[NUM_DIRECT_POINTERS] = indirect_block;
           // }
           printf("INDIRECT_BLOCK : %d \n", indirect_block);
        }
    }
};

struct directory_t {
    inode_t inode;
    vector< pair<uint16_t, string> > a;

    void read_children_list(FILE *fp) {
        vector<uint8_t> buffer;
        uint8_t block_buffer[BLOCK_SIZE];
        for (pointer_t data_block_id : inode.data_blocks_ids) {
            fseek(fp, START_BYTE_OF_DATA_REGION + data_block_id * BLOCK_SIZE, SEEK_SET);
            fread(block_buffer, sizeof(uint8_t), BLOCK_SIZE, fp);
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                buffer.push_back(block_buffer[i]);
            }
        }
/*
        printf("buffer=");
        for (int i = 0; i < buffer.size(); ++i) {
            printf("%02x ", buffer[i]);
        }
        printf("\n");
*/      
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
            //printf("new element inum=%d, name=%s, cur_pos=%d\n", inum_child, file_name.c_str(), cur_pos);
            a.push_back(make_pair(inum_child, file_name));
            cur_pos += 1;
            if (cur_pos == buffer.size() || buffer[cur_pos] == 0) break;
        }
        //printf("a.size()=%d\n", (int)a.size());
    }

    void read_from_disk(FILE *fp, uint16_t inum) {
        inode.read_from_disk(fp, inum);
        //printf("directory_t, done read inode, inum=%d\n", inum);
        assert(inode.type == inode_t::TYPE_DIRECTORY);
        read_children_list(fp);
    }

    uint16_t get_inum_of_child(string child) {
   		for(int i = 0; i < a.size(); i++) {
   			if(a[i].second == child){
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

    uint32_t serialize(uint8_t *&res) {
        int num_bytes = get_byte_array_size();

        res = new uint8_t[num_bytes];
        int cur_pos = 0;
        for (auto &it : a) {
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

        /*printf("directory_t::write_to_disk(), total_size=%d\n", total_size);
        printf("byte_array=");
        for (int i = 0; i < total_size; ++i) {
            printf("%02x ", byte_array[i]);
        }
        printf("\n");*/

        inode.write_to_disk(fp);

       // printf("directory_t::write_to_disk(), write inode success!, inode.data_block_ids.size()=%d, inode.size=%d, inode.block_count=%d\n", (int)inode.data_blocks_ids.size(), inode.size, inode.block_count);
        uint32_t cur_pos = 0;
        for (pointer_t data_block_id : inode.data_blocks_ids) {
         //   printf("data_block_id=%d\n", data_block_id);
            fseek(fp, START_BYTE_OF_DATA_REGION + data_block_id * BLOCK_SIZE, SEEK_SET);
            fwrite(byte_array + cur_pos, sizeof(uint8_t), min(BLOCK_SIZE, total_size - cur_pos), fp);
            cur_pos += BLOCK_SIZE;
        }
        //printf("end\n");
        delete byte_array;
    }
};

void create_empty_directory(FILE *fp, const char *name, uint16_t parent_inum) {
    directory_t dir;
    if (parent_inum == NON_EXIST_CONSTANT) {
        // create root
        dir.inode.inum = ROOT_INUM;
        dir.inode.size = 0;
        dir.inode.block_count = 0;
        dir.inode.type = inode_t::TYPE_DIRECTORY;
        dir.a.push_back(make_pair(0, "."));
        dir.write_to_disk(fp);

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

    create_empty_directory(fp, "/", -1);
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

    imap.read_from_disk(fp);
    dmap.read_from_disk(fp);

    printf("NUM_INODE = %d\n", imap.count());
    printf("NUM_DATA_BLOCKS = %d\n", dmap.count());

    fclose(fp);
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

    while (1) {
        int num_read = fread(buffer, 1, BUFFER_SIZE, fp);
        for (int i = 0; i < num_read; ++i) {
            printf("%02x ", buffer[i]);
        }
        printf("\n");
        if (num_read < BUFFER_SIZE) break;
    }

    fclose(fp);
}

directory_t get_dir_from_path(vector < string > dir, FILE *fp) {
	directory_t cur_dir;
    cur_dir.read_from_disk(fp, ROOT_INUM);
    //printf("read from disk\n");
	for(int i = 0; i < dir.size(); i++) {
        uint16_t inum = cur_dir.get_inum_of_child(dir[i]);
        assert(inum != NON_EXIST_CONSTANT);
		cur_dir.read_from_disk(fp, inum);
	}
	return cur_dir;

}

vector< string > split_path(string path) {
    vector< string > dir;
	for (int i = 0; i < path.size();) {
		int pos = i;
		while (path[pos] != '/' && pos < path.size())
			pos++;
		if (i != pos)
			dir.push_back(path.substr(i, pos - i));
		i = pos + 1;
    }
    return dir;
}

void dfs(FILE *fp, uint16_t inum, string cur_name = "/", int level = 0) {
    inode_t inode;
    inode.read_from_disk(fp, inum);

    for (int i = 0; i < level; ++i) {
        printf("-");
    }
    printf("| %s, inum=%d, size=%d\n", cur_name.c_str(), inode.inum, inode.size);
    //return;

    if (inode.type == inode_t::TYPE_DIRECTORY) {
        directory_t dir;
        dir.inode = inode;
        dir.read_children_list(fp);
        //printf("dfs(), children_list.size() = %d\n", (int)dir.a.size());
        for (auto it : dir.a) {
            //printf("child inum=%d, name=%s\n", it.first, it.second.c_str());
            if (it.first != inum && it.second != "..") { // not me, not my parent
                dfs(fp, it.first, it.second, level + 1);
            }
        }
    }
}

void list_disk() {
    FILE *fp = fopen(DEFAULT_DISK, "rb");
    dfs(fp, ROOT_INUM);
    fclose(fp);
}

void copy_file_from_outside(string path, string file_name) {
	FILE *fp = fopen(DEFAULT_DISK, "r+b");

	vector<string> dir = split_path(path);
    directory_t cur_dir = get_dir_from_path(dir, fp);
    
    

	uint16_t new_inum = imap.find_free();
	inode_t new_inode;

	new_inode.inum = new_inum;
	new_inode.block_count = 0;
    new_inode.size = 0;
    

	FILE *cur_file = fopen(file_name.c_str(), "rb");

    uint8_t buffer[BLOCK_SIZE];
    memset(buffer, 0, sizeof(buffer));

	while (true) {
        uint32_t num_read = fread(buffer, 1, BLOCK_SIZE, cur_file);
        assert(num_read <= BLOCK_SIZE);
        
        new_inode.push_more_data(num_read);
        //printf("num_read=%d, last_block_id=%d\n", num_read, (int)new_inode.data_blocks_ids.back());
		fseek(fp, START_BYTE_OF_DATA_REGION + new_inode.data_blocks_ids.back() * BLOCK_SIZE, SEEK_SET);
		fwrite(buffer, sizeof(uint8_t), num_read, fp);
        if (num_read < BLOCK_SIZE) break;
    }
    fclose(cur_file);

    new_inode.write_to_disk(fp);
    cur_dir.a.push_back(make_pair(new_inode.inum, file_name));
    cur_dir.write_to_disk(fp);

    fclose(fp);
    

}

void copy_file_to_outside(string path, string copy_file_name, string paste_file_name){
    FILE *fp = fopen(DEFAULT_DISK, "rb");
    FILE *paste_file = fopen(paste_file_name.c_str(), "wb");

    vector < string > dir = split_path(path);
    directory_t cur_dir = get_dir_from_path(dir, fp);

    uint16_t inum = cur_dir.get_inum_of_child(copy_file_name);
    printf("inum of copy file %d \n", inum);

    inode_t inode;
    inode.read_from_disk(fp, inum);

    uint8_t block_buffer[BLOCK_SIZE];
    memset(block_buffer, 0, sizeof(block_buffer));

    //printf("size of file = %d nbblocks = %d \n", inode.size, inode.block_count);
    uint32_t size_file = inode.size;

    for(int i = 0; i < inode.data_blocks_ids.size(); i++) {
        pointer_t block_id = inode.data_blocks_ids[i];
        printf("block_id = %d \n", block_id);
        
        fseek(fp, START_BYTE_OF_DATA_REGION + block_id * BLOCK_SIZE, SEEK_SET);
        fread(block_buffer, sizeof(uint8_t), BLOCK_SIZE, fp);
        fwrite(block_buffer, sizeof(uint8_t), min(size_file, BLOCK_SIZE), paste_file);

        size_file -= BLOCK_SIZE;
    }


    fclose(fp);
    fclose(paste_file);




}

int main() {
    create_empty_disk("HD.dat");
  //  read_disk_info();
    //read_all_bytes("HD.dat");

    copy_file_from_outside("/", "os.txt");
    //read_disk_info();
   // list_disk();

   // copy_file_from_outside("/", "ahihi.cpp");

    //list_disk();
    copy_file_to_outside("/", "os.txt", "copyfile.txt");
    return 0;
}