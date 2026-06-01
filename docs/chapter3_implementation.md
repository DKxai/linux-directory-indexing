# Chương 3: Thiết Kế và Cài Đặt Hệ Thống

## 3.1. Tổng Quan Kiến Trúc

Hệ thống được xây dựng hoàn toàn bằng ngôn ngữ C, biên dịch trên nền tảng Linux với GCC. Tổng quy mô mã nguồn khoảng **2.840 dòng code**, được tổ chức theo kiến trúc module hóa rõ ràng:

```
linux-directory-indexing/
├── include/                    # Header files (API contracts)
│   ├── common.h                # Types, constants, timer macros (81 LOC)
│   ├── dir_entry.h             # DirEntry struct + generator API
│   ├── linear_search.h         # Linear Search API
│   ├── hash_table.h            # Hash Table API
│   ├── btree.h                 # B-Tree API
│   ├── htree.h                 # HTree API
│   └── benchmark.h             # Benchmark framework API
├── src/                        # Implementation files
│   ├── main.c                  # Entry point + Terminal UI (470 LOC)
│   ├── dir_entry.c             # Random entry generator (122 LOC)
│   ├── linear_search.c         # Linear Search (107 LOC)
│   ├── hash_table.c            # Hash Table + rehash (249 LOC)
│   ├── btree.c                 # B-Tree full operations (539 LOC)
│   ├── htree.c                 # HTree ext4 simulation (495 LOC)
│   └── benchmark.c             # Benchmark engine (487 LOC)
├── scripts/visualize.py        # Matplotlib chart generator (449 LOC)
├── results/                    # Output CSV + PNG charts
└── Makefile                    # Build system
```

**Luồng dữ liệu chính:**

```
User Input → main.c (menu/CLI)
  → benchmark.c (chạy tests)
    → dir_entry.c (sinh N random entries)
    → 4 indexing modules (insert / lookup / delete)
    → BenchmarkResult[] (timing + comparisons + memory)
  → export_csv() → results/benchmark_results.csv
  → visualize.py → results/*.png (6 biểu đồ)
```

---

## 3.2. Cấu Trúc Dữ Liệu Nền Tảng

### 3.2.1. DirEntry — Mô phỏng ext4 Directory Entry

Cấu trúc `DirEntry` được thiết kế mô phỏng sát với `ext4_dir_entry_2` trong nhân Linux:

```c
typedef struct {
    uint32_t inode;                       // Số inode (định danh file)
    uint16_t rec_len;                     // Độ dài record
    uint8_t  name_len;                    // Độ dài tên file
    uint8_t  file_type;                   // Loại file (FT_REG_FILE, FT_DIR, ...)
    char     name[MAX_FILENAME_LEN + 1];  // Tên file (tối đa 255 ký tự)
} DirEntry;
```

Các hằng số quan trọng:
- `MAX_FILENAME_LEN = 255` — giới hạn tên file giống ext4.
- `BLOCK_SIZE = 4096` — mô phỏng kích thước disk block 4KB.

### 3.2.2. Bộ Sinh Dữ Liệu Test (Data Generator)

Hàm `generate_random_entries()` tạo dữ liệu test đa dạng và có tính lặp lại (reproducible):

- **32 prefixes** (`main`, `utils`, `config`, `kernel`, ...) × **30 extensions** (`.c`, `.h`, `.py`, `.json`, ...) → không gian tên file phong phú.
- Mỗi tên file có dạng: `{prefix}_{index:06d}{extension}` (ví dụ: `kernel_000042.c`).
- Sử dụng thuật toán **Fisher-Yates Shuffle** để xáo trộn ngẫu nhiên thứ tự entries.
- **Seed cố định** (`srand(42)`) đảm bảo mỗi lần chạy cho kết quả giống nhau, loại bỏ nhiễu ngẫu nhiên khi so sánh benchmark.

---

## 3.3. Chi Tiết Cài Đặt Các Module Chỉ Mục

### 3.3.1. Linear Search — Baseline O(n)

**Cấu trúc dữ liệu:**

```c
typedef struct {
    DirEntry* entries;   // Mảng cấp phát động
    int       count;     // Số entries hiện tại
    int       capacity;  // Dung lượng mảng (tự nhân đôi)
} LinearDir;
```

```
┌──────────────────────────────────────────────┐
│ LinearDir                                     │
│ ┌─────┬─────┬─────┬─────┬─────┬──────────┐  │
│ │ E₀  │ E₁  │ E₂  │ ... │ Eₙ₋₁│ (trống)  │  │
│ └─────┴─────┴─────┴─────┴─────┴──────────┘  │
│ count = n              capacity (tự ×2)       │
└──────────────────────────────────────────────┘
```

**Các thao tác chính:**

| Thao tác | Độ phức tạp | Cơ chế cài đặt |
|----------|------------|-----------------|
| **Insert** | O(1) amortized | Append cuối mảng + auto-resize ×2 khi hết chỗ (`realloc`) |
| **Lookup** | O(n) | Quét tuần tự từ đầu, so sánh `strcmp()` từng entry |
| **Delete** | O(n) scan + O(1) xóa | Tìm entry → swap với phần tử cuối → giảm `count` |
| **Memory** | — | `sizeof(LinearDir) + capacity × sizeof(DirEntry)` |

**Điểm đáng chú ý:** Kỹ thuật **swap-with-last** khi xóa — thay vì dịch toàn bộ mảng O(n), chỉ cần hoán đổi phần tử cần xóa với phần tử cuối cùng rồi giảm `count`, biến chi phí xóa thực tế (sau khi tìm thấy) thành O(1). Dung lượng khởi tạo `LINEAR_INITIAL_CAPACITY = 128`, tự nhân đôi khi đầy.

---

### 3.3.2. Hash Table — O(1) Trung Bình

**Cấu trúc dữ liệu:**

```c
typedef struct HashNode {
    DirEntry          entry;
    struct HashNode*  next;      // Con trỏ tới node kế tiếp (Chaining)
} HashNode;

typedef struct {
    HashNode**  buckets;         // Mảng con trỏ tới head của mỗi chain
    int         num_buckets;     // Kích thước mảng buckets
    int         count;           // Tổng số entries
} HashTable;
```

```
┌─────────────────────────────────────────────────┐
│ HashTable (num_buckets, count)                   │
│ buckets[0] → [Node] → [Node] → NULL             │
│ buckets[1] → NULL                                │
│ buckets[2] → [Node] → NULL                       │
│ buckets[3] → [Node] → [Node] → [Node] → NULL    │
│   ...                                            │
│ buckets[m-1] → NULL                              │
└─────────────────────────────────────────────────┘
```

**Hàm băm DJB2:**

```c
uint32_t djb2_hash(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash;
}
```

Hàm DJB2 của Daniel J. Bernstein được chọn vì: đơn giản, phân bố đều, hiệu quả cho chuỗi ngắn (tên file).

**Cơ chế xử lý đụng độ — Separate Chaining:** Mỗi bucket là một Singly Linked List. Insert prepend vào đầu chain (O(1)). Lookup duyệt chain tại `bucket[hash % num_buckets]`. Delete duyệt chain, tìm và xóa node.

**Auto-Rehash:** Khi **load factor** > 0.7, tự động nhân đôi số buckets và phân bố lại toàn bộ entries. Khởi tạo với `HASH_INITIAL_CAPACITY = 256` buckets.

| Thao tác | Độ phức tạp | Chi tiết |
|----------|------------|----------|
| **Insert** | O(1) avg | Hash → prepend đầu chain, rehash nếu quá tải |
| **Lookup** | O(1) avg | Hash → duyệt chain tại bucket tương ứng |
| **Delete** | O(1) avg | Hash → tìm trong chain → xóa node |
| **Rehash** | O(n) | Nhân đôi buckets, re-distribute toàn bộ |

---

### 3.3.3. B-Tree — O(log n)

**Cấu trúc dữ liệu:**

```c
typedef struct BTreeNode {
    DirEntry         *keys;       // Mảng keys (tên file)
    struct BTreeNode **children;  // Mảng con trỏ tới các node con
    int              num_keys;    // Số keys hiện tại
    bool             is_leaf;
} BTreeNode;

typedef struct {
    BTreeNode *root;
    int       min_degree;  // Bậc tối thiểu t = 50
    int       count;
} BTree;
```

```
                    ┌─────────────────────┐
                    │  [K₁ | K₂ | ... ]   │  ← Root (tối đa 99 keys)
                    └──┬────┬────┬────┬───┘
                       │    │    │    │
              ┌────────┘    │    │    └────────┐
              ▼             ▼    ▼             ▼
        ┌──────────┐  ┌──────────┐      ┌──────────┐
        │ [keys...]│  │ [keys...]│      │ [keys...]│
        └──┬──┬──┬─┘  └──┬──┬──┬─┘      └──┬──┬──┬─┘
           ▼  ▼  ▼       ▼  ▼  ▼            ▼  ▼  ▼
         [leaf] [leaf] [leaf] [leaf]      [leaf] [leaf]

  t = 50 → Keys/node: 49..99, Children/node: 50..100
```

**`BTREE_MIN_DEGREE = 50`** — mỗi node chứa tối đa 99 keys, mô phỏng kích thước vừa 1 disk block 4KB.

**Các thao tác chính:**

- **Insert (Proactive Split):** Khi đi xuống cây, nếu gặp node con đầy (2t−1 keys), tách ngay trước khi đi xuống → đảm bảo parent luôn có chỗ nhận key trung vị.
- **Lookup:** Tại mỗi node: quét tuần tự các keys. Tìm thấy → trả về. Không → đệ quy xuống node con tương ứng.
- **Delete:** Xử lý đầy đủ: (1) Key ở leaf → xóa trực tiếp. (2) Key ở internal → thay bằng predecessor/successor. (3) Trước khi xuống con → đảm bảo con có ≥ t keys bằng borrow hoặc merge.

| Thao tác | Độ phức tạp | Cơ chế |
|----------|------------|--------|
| **Insert** | O(log n) | Proactive split, insert tại leaf |
| **Lookup** | O(log n) | Linear search trong node → đệ quy |
| **Delete** | O(log n) | Predecessor/Successor + Borrow/Merge |

---

### 3.3.4. HTree — Mô phỏng ext3/ext4, O(log b + k)

HTree là cấu trúc trọng tâm, mô phỏng chính xác cơ chế `dx_root` trong nhân Linux ext4.

**Cấu trúc dữ liệu:**

```c
typedef struct {
    DirEntry entries[HTREE_BLOCK_CAPACITY];  // Tối đa 64 entries/block
    int      count;
} HTreeLeafBlock;

typedef struct {
    uint32_t        hash;    // Giá trị hash 32-bit làm ranh giới
    HTreeLeafBlock *block;   // Con trỏ tới leaf block
} HTreeDxEntry;

typedef struct {
    HTreeDxEntry    *dx_entries;      // Mảng index entries (sắp xếp theo hash)
    int             num_dx_entries;
    int             dx_capacity;      // Dung lượng mảng (tự nhân đôi)
    int             count;
    HTreeHashVersion hash_version;
    int             depth;
} HTree;
```

```
┌──────────────────────────────────────────────────────────┐
│ HTree (dx_root)                                          │
│ dx_entries[] (sắp xếp theo hash, binary search):         │
│ ┌──────────┬──────────┬──────────┬──────────┐           │
│ │ hash=0   │ hash=A   │ hash=B   │ hash=C   │           │
│ │ →block₀  │ →block₁  │ →block₂  │ →block₃  │           │
│ └────┬─────┴────┬─────┴────┬─────┴────┬─────┘           │
│      ▼          ▼          ▼          ▼                  │
│ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐             │
│ │LeafBlk │ │LeafBlk │ │LeafBlk │ │LeafBlk │ (64 entries │
│ │E₀..E₆₃│ │E₀..E₆₃│ │E₀..E₆₃│ │E₀..E₆₃│  max/block) │
│ └────────┘ └────────┘ └────────┘ └────────┘             │
└──────────────────────────────────────────────────────────┘
```

**Điểm mấu chốt:** Index node **chỉ lưu hash 32-bit** (4 bytes) thay vì chuỗi tên file (tối đa 255 bytes). Nhờ vậy, một block 4KB chứa được hàng trăm index entries, giữ chiều cao cây ở mức 1-2.

**Hàm băm Half-MD4:** Mô phỏng theo `fs/ext4/hash.c`, gồm 3 rounds (F, G, H) với các phép bitwise mixing tương tự MD4. Hỗ trợ thêm TEA và DJB2 như hash thay thế.

**Luồng Lookup:**
```
filename → Half-MD4(name) → hash 32-bit
  → Binary search trong dx_entries → tìm leaf block
    → Linear scan trong leaf block (tối đa 64 entries)
```

**Split Leaf Block:** Khi block đầy (64 entries): tính hash tất cả entries → sắp xếp theo hash → tách đôi → thêm dx_entry mới.

| Thao tác | Độ phức tạp | Cơ chế |
|----------|------------|--------|
| **Insert** | O(log b) | Hash → binary search dx → append leaf, split nếu đầy |
| **Lookup** | O(log b + k) | Hash → binary search → linear scan leaf block |
| **Delete** | O(log b + k) | Hash → binary search → scan + swap-with-last |

*(b = số leaf blocks, k = số entries/block ≤ 64)*

---

## 3.4. Benchmark Framework

### 3.4.1. Bộ Đếm Thời Gian

```c
#define TIMER_DECLARE()  struct timespec _ts_start, _ts_end
#define TIMER_START()    clock_gettime(CLOCK_MONOTONIC, &_ts_start)
#define TIMER_END()      clock_gettime(CLOCK_MONOTONIC, &_ts_end)
#define TIMER_ELAPSED_NS() \
    ((uint64_t)(_ts_end.tv_sec - _ts_start.tv_sec) * 1000000000ULL + \
     (uint64_t)(_ts_end.tv_nsec - _ts_start.tv_nsec))
```

`CLOCK_MONOTONIC` — đồng hồ đơn điệu, chính xác tới nanosecond, không bị ảnh hưởng bởi thay đổi giờ hệ thống.

### 3.4.2. Kịch Bản và Metrics

**9 mức kích thước:** N = {100, 500, 1K, 5K, 10K, 50K, 100K, 500K, 1M}

Mỗi test case thu thập 8 metrics: insert time, lookup hit/miss time, comparisons hit/miss, delete time, delete comparisons, memory usage.

- **Lookup Hit:** 1.000 lượt tra cứu entries tồn tại.
- **Lookup Miss:** 100 lượt tra cứu entries không tồn tại (đuôi `.xyz`).
- **Delete:** 500 lượt xóa. Cấu trúc được **rebuild hoàn toàn** trước khi test delete.

### 3.4.3. Xuất Kết Quả

- **CSV:** `results/benchmark_results.csv` — dữ liệu thô.
- **PNG:** 6 biểu đồ dark-theme (Matplotlib, 300 DPI) — lookup, insert, delete, comparisons, memory, summary.

---

## 3.5. An Toàn Bộ Nhớ

Kiểm tra bằng Valgrind:
```
Valgrind: 0 bytes leaked, 0 errors, 3,611,188 allocs = 3,611,188 frees
```

Mỗi module cài đặt hàm `*_destroy()` giải phóng bộ nhớ đệ quy đầy đủ.

---

## 3.6. Build System

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm
```

| Lệnh | Chức năng |
|-------|-----------|
| `make` | Build → `./benchmark` |
| `make run` | Build + chạy full benchmark |
| `make valgrind` | Kiểm tra memory leak |
| `make plot` | Sinh biểu đồ từ CSV |

Hỗ trợ 2 chế độ: **Interactive** (`./benchmark`) và **CLI** (`./benchmark --full`).

---

## 3.7. Tổng Kết Thiết Kế

| Tiêu chí | Giải pháp |
|----------|-----------|
| Ngôn ngữ | C (GCC, Linux) — sát lập trình hệ thống |
| Mô phỏng ext4 | DirEntry giống `ext4_dir_entry_2`, Half-MD4 hash |
| Đo lường | `clock_gettime(CLOCK_MONOTONIC)` — nanosecond |
| Tính lặp lại | Seed cố định (`srand(42)`) + Fisher-Yates shuffle |
| An toàn bộ nhớ | Valgrind: 0 errors, 0 leaks / 3.6M allocations |
| Quy mô test | 9 mức, tối đa 1.000.000 entries |
| Version control | Git branches + Pull Requests + 15 commits |
