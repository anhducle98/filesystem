#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <iostream>

#include "file_system.h"
#include "constants.h"

struct interface_t {
    file_system_t FS;
    string cur_path = "/";
    directory_t cur_dir;

    interface_t() {
        cur_dir.read_from_disk(FS.fp, ROOT_INUM);
    }

    void execute() {
        while (1) {
            printf("%stest@test%s:%s%s%s$%s ", CL_GREEN, CL_WHITE, CL_BLUE, cur_path.c_str(), CL_WHITE, CL_RESET);
            string type;
            cin >> type;
            
            if (type == "cd") {
                do_cd();
            } else if (type == "ls"){
                do_ls();
            } else if (type == "add") {
                do_add();
            } else if (type == "mkdir") {
                do_mkdir();
            } else if (type == "rm" || type == "delete") {
                do_delete();
            } else if (type == "copy" || type == "cp" ) {
                do_copy();
            } else if (type == "cat") {
                do_cat();
            } else if (type == "tree") {
                do_tree();
            } else if (type == "poweroff") {
                printf("Goodbye!\n");
                break;
            } else {
                printf("%s: command not found\n", type.c_str());
            }
        }
    }

    void do_cd() {
        // TODO: fix cd to file
        // TODO: cd to path
        bool pick = 0;
        string filename;
        cin >> filename;
        for(auto it : cur_dir.a) {
            if (it.second != filename) {
                continue;
            }
            if (it.second == ".") {
                pick = 1;
            } else if (it.second != "..") {
                cur_dir.read_from_disk(FS.fp, it.first);
                cur_path += filename + "/";
                pick = 1;
            } else {
                cur_path.pop_back();
                while(cur_path.back() != '/') {
                    cur_path.pop_back();
                }
                cur_dir.read_from_disk(FS.fp, it.first);
                pick = 1;
            }
            break;
        }
        if (!pick) {
            printf("bash: cd: %s: No such file or directory\n", filename.c_str());
        }
    }

    void do_ls() {
        for (auto it : cur_dir.a) {
            if (it.first == cur_dir.inode.inum || it.second == "..") {
                continue;
            }
            if (FS.is_directory(it.first)) {
                printf("%s%s%s\n", CL_BLUE, it.second.c_str(), CL_RESET);
            } else {
                printf("%s\n", it.second.c_str());
            }
        }
    }

    void do_add() {
        string filename;
        cin >> filename;
        FS.copy_file_from_outside(cur_path, filename);
        cur_dir.read_from_disk(FS.fp, cur_dir.inode.inum);
    }

    void do_mkdir() {
        string foldername;
        cin >> foldername;
        FS.create_empty_folder(cur_path, foldername);
        cur_dir.read_from_disk(FS.fp, cur_dir.inode.inum);
    }

    void do_delete() {
        string filename;
        cin >> filename;
        FS.delete_file(cur_path, filename);
        cur_dir.read_from_disk(FS.fp, cur_dir.inode.inum);
    }

    void do_copy() {
        string filename;
        string paste_file_name;
        cin >> filename >> paste_file_name;
        FS.copy_file_to_outside(cur_path, filename, paste_file_name);
    }

    void do_cat() {
        string file_path;
        cin >> file_path;
        if (file_path[0] != '/') {
            file_path = cur_path + file_path;
        }
        FS.view_text_file(file_path);
    }

    void do_tree() {
        FS.list_disk();
    }
};

#endif