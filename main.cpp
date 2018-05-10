#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

using namespace std;

const int BUFFER_SIZE = 1 << 9;

void create_zeros_file(const char *file_name, int num_bytes) {
    FILE *fp = fopen(file_name, "wb");
    unsigned char zeros_buffer[BUFFER_SIZE] = {0};

    for (int i = 0; i < num_bytes; i += BUFFER_SIZE) {
        fwrite(zeros_buffer, 1, min(num_bytes - i, BUFFER_SIZE), fp);
    }

    fclose(fp);
}

void create_random_file(const char *file_name, int num_bytes) {
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
    }

    fclose(fp);
}

int main() {
    printf("zeros\n");
    create_zeros_file("HD.dat", 1 * 1024); // 1MB file
    read_all_bytes("HD.dat");

    printf("\nrandom\n");

    create_random_file("HD.dat", 2 * 1024); // 2MB file
    read_all_bytes("HD.dat");
    return 0;
}