// linear_search.c - Trien khai cac function cua LinearDir

#include "linear_search.h"

#define LINEAR_INITIAL_CAPACITY 128

// linear_create - Cap phat bo nho cho mang entries
LinearDir *linear_create(void) {
  LinearDir *dir = (LinearDir *)malloc(sizeof(LinearDir));
  if (!dir)
    return NULL;

  dir->entries = (DirEntry *)malloc(LINEAR_INITIAL_CAPACITY * sizeof(DirEntry));
  if (!dir->entries) {
    free(dir);
    return NULL;
  }

  dir->count = 0;
  dir->capacity = LINEAR_INITIAL_CAPACITY;
  return dir;
}

// linear_insert - Append entry vao cuoi mang (O(1))
int linear_insert(LinearDir *dir, const DirEntry *entry) {
  if (!dir || !entry)
    return -1;

  // Gioi han capacity: Nhan doi bo nho neu het cho
  if (dir->count >= dir->capacity) {
    int new_cap = dir->capacity * 2;
    DirEntry *new_entries =
        (DirEntry *)realloc(dir->entries, new_cap * sizeof(DirEntry));
    if (!new_entries)
      return -1;
    dir->entries = new_entries;
    dir->capacity = new_cap;
  }

  dir->entries[dir->count] = *entry;
  dir->count++;
  return 0;
}


// linear_lookup - Quet mang tu dau den cuoi de tim name
DirEntry *linear_lookup(LinearDir *dir, const char *name,
                        uint64_t *comparisons) {
  if (!dir || !name)
    return NULL;

  uint64_t comp = 0;

  // Quet toan bo mang (worst case O(n))
  for (int i = 0; i < dir->count; i++) {
    comp++;
    if (strcmp(dir->entries[i].name, name) == 0) {
      if (comparisons)
        *comparisons = comp;
      return &dir->entries[i];
    }
  }

  if (comparisons)
    *comparisons = comp;
  return NULL;
}

// linear_delete - Quet mang de tim, swap voi phan tu cuoi roi giam count
int linear_delete(LinearDir *dir, const char *name, uint64_t *comparisons) {
  if (!dir || !name)
    return -1;

  uint64_t comp = 0;

  // Quet toan bo mang de tim entry can xoa (O(n))
  for (int i = 0; i < dir->count; i++) {
    comp++;
    if (strcmp(dir->entries[i].name, name) == 0) {
      // Swap voi phan tu cuoi de xoa O(1) thay vi shift O(n)
      dir->entries[i] = dir->entries[dir->count - 1];
      dir->count--;
      if (comparisons)
        *comparisons = comp;
      return 0; // Xoa thanh cong
    }
  }

  if (comparisons)
    *comparisons = comp;
  return -1; // Khong tim thay
}

// linear_memory_usage - Tra ve tong size cua struct + mang
size_t linear_memory_usage(const LinearDir *dir) {
  if (!dir)
    return 0;
  return sizeof(LinearDir) + (size_t)dir->capacity * sizeof(DirEntry);
}

// linear_destroy - Free bo nho
void linear_destroy(LinearDir *dir) {
  if (!dir)
    return;
  free(dir->entries);
  free(dir);
}