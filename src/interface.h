#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <iostream>

#include "file_system.h"
#include "constants.h"

#ifndef __unix__
#define CL_WHITE ""
#define CL_BLUE ""
#define CL_GREEN ""
#define CL_RESET ""
#endif

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
            } else if (type == "mv") {
                do_move();
            } else if (type == "info") {
                do_info();
            }else if (type == "tree") {
                do_tree();
            } else if (type == "poweroff") {
                printf("Goodbye!\n");
                break;
            } else {
                printf("%s: command not found\n", type.c_str());
            }
        }
    }

    /**
     * cd to single directory
     * @param dir directory we want to go
     * @return -1 if it's a file, 0 if dir is not exist, 1 if success
     */
    int do_cd_one_dir(const string &dir) {
        bool pick = 0;
        for(auto it : cur_dir.a) {
            if (it.second != dir) {
                continue;
            }
            if (it.second == ".") {
                pick = 1;
            } else if (it.second != "..") {
                if (cur_dir.read_from_disk(FS.fp, it.first)) {
                    cur_path += dir + "/";
                    pick = 1;
                } else {
                    return -1;
                }
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
        return pick;
    }

    void do_cd() {
        string backup_path = cur_path;
        directory_t backup_dir = cur_dir;
        string filename;
        cin >> filename;
        if (filename[0] == '/') {
            cur_dir.read_from_disk(FS.fp, ROOT_INUM);
            cur_path = "/";
        }
        vector<string> dir = FS.split_path(filename);
        for (string u : dir) {
            int verdict = do_cd_one_dir(u);
            if (verdict == -1) {
                printf("bash: cd: %s: Not a directory\n", filename.c_str());
                cur_path = backup_path;
                cur_dir = backup_dir;
                return;
            }
            if (verdict == 0) {
                printf("bash: cd: %s: No such file or directory\n", filename.c_str());
                cur_path = backup_path;
                cur_dir = backup_dir;
                return;
            }
        }
    }

    void do_ls() {
        cur_dir.read_from_disk(FS.fp, cur_dir.inode.inum);
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

    void do_move() {
        string from_path, to_path;
        cin >> from_path >> to_path;
        if (from_path[0] != '/') {
            from_path = cur_path + from_path;
        }
        if (to_path[0] != '/') {
            to_path = cur_path + to_path;
        }
        FS.move_entry(from_path, to_path);
    }

    void do_tree() {
        FS.list_disk();
    }

    void do_info() {
        FS.read_disk_info();
    }
};

#endif