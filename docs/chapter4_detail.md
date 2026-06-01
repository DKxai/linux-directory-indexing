# Chương 4: Triển Khai Chi Tiết

## 4.1. Module dir_entry — Mô Phỏng Directory Entry của ext4

### 4.1.1. Cấu trúc DirEntry

Cấu trúc `DirEntry` mô phỏng trực tiếp `ext4_dir_entry_2` trong nhân Linux (`fs/ext4/ext4.h`):

```c
typedef struct {
    uint32_t inode;                       // Số inode định danh file
    uint16_t rec_len;                     // Độ dài record (byte)
    uint8_t  name_len;                    // Độ dài tên file thực tế
    uint8_t  file_type;                   // FT_REG_FILE=1, FT_DIR=2, FT_SYMLINK=7
    char     name[MAX_FILENAME_LEN + 1];  // Tên file, tối đa 255 ký tự
} DirEntry;
```

Các hằng số mô phỏng giống ext4: `MAX_FILENAME_LEN = 255`, `BLOCK_SIZE = 4096`.

### 4.1.2. Bộ sinh dữ liệu test

```c
static const char* PREFIXES[] = {
    "main", "utils", "config", "test", "module", "handler",
    "service", "model", "view", "controller", "helper", "data",
    "cache", "queue", "worker", "manager", "factory", "builder",
    "parser", "lexer", "driver", "kernel", "init", "setup",
    "auth", "user", "admin", "api", "db", "log", "core", "base"
};  // 32 prefixes

static const char* EXTENSIONS[] = {
    ".c", ".h", ".o", ".py", ".js", ".html", ".css", ".txt",
    ".md", ".log", ".conf", ".sh", ".json", ".xml", ".yaml",
    ".jpg", ".png", ".gif", ".pdf", ".zip", ".tar", ".gz",
    ".so", ".a", ".out", ".bin", ".dat", ".csv", ".sql", ".rs"
};  // 30 extensions
```

Mỗi tên file có dạng `{prefix}_{index:06d}{ext}` (vd: `kernel_000042.c`). Thuật toán **Fisher-Yates Shuffle** xáo trộn thứ tự:

```c
// Fisher-Yates shuffle — đảm bảo phân bố đều
for (int i = count - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    DirEntry temp = entries[i];
    entries[i] = entries[j];
    entries[j] = temp;
}
```

Seed cố định `srand(42)` → kết quả reproducible giữa các lần chạy.

### 4.1.3. Hàm hỗ trợ benchmark

- `generate_lookup_hits()`: Chọn ngẫu nhiên N tên file **đã tồn tại** để test lookup hit.
- `generate_lookup_misses()`: Tạo tên file không tồn tại (đuôi `.xyz`) để test lookup miss.
- `generate_delete_targets()`: Lấy entries từ **cuối mảng** để tránh trùng với lookup targets.
- `scan_real_directory()`: Đọc thư mục thật từ filesystem bằng `opendir()`/`readdir()` (POSIX), chuyển đổi `d_type` sang `file_type` giống ext4.

---

## 4.2. Module Linear Search — Baseline O(n)

### 4.2.1. Cấu trúc dữ liệu

```c
typedef struct {
    DirEntry* entries;   // Mảng cấp phát động (realloc khi đầy)
    int       count;     // Số entries hiện tại
    int       capacity;  // Dung lượng mảng, khởi tạo = 128
} LinearDir;
```

### 4.2.2. Insert — O(1) amortized

```c
int linear_insert(LinearDir *dir, const DirEntry *entry) {
    if (dir->count >= dir->capacity) {
        int new_cap = dir->capacity * 2;  // Nhân đôi dung lượng
        DirEntry *new_entries =
            (DirEntry *)realloc(dir->entries, new_cap * sizeof(DirEntry));
        if (!new_entries) return -1;
        dir->entries = new_entries;
        dir->capacity = new_cap;
    }
    dir->entries[dir->count] = *entry;  // Append cuối mảng
    dir->count++;
    return 0;
}
```

Chiến lược **doubling** (`capacity × 2`) giúp insert đạt O(1) amortized — chi phí realloc được phân bổ đều.

### 4.2.3. Lookup — O(n) quét tuần tự

```c
DirEntry *linear_lookup(LinearDir *dir, const char *name, uint64_t *comparisons) {
    uint64_t comp = 0;
    for (int i = 0; i < dir->count; i++) {
        comp++;
        if (strcmp(dir->entries[i].name, name) == 0) {
            if (comparisons) *comparisons = comp;
            return &dir->entries[i];  // Tìm thấy
        }
    }
    if (comparisons) *comparisons = comp;
    return NULL;  // Không tìm thấy → đã duyệt toàn bộ N entries
}
```

Worst case: duyệt toàn bộ N entries. Trung bình: N/2 comparisons.

### 4.2.4. Delete — Kỹ thuật Swap-with-Last

```c
int linear_delete(LinearDir *dir, const char *name, uint64_t *comparisons) {
    uint64_t comp = 0;
    for (int i = 0; i < dir->count; i++) {
        comp++;
        if (strcmp(dir->entries[i].name, name) == 0) {
            // Swap phần tử cần xóa với phần tử cuối → O(1)
            dir->entries[i] = dir->entries[dir->count - 1];
            dir->count--;
            if (comparisons) *comparisons = comp;
            return 0;
        }
    }
    if (comparisons) *comparisons = comp;
    return -1;
}
```

Thay vì dịch mảng O(n), chỉ cần **hoán đổi với phần tử cuối** rồi giảm `count` → chi phí xóa (sau khi tìm thấy) chỉ O(1). Tổng: O(n) scan + O(1) xóa.

---

## 4.3. Module Hash Table — O(1) Trung Bình

### 4.3.1. Cấu trúc Separate Chaining

```c
typedef struct HashNode {
    DirEntry          entry;
    struct HashNode*  next;   // Linked list cho collision
} HashNode;

typedef struct {
    HashNode**  buckets;      // Mảng head pointers
    int         num_buckets;  // Khởi tạo = 256
    int         count;
} HashTable;
```

### 4.3.2. Hàm băm DJB2

```c
uint32_t djb2_hash(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;  // hash = hash * 33 + c
    }
    return hash;
}
```

DJB2 sử dụng magic number 5381 và hệ số nhân 33. Ưu điểm: cài đặt đơn giản, phân bố đều cho chuỗi ngắn, chi phí tính toán thấp (chỉ dùng phép dịch bit và cộng).

### 4.3.3. Auto-Rehash

```c
static int hash_rehash(HashTable* ht) {
    int new_num_buckets = ht->num_buckets * 2;
    HashNode** new_buckets = (HashNode**)calloc(new_num_buckets, sizeof(HashNode*));

    // Di chuyển từng node sang bảng mới
    for (int i = 0; i < ht->num_buckets; i++) {
        HashNode* current = ht->buckets[i];
        while (current) {
            HashNode* next = current->next;
            uint32_t idx = djb2_hash(current->entry.name) % new_num_buckets;
            current->next = new_buckets[idx];  // Prepend vào chain mới
            new_buckets[idx] = current;
            current = next;
        }
    }
    free(ht->buckets);
    ht->buckets = new_buckets;
    ht->num_buckets = new_num_buckets;
    return 0;
}
```

Rehash được kích hoạt khi `load_factor > 0.7` (`HASH_LOAD_FACTOR_THRESHOLD`). Các node **không cần tạo mới** — chỉ di chuyển con trỏ sang mảng buckets mới, tối ưu bộ nhớ.

### 4.3.4. Insert với kiểm tra load factor

```c
int hash_insert(HashTable* ht, const DirEntry* entry) {
    double lf = (double)(ht->count + 1) / (double)ht->num_buckets;
    if (lf > HASH_LOAD_FACTOR_THRESHOLD) {
        if (hash_rehash(ht) != 0) return -1;
    }
    uint32_t idx = djb2_hash(entry->name) % ht->num_buckets;
    HashNode* node = (HashNode*)malloc(sizeof(HashNode));
    node->entry = *entry;
    node->next = ht->buckets[idx];  // Prepend O(1)
    ht->buckets[idx] = node;
    ht->count++;
    return 0;
}
```

---

## 4.4. Module B-Tree — O(log n)

### 4.4.1. Cấu trúc node

```c
#define BTREE_MIN_DEGREE 50  // t=50 → mỗi node: 49..99 keys, 50..100 children

typedef struct BTreeNode {
    DirEntry         *keys;       // Mảng tối đa 2t-1 = 99 keys
    struct BTreeNode **children;  // Mảng tối đa 2t = 100 con trỏ
    int              num_keys;
    bool             is_leaf;
} BTreeNode;
```

Bậc `t = 50` được chọn để mô phỏng: mỗi node chứa tối đa 99 DirEntry, tương đương kích thước ~1 disk block.

### 4.4.2. Proactive Split

Khi insert, nếu gặp node con đầy (2t−1 keys) trên đường đi xuống → **tách trước** khi đi xuống:

```c
static void split_child(BTreeNode *parent, int idx, int t) {
    BTreeNode *full_child = parent->children[idx];
    BTreeNode *new_child = create_node(t, full_child->is_leaf);
    new_child->num_keys = t - 1;

    // Copy nửa phải keys sang node mới
    for (int j = 0; j < t - 1; j++)
        new_child->keys[j] = full_child->keys[j + t];

    // Copy nửa phải children (nếu internal node)
    if (!full_child->is_leaf)
        for (int j = 0; j < t; j++)
            new_child->children[j] = full_child->children[j + t];

    full_child->num_keys = t - 1;

    // Đẩy key giữa (median) lên parent
    // Dịch phải keys và children của parent để chèn
    for (int j = parent->num_keys - 1; j >= idx; j--)
        parent->keys[j + 1] = parent->keys[j];
    parent->keys[idx] = full_child->keys[t - 1];
    parent->num_keys++;

    for (int j = parent->num_keys; j >= idx + 1; j--)
        parent->children[j + 1] = parent->children[j];
    parent->children[idx + 1] = new_child;
}
```

Ưu điểm: chỉ cần **1 lượt đi xuống** (single-pass), không cần quay ngược lên — phù hợp cho ứng dụng disk-based.

### 4.4.3. Delete — Xử lý 3 trường hợp

**Case 1 — Key ở node lá:** Xóa trực tiếp, dịch trái các key phía sau.

**Case 2 — Key ở node trung gian:**
```c
if (node->children[idx]->num_keys >= t) {
    // Thay bằng predecessor (key lớn nhất cây con trái)
    DirEntry pred = get_predecessor(node->children[idx]);
    node->keys[idx] = pred;
    return delete_from_node(node->children[idx], pred.name, t, comp);
}
```
Nếu cây con trái có ≥ t keys → thay key bằng **predecessor**. Ngược lại, thử **successor** từ cây con phải. Nếu cả hai đều chỉ có t−1 keys → **merge** hai node con.

**Case 3 — Đảm bảo node con có đủ keys (fill_child):**
```c
static void fill_child(BTreeNode *parent, int idx, int t) {
    // Ưu tiên 1: Borrow từ sibling trái (nếu có ≥ t keys)
    if (idx > 0 && parent->children[idx - 1]->num_keys >= t) {
        // Kéo key từ parent xuống, đẩy key cuối sibling lên parent
        ...
        return;
    }
    // Ưu tiên 2: Borrow từ sibling phải
    if (idx < parent->num_keys && parent->children[idx + 1]->num_keys >= t) {
        ...
        return;
    }
    // Ưu tiên 3: Merge hai node con + key cha
    merge_children(parent, idx < parent->num_keys ? idx : idx - 1, t);
}
```

### 4.4.4. Merge Children

```c
static void merge_children(BTreeNode *parent, int idx, int t) {
    BTreeNode *left = parent->children[idx];
    BTreeNode *right = parent->children[idx + 1];

    left->keys[t - 1] = parent->keys[idx];  // Kéo key cha xuống

    for (int j = 0; j < right->num_keys; j++)
        left->keys[j + t] = right->keys[j];  // Copy keys từ right

    if (!left->is_leaf)
        for (int j = 0; j <= right->num_keys; j++)
            left->children[j + t] = right->children[j];

    left->num_keys += right->num_keys + 1;
    parent->num_keys--;

    free(right->keys); free(right->children); free(right);
}
```

---

## 4.5. Module HTree — Mô Phỏng ext4 Hash Tree

### 4.5.1. Hàm băm Half-MD4

Mô phỏng trực tiếp từ `fs/ext4/hash.c` trong Linux kernel:

```c
static void half_md4_transform(uint32_t buf[4], const uint32_t in[8]) {
    uint32_t a = buf[0], b = buf[1], c = buf[2], d = buf[3];

    // Round 1 — F(x,y,z) = (x & y) | (~x & z)
    #define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
    #define ROUND1(a, b, c, d, k, s) \
        (a) += F((b), (c), (d)) + (in[(k)]); \
        (a) = ((a) << (s)) | ((a) >> (32 - (s)));
    ROUND1(a,b,c,d, 0,3); ROUND1(d,a,b,c, 1,7);
    ROUND1(c,d,a,b, 2,11); ROUND1(b,c,d,a, 3,19);
    // ... tiếp tục 4 rounds nữa cho Round 1

    // Round 2 — G(x,y,z) = (x&y) | (x&z) | (y&z), magic = 0x5A827999
    // Round 3 — H(x,y,z) = x ^ y ^ z, magic = 0x6ED9EBA1

    buf[0] += a; buf[1] += b; buf[2] += c; buf[3] += d;
}
```

Hàm `htree_half_md4()` pack tên file thành mảng `uint32_t in[8]` (little-endian), gọi `half_md4_transform()`, rồi lấy `buf[1]` làm hash 32-bit. Giá trị hash = 0 bị loại bỏ (reserved trong ext4).

### 4.5.2. Binary Search trong dx_entries

```c
static int find_dx_entry_index(const HTree* ht, uint32_t target_hash,
                                uint64_t* comparisons) {
    int lo = 0, hi = ht->num_dx_entries - 1;
    int result = 0;

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (comparisons) (*comparisons)++;

        if (ht->dx_entries[mid].hash <= target_hash) {
            result = mid;   // Ghi nhớ dx_entry có hash lớn nhất ≤ target
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return result;
}
```

Tìm dx_entry có hash **lớn nhất ≤ target_hash** — đây chính là leaf block chứa entry cần tìm.

### 4.5.3. Lookup — 2 bước (Binary Search + Linear Scan)

```c
DirEntry* htree_lookup(HTree* ht, const char* name, uint64_t* comparisons) {
    uint64_t comp = 0;
    uint32_t hash = compute_hash(ht->hash_version, name);

    // Bước 1: Binary search tìm leaf block
    int dx_idx = find_dx_entry_index(ht, hash, &comp);

    // Bước 2: Linear scan trong leaf block (giống ext4)
    HTreeLeafBlock* block = ht->dx_entries[dx_idx].block;
    for (int i = 0; i < block->count; i++) {
        comp++;
        if (strcmp(block->entries[i].name, name) == 0) {
            if (comparisons) *comparisons = comp;
            return &block->entries[i];
        }
    }

    // Xử lý hash collision: kiểm tra block kế tiếp
    if (dx_idx + 1 < ht->num_dx_entries) {
        HTreeLeafBlock* next_block = ht->dx_entries[dx_idx + 1].block;
        for (int i = 0; i < next_block->count; i++) {
            comp++;
            if (strcmp(next_block->entries[i].name, name) == 0)
                return &next_block->entries[i];
        }
    }
    return NULL;
}
```

Cơ chế kiểm tra **block kế tiếp** xử lý trường hợp hash collision: hai tên file khác nhau có thể cùng hash value, nằm ở ranh giới giữa hai leaf blocks.

### 4.5.4. Split Leaf Block

Khi leaf block đầy (64 entries):

```c
static int split_leaf_block(HTree* ht, int dx_idx) {
    HTreeLeafBlock* old_block = ht->dx_entries[dx_idx].block;

    // 1. Tính hash cho tất cả entries
    uint32_t hashes[HTREE_BLOCK_CAPACITY];
    for (int i = 0; i < old_block->count; i++)
        hashes[i] = compute_hash(ht->hash_version, old_block->entries[i].name);

    // 2. Sắp xếp theo hash (insertion sort — block nhỏ nên OK)
    for (int i = 1; i < old_block->count; i++) {
        uint32_t key_hash = hashes[i];
        DirEntry key_entry = old_block->entries[i];
        int j = i - 1;
        while (j >= 0 && hashes[j] > key_hash) {
            hashes[j+1] = hashes[j];
            old_block->entries[j+1] = old_block->entries[j];
            j--;
        }
        hashes[j+1] = key_hash;
        old_block->entries[j+1] = key_entry;
    }

    // 3. Tách đôi: nửa đầu ở lại, nửa sau sang block mới
    int mid = old_block->count / 2;
    HTreeLeafBlock* new_block = create_leaf_block();
    for (int i = 0; i < old_block->count - mid; i++)
        new_block->entries[i] = old_block->entries[mid + i];
    new_block->count = old_block->count - mid;
    old_block->count = mid;

    // 4. Thêm dx_entry mới với hash = hash của entry đầu block mới
    ht->dx_entries[dx_idx + 1].hash = hashes[mid];
    ht->dx_entries[dx_idx + 1].block = new_block;
    ht->num_dx_entries++;
    return 0;
}
```

Giống ext4 `dx_make_map` + `dx_split_leaf`: sắp xếp theo hash → chia đôi → cập nhật index.

---

## 4.6. Module Benchmark Framework

### 4.6.1. Timer macros — Đo chính xác nanosecond

```c
#define TIMER_DECLARE()  struct timespec _ts_start, _ts_end
#define TIMER_START()    clock_gettime(CLOCK_MONOTONIC, &_ts_start)
#define TIMER_END()      clock_gettime(CLOCK_MONOTONIC, &_ts_end)
#define TIMER_ELAPSED_NS() \
    ((uint64_t)(_ts_end.tv_sec - _ts_start.tv_sec) * 1000000000ULL + \
     (uint64_t)(_ts_end.tv_nsec - _ts_start.tv_nsec))
```

`CLOCK_MONOTONIC` đảm bảo đồng hồ chỉ tăng, không bị ảnh hưởng bởi NTP hoặc thay đổi giờ hệ thống.

### 4.6.2. BenchmarkResult — Cấu trúc lưu kết quả

```c
typedef struct {
    char     method_name[32];
    int      num_entries;
    uint64_t insert_time_ns;
    uint64_t avg_lookup_hit_ns;
    uint64_t avg_lookup_miss_ns;
    uint64_t avg_comparisons_hit;
    uint64_t avg_comparisons_miss;
    uint64_t avg_delete_time_ns;
    uint64_t avg_delete_comparisons;
    size_t   memory_usage_bytes;
} BenchmarkResult;
```

### 4.6.3. Quy trình benchmark cho mỗi phương pháp

Mỗi hàm `benchmark_xxx()` tuân theo quy trình 5 bước chuẩn:

1. **Insert**: Tạo cấu trúc rỗng → insert N entries → đo tổng thời gian.
2. **Lookup Hit**: 1.000 lookup entries tồn tại → đo TB thời gian + comparisons.
3. **Lookup Miss**: 100 lookup entries không tồn tại → đo TB thời gian + comparisons.
4. **Ghi nhận Memory** trước khi delete.
5. **Delete**: Destroy + rebuild cấu trúc → 500 deletes → đo TB thời gian + comparisons.

**Lưu ý quan trọng:** Cấu trúc được **rebuild hoàn toàn** trước delete test để đảm bảo có đủ entries, không bị ảnh hưởng bởi trạng thái sau lookup.

### 4.6.4. Xuất CSV

```c
int export_csv(const BenchmarkResult *results, int count, const char *filename) {
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "method,num_entries,insert_time_ns,avg_lookup_hit_ns,"
                "avg_lookup_miss_ns,avg_comparisons_hit,...\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s,%d,%lu,%lu,...\n", ...);
    }
    fclose(fp);
    return 0;
}
```

File CSV được script `visualize.py` (Matplotlib) đọc để sinh 6 biểu đồ dark-theme PNG (300 DPI).

---

## 4.7. Quản Lý Bộ Nhớ An Toàn

Mỗi module cài đặt hàm `*_destroy()` giải phóng đệ quy:

- **linear_destroy**: `free(entries)` + `free(dir)`.
- **hash_destroy**: Duyệt mỗi bucket → free từng node trong chain → free mảng buckets → free struct.
- **btree_destroy**: Gọi `destroy_node()` đệ quy — free keys, children, node ở mỗi level.
- **htree_destroy**: Free từng leaf block → free mảng dx_entries → free struct.

Hàm `*_memory_usage()` tính chính xác bộ nhớ đã cấp phát (không dùng `getrusage` mà tính trực tiếp từ cấu trúc):

```c
// Ví dụ: Hash Table memory
size_t hash_memory_usage(const HashTable* ht) {
    size_t usage = sizeof(HashTable);
    usage += (size_t)ht->num_buckets * sizeof(HashNode*);  // Mảng buckets
    usage += (size_t)ht->count * sizeof(HashNode);          // N nodes
    return usage;
}
```

Kết quả Valgrind: **0 bytes leaked, 0 errors, 3.611.188 allocs = 3.611.188 frees**.
