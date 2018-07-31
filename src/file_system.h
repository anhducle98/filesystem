#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include <stdio.h>

#include "constants.h"
#include "bitmap.h"
#include "inode.h"
#include "directory.h"

struct file_system_t {
    imap_t imap;
    dmap_t dmap;
    FILE *fp;

    file_system_t(const char *file_name = DEFAULT_DISK) {
        fp = fopen(file_name, "r+b");
        if (fp == nullptr) {
            create_empty_disk(file_name);
            fp = fopen(file_name, "r+b");
        }
        assert(fp != nullptr);

        read_disk_info();
    }

    void create_empty_disk(const char *file_name = DEFAULT_DISK) {
        if (fp != nullptr) {
            fclose(fp);
        }
        fp = fopen(file_name, "wb");

        fwrite(&DISK_SIZE, sizeof DISK_SIZE, 1, fp);
        fwrite(&BLOCK_SIZE, sizeof BLOCK_SIZE, 1, fp);
        fwrite(&NUM_BLOCK_BITMAP, 1, 1, fp);
        fwrite(&NUM_BLOCK_BITMAP, 1, 1, fp);
        fwrite(&POINTER_SIZE, 1, 1, fp);

        // jump to second block to write inode table and data region
        // everything is zeros initially
        fseek(fp, BLOCK_SIZE, SEEK_SET);
        uint8_t buffer[BLOCK_SIZE] = {0};
        for (int i = 1; i < NUM_BLOCK_DISK; ++i) {
            fwrite(buffer, 1, BLOCK_SIZE, fp);
        }

        imap.clear();
        dmap.clear();
        create_empty_directory("/", -1);

        fclose(fp);

        fp = fopen(file_name, "r+b");
    }

    void read_disk_info() {
        uint32_t disk_size;
        uint32_t block_size;
        uint8_t num_block_imap;
        uint8_t num_block_dmap;
        uint8_t pointer_size; 

        fseek(fp, 0, SEEK_SET);
        fread(&disk_size, sizeof disk_size, 1, fp);
        fread(&block_size, sizeof block_size, 1, fp);
        fread(&num_block_imap, sizeof num_block_imap, 1, fp);
        fread(&num_block_dmap, sizeof num_block_dmap, 1, fp);
        fread(&pointer_size, sizeof pointer_size, 1, fp);

        printf("\n# DISK INFORMATION:\n");
        printf("# DISK_SIZE = %d\n", disk_size);
        printf("# BLOCK_SIZE = %d\n", block_size);
        printf("# NUM_BLOCK_IMAP = %d\n", num_block_imap);
        printf("# NUM_BLOCK_DMAP = %d\n", num_block_dmap);
        printf("# POINTER_SIZE = %d\n", pointer_size);

        imap.read_from_disk(fp);
        dmap.read_from_disk(fp);

        printf("# NUM_INODE = %d\n", imap.count());
        printf("# NUM_DATA_BLOCKS = %d\n\n", dmap.count());
    }

    void read_all_bytes() {
        uint8_t buffer[BLOCK_SIZE];
        while (1) {
            int num_read = fread(buffer, 1, BLOCK_SIZE, fp);
            for (int i = 0; i < num_read; ++i) {
                printf("%02x ", buffer[i]);
            }
            printf("\n");
            if (num_read < BLOCK_SIZE) break;
        }
    }

    vector< string > split_path(string path, bool ommit_last_entry = false) {
        vector< string > dir;
        for (int i = 0; i < path.size();) {
            int pos = i;
            while (path[pos] != '/' && pos < path.size())
                pos++;
            if (i != pos)
                dir.push_back(path.substr(i, pos - i));
            i = pos + 1;
        }
        if (ommit_last_entry) {
            if (!dir.empty()) {
                dir.pop_back();
            }
        }
        return dir;
    }

    directory_t get_dir_from_path(vector< string > dir) {
        directory_t cur_dir(&imap, &dmap);
        cur_dir.read_from_disk(fp, ROOT_INUM);
        for (int i = 0; i < dir.size(); i++) {
            uint16_t inum = cur_dir.get_inum_of_child(dir[i]);
            assert(inum != NON_EXIST_CONSTANT);
            cur_dir.read_from_disk(fp, inum);
        }
        return cur_dir;
    }

    void create_empty_directory(string dir_name, uint16_t parent_inum) {
        directory_t cur_dir(&imap, &dmap);
        if (parent_inum == NON_EXIST_CONSTANT) {
            // create root
            imap.set(ROOT_INUM);
            cur_dir.inode.inum = ROOT_INUM;
            cur_dir.inode.size = 0;
            cur_dir.inode.block_count = 0;
            cur_dir.inode.type = inode_t::TYPE_DIRECTORY;
            cur_dir.a.push_back(make_pair(0, "."));
            cur_dir.write_to_disk(fp);
        } else {        
            cur_dir.inode.inum = imap.find_free();
            cur_dir.inode.size = cur_dir.inode.block_count = 0;
            cur_dir.inode.type = inode_t::TYPE_DIRECTORY;
            cur_dir.a.push_back(make_pair(parent_inum, ".."));
            cur_dir.a.push_back(make_pair(cur_dir.inode.inum, "."));
            
            directory_t parent_dir(&imap, &dmap);
            
            parent_dir.read_from_disk(fp, parent_inum);
            parent_dir.a.push_back(make_pair(cur_dir.inode.inum, dir_name));
        
            parent_dir.write_to_disk(fp);
            cur_dir.write_to_disk(fp);
        }
    }

    void create_empty_folder(string path, string folder_name) {
        FILE *fp = fopen(DEFAULT_DISK, "r+b");

        vector < string > dir = split_path(path);
        directory_t cur_dir = get_dir_from_path(dir);

        if (cur_dir.contains(folder_name)) {
            printf("Folder exist! Do nothing\n");
            return;
        }
        
        create_empty_directory(folder_name, cur_dir.inode.inum);
        fclose(fp);
    }

    void dfs(uint16_t inum, string cur_name = "/", int level = 0) {
        inode_t inode(&imap, &dmap);
        inode.read_from_disk(fp, inum);

        for (int i = 0; i < level; ++i) {
            printf("----");
        }
        printf("| \"%s\" - [%d bytes]\n", cur_name.c_str(), inode.size);

        if (inode.type == inode_t::TYPE_DIRECTORY) {
            directory_t dir(&imap, &dmap);
            dir.inode = inode;
            dir.read_children_list(fp);
            for (auto it : dir.a) {
                if (it.first != inum && it.second != "..") { // not me, not my parent
                    dfs(it.first, it.second, level + 1);
                }
            }
        }
    }

    void list_disk() {
        dfs(ROOT_INUM);
    }

    void view_text_file(string file_path) {
        vector<string> dir = split_path(file_path);
        string file_name = dir.back();
        dir.pop_back();

        inode_t inode(&imap, &dmap);
        inode.read_from_disk(fp, get_dir_from_path(dir).get_inum_of_child(file_name));
        printf("\n# START READING FILE %s\n", file_path.c_str());
        uint8_t buffer[BLOCK_SIZE];
        int size_remain = inode.size;
        for (uint16_t data_block_id : inode.data_blocks_ids) {
            fseek(fp, START_BYTE_OF_DATA_REGION + data_block_id * BLOCK_SIZE, SEEK_SET);
            int num_read = min((int)BLOCK_SIZE, size_remain);
            fread(buffer, 1, num_read, fp);
            size_remain -= num_read;
            for (int i = 0; i < num_read; ++i) {
                printf("%c", (char) buffer[i]);
            }
        }
        printf("\n# END OF FILE %s\n", file_path.c_str());
    }

    bool is_directory(uint16_t inum) {
        inode_t inode;
        inode.read_from_disk(fp, inum);
        return inode.type == inode_t::TYPE_DIRECTORY;
    }

    void copy_file_from_outside(string path, string file_name) {
        FILE *cur_file = fopen(file_name.c_str(), "rb");
        if (cur_file == nullptr) {
            printf("\n# ERROR: Cannot find file %s\n", file_name.c_str());
            return;
        }

        vector<string> dir = split_path(path);
        directory_t cur_dir = get_dir_from_path(dir);
        
        if (cur_dir.contains(file_name)) {
            printf("# ERROR: File exist! Do nothing\n");
            return;
        }

        file_name = split_path(file_name).back();

        uint16_t new_inum = imap.find_free();
        inode_t new_inode(&imap, &dmap);

        new_inode.inum = new_inum;
        new_inode.block_count = 0;
        new_inode.size = 0;

        uint8_t buffer[BLOCK_SIZE];
        memset(buffer, 0, sizeof(buffer));

        while (true) {
            uint32_t num_read = fread(buffer, 1, BLOCK_SIZE, cur_file);
            assert(num_read <= BLOCK_SIZE);
            new_inode.push_more_data(num_read);
            fseek(fp, START_BYTE_OF_DATA_REGION + new_inode.data_blocks_ids.back() * BLOCK_SIZE, SEEK_SET);
            fwrite(buffer, sizeof(uint8_t), num_read, fp);
            if (num_read < BLOCK_SIZE) break;
        }
        fclose(cur_file);

        new_inode.write_to_disk(fp);
        cur_dir.a.push_back(make_pair(new_inode.inum, file_name));
        cur_dir.write_to_disk(fp);
        
        printf("\n# INFO: File %s imported to %s\n", file_name.c_str(), path.c_str());
    }

    void copy_file_to_outside(string path, string copy_file_name, string paste_file_name) {
        FILE *paste_file = fopen(paste_file_name.c_str(), "wb");

        vector < string > dir = split_path(path);
        directory_t cur_dir = get_dir_from_path(dir);

        uint16_t inum = cur_dir.get_inum_of_child(copy_file_name);

        inode_t inode(&imap, &dmap);
        inode.read_from_disk(fp, inum);

        uint8_t block_buffer[BLOCK_SIZE];
        memset(block_buffer, 0, sizeof(block_buffer));
    
        uint32_t size_file = inode.size;

        for (int i = 0; i < inode.data_blocks_ids.size(); i++) {
            uint16_t block_id = inode.data_blocks_ids[i];
            
            fseek(fp, START_BYTE_OF_DATA_REGION + block_id * BLOCK_SIZE, SEEK_SET);
            fread(block_buffer, sizeof(uint8_t), BLOCK_SIZE, fp);
            fwrite(block_buffer, sizeof(uint8_t), min(size_file, BLOCK_SIZE), paste_file);

            size_file -= BLOCK_SIZE;
        }

        fclose(paste_file);
        printf("\n# INFO: File %s exported to %s\n", copy_file_name.c_str(), paste_file_name.c_str());
    }

    void delete_directory(uint16_t inum) {
        inode_t inode(&imap, &dmap);
        inode.read_from_disk(fp, inum);
        if (inode.type == inode_t::TYPE_DIRECTORY) {
            directory_t cur_dir;
            cur_dir.inode = inode;
            cur_dir.read_children_list(fp);

            for (const auto &it : cur_dir.a) {
                if (it.second != "." && it.second != "..") {
                    delete_directory(it.first);
                }
            }

            cur_dir.inode.delete_myself();
        } else {
            inode.delete_myself();
        }
    }

    void delete_file(string path, string file_name) {
        vector < string > dir = split_path(path);
        directory_t cur_dir = get_dir_from_path(dir);
        uint16_t inum = cur_dir.get_inum_of_child(file_name);

        if (inum == NON_EXIST_CONSTANT) {
            printf("\n# ERROR: Cannot find file %s\n", file_name.c_str());
            return;
        }

        delete_directory(inum);

        cur_dir.delete_entry(file_name);
        cur_dir.write_to_disk(fp);

        printf("\n# INFO: File %s deleted\n", file_name.c_str());
    }

    void move_entry(string from_path, string to_path) {
        vector< string > from_splitted = split_path(from_path);
        vector< string > to_splitted = split_path(to_path);
        string from_name = from_splitted.back();
        string to_name = to_splitted.back();
        from_splitted.pop_back();
        to_splitted.pop_back();

        directory_t from_dir = get_dir_from_path(from_splitted);
        directory_t to_dir = get_dir_from_path(to_splitted);
        if (!from_dir.contains(from_name)) {
            printf("\n# ERROR: File %s doesn't exist!\n", from_path.c_str());
            return;
        }
        if (to_dir.contains(to_name)) {
            printf("\n# ERROR: File %s exist!\n", to_path.c_str());
            return;
        }
        if (from_dir.inode.inum == to_dir.inode.inum) {
            from_dir.rename(from_name, to_name);
            from_dir.write_to_disk(fp);
        } else {
            uint16_t inum = from_dir.delete_entry(from_name);        
            to_dir.a.push_back(make_pair(inum, to_name));
            from_dir.write_to_disk(fp);
            to_dir.write_to_disk(fp);
        }
    }

    ~file_system_t() {
        fclose(fp);
    }
};

#endif