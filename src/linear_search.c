// linear_search.c - Triển khai các function của LinearDir

#include "linear_search.h"

#define LINEAR_INITIAL_CAPACITY 128

// linear_create - Cấp phát bộ nhớ cho mảng entries
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

// linear_insert - Append entry vào cuối mảng (O(1))
int linear_insert(LinearDir *dir, const DirEntry *entry) {
  if (!dir || !entry)
    return -1;

  // Giới hạn capacity: Nhân đôi bộ nhớ nếu hết chỗ
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

// linear_lookup - Quét mảng từ đầu đến cuối để tìm name
DirEntry *linear_lookup(LinearDir *dir, const char *name,
                        uint64_t *comparisons) {
  if (!dir || !name)
    return NULL;

  uint64_t comp = 0;

  // Quét toàn bộ mảng (worst case O(n))
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

// linear_memory_usage - Trả về tổng size của struct + mảng
size_t linear_memory_usage(const LinearDir *dir) {
  if (!dir)
    return 0;
  return sizeof(LinearDir) + (size_t)dir->capacity * sizeof(DirEntry);
}

// linear_destroy - Free bộ nhớ
void linear_destroy(LinearDir *dir) {
  if (!dir)
    return;
  free(dir->entries);
  free(dir);
}
