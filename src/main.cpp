#include <stdio.h>
#include <vector>
#include <cassert>

#include "file_system.h"

void run_test() {
    file_system_t FS;
    FS.create_empty_disk("HD.dat");

    FS.create_empty_folder("/", "code");
    FS.copy_file_from_outside("/code/", "os.txt");
    FS.copy_file_from_outside("/code/", "shit.cpp");
    FS.copy_file_from_outside("/", "big.txt");

    FS.read_disk_info();
    FS.list_disk();

    FS.delete_file_disk("/code/", "os.txt");
    FS.read_disk_info();
    FS.list_disk();

    FS.delete_file_disk("/", "big.txt");
    FS.list_disk();

    FS.view_text_file("/code/shit.cpp");
}

int main() {
    run_test();
    return 0;
}