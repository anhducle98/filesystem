#include <stdio.h>
#include <vector>
#include <cassert>
#include <bits/stdc++.h>

#include "file_system.h"

void callsystem() {
    file_system_t FS;
    FS.create_empty_disk("HD.dat");

    FS.create_empty_folder("/", "code");
    FS.copy_file_from_outside("/code/", "os.txt");
    FS.copy_file_from_outside("/code/", "shit.cpp");
    FS.copy_file_from_outside("/", "big.txt");
    FS.create_empty_folder("/code/", "cpp");
    FS.copy_file_from_outside("/code/cpp/", "shit.cpp");

    FS.read_disk_info();
    FS.list_disk();

    /*FS.delete_folder("/", "code");
    FS.read_disk_info();
    FS.list_disk();*/
}

void interface(){
    string type, filename, paste_file_name;

    file_system_t FS;

    directory_t cur_dir;
    cur_dir.read_from_disk(FS.fp, ROOT_INUM);

    string cur_path = "/";

    while(1){
        printf("%s : ", cur_path.c_str());
        cin >> type;
        
        if (type == "ls") {
            printf("--------> : | ");
            for(auto it : cur_dir.a){
                if(it.first == cur_dir.inode.inum || it.second == "..") continue;
                printf("%s | ", it.second.c_str());
            }
            printf("\n");
        }

        if (type == "cd") {
            
            bool pick = 0;
            while(pick == 0){
                cin >> filename;
                for(auto it : cur_dir.a){
                    if(it.first == cur_dir.inode.inum) continue;
                    if(it.second == filename && it.second != ".."){
                        cur_dir.read_from_disk(FS.fp, it.first);
                        cur_path += filename + "/";
                        pick = 1;
                        break;
                    }
                    if(it.second == filename && it.second == ".."){
                        cur_path.pop_back();
                        while(cur_path.back() != '/')
                            cur_path.pop_back();
                        cur_dir.read_from_disk(FS.fp, it.first);
                        pick = 1;
                        break;
                    }
                }
            }
        }

        if (type == "add") {
            cin >> filename;
            FS.copy_file_from_outside(cur_path, filename);
            cur_dir.read_from_disk(FS.fp, cur_dir.inode.inum);
        }

        if (type == "mkdir") { 
            string foldername;
            cin >> foldername;
            FS.create_empty_folder(cur_path, foldername);
            cur_dir.read_from_disk(FS.fp, cur_dir.inode.inum);
        }

        if (type == "delete") {
            cin >> filename;
            FS.delete_file(cur_path, filename);
            cur_dir.read_from_disk(FS.fp, cur_dir.inode.inum);
        }

        if (type == "copy") {
            cin >> filename >> paste_file_name;
            FS.copy_file_to_outside(cur_path, filename, paste_file_name);
        }
    }
}

int main() {
    //callsystem();
    interface();
    return 0;
}