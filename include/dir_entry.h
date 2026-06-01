// dir_entry.h - Cau truc DirEntry va utils

#ifndef DIR_ENTRY_H
#define DIR_ENTRY_H

#include "common.h"

// Types gia lap giong ext4
#define FT_UNKNOWN   0
#define FT_REG_FILE  1
#define FT_DIR       2
#define FT_CHRDEV    3
#define FT_BLKDEV    4
#define FT_FIFO      5
#define FT_SOCK      6
#define FT_SYMLINK   7

// DirEntry - Thong tin cua mot thu muc / tep
typedef struct {
    uint32_t inode;                     
    uint16_t rec_len;                   
    uint8_t  name_len;                  
    uint8_t  file_type;                 
    char     name[MAX_FILENAME_LEN + 1]; 
} DirEntry;

// generate_random_entries - Tao mang entries ngau nhien test
DirEntry* generate_random_entries(int count);

// generate_lookup_hits - Lay mang ten dang ton tai
char** generate_lookup_hits(DirEntry* entries, int num_entries, int num_targets);

// generate_lookup_misses - Tao mang ten chac chan khong ton tai
char** generate_lookup_misses(int num_targets);

// free_lookup_targets - Free mang ten
void free_lookup_targets(char** targets, int count);

// generate_delete_targets - Tao mang ten de test delete (lay tu entries da ton tai)
char** generate_delete_targets(DirEntry* entries, int num_entries, int num_targets);

// print_dir_entry - In thong tin DirEntry
void print_dir_entry(const DirEntry* entry);

// scan_real_directory - Doc thu muc that tren filesystem
// Tra ve mang DirEntry va ghi so luong vao *out_count
DirEntry* scan_real_directory(const char* path, int* out_count);

#endif /* DIR_ENTRY_H */
