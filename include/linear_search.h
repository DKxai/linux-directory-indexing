// linear_search.h - Mo phong directory indexing cau truc Linear (O(n))

#ifndef LINEAR_SEARCH_H
#define LINEAR_SEARCH_H

#include "dir_entry.h"

// LinearDir - Chua mang directory entries
typedef struct {
    DirEntry* entries;
    int       count;
    int       capacity;
} LinearDir;

// linear_create - Khoi tao LinearDir
LinearDir* linear_create(void);

// linear_insert - Them entry moi
int linear_insert(LinearDir* dir, const DirEntry* entry);

// linear_lookup - Tim kiem entry (tuan tu)
DirEntry* linear_lookup(LinearDir* dir, const char* name, uint64_t* comparisons);

// linear_memory_usage - Tinh bo nho da dung
size_t linear_memory_usage(const LinearDir* dir);

// linear_destroy - Giai phong bo nho
void linear_destroy(LinearDir* dir);

#endif /* LINEAR_SEARCH_H */
