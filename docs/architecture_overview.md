# 🏗️ Tổng Quan Kiến Trúc — Linux Directory Indexing

> **Dự án:** Improving Directory Lookup Performance Using Indexing Structures  
> **Ngôn ngữ:** C (GCC, Linux) | **Tổng:** ~2,840 dòng code  
> **Cập nhật:** 28/05/2026 — Tuần 8 (Final)

---

## 1. Kiến trúc Tổng Quan

```
┌─────────────────────────────────────────────────────────────────┐
│                        main.c (470 LOC)                        │
│              Terminal UI + CLI Router + Menu Loop               │
├─────────────────────────────────────────────────────────────────┤
│                    benchmark.c (487 LOC)                        │
│        Benchmark Framework: run / measure / export CSV          │
├────────────┬────────────┬──────────────┬────────────────────────┤
│ linear_    │ hash_      │  btree.c     │     htree.c            │
│ search.c   │ table.c    │  (539 LOC)   │     (495 LOC)          │
│ (107 LOC)  │ (249 LOC)  │              │                        │
│ O(n)       │ O(1) avg   │  O(log n)    │  O(log b + k)          │
├────────────┴────────────┴──────────────┴────────────────────────┤
│                     dir_entry.c (122 LOC)                       │
│            DirEntry generator + test data utilities             │
├─────────────────────────────────────────────────────────────────┤
│                      common.h (81 LOC)                          │
│     Types, Constants, Timer Macros, BenchmarkResult struct      │
└─────────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────┐
│                  scripts/visualize.py (449 LOC)                 │
│     Matplotlib dark-theme charts (6 loại biểu đồ PNG)          │
└─────────────────────────────────────────────────────────────────┘
```

### Luồng dữ liệu chính

```
User Input → main.c (menu/CLI)
  → benchmark.c (run tests)
    → dir_entry.c (generate N random entries)
    → 4 indexing modules (insert/lookup/delete)
    → BenchmarkResult[] (timing + comparisons + memory)
  → export_csv() → results/benchmark_results.csv
  → visualize.py → results/*.png (6 charts)
```

---

## 2. Cấu Trúc Thư Mục

```
linux-directory-indexing/
├── Makefile                    # Build: gcc -O2 -Wall -Wextra -Iinclude
├── README.md                   # Tài liệu dự án
├── include/                    # Header files (API contracts)
│   ├── common.h                # Shared types, constants, timer macros
│   ├── dir_entry.h             # DirEntry struct + generator API
│   ├── linear_search.h         # Linear Search API
│   ├── hash_table.h            # Hash Table API
│   ├── btree.h                 # B-Tree API
│   ├── htree.h                 # HTree API
│   └── benchmark.h             # Benchmark framework API
├── src/                        # Implementation files
│   ├── main.c                  # Entry point + terminal UI
│   ├── dir_entry.c             # Random entry generator
│   ├── linear_search.c         # Linear Search implementation
│   ├── hash_table.c            # Hash Table + rehash
│   ├── btree.c                 # B-Tree + proactive split + delete
│   ├── htree.c                 # HTree (ext4) + Half-MD4 hash
│   └── benchmark.c             # Benchmark engine + CSV export
├── scripts/
│   └── visualize.py            # Dark-theme chart generator
├── results/                    # Output
│   ├── benchmark_results.csv   # Raw data
│   └── *.png                   # 6 visualization charts
└── docs/                       # Weekly progress reports
    ├── week3_progress.md
    ├── week4_progress.md
    ├── week5_progress.md
    ├── week6_progress.md
    ├── week7_progress.md
    └── architecture_overview.md  # (File này)
```

---

## 3. Chi Tiết Các Module

### 3.1. common.h — Nền tảng chung

**Vai trò:** Định nghĩa types, constants, macros dùng chung toàn project.

| Thành phần | Mô tả |
|-----------|-------|
| `MAX_FILENAME_LEN` | 255 — giới hạn tên file (giống ext4) |
| `BLOCK_SIZE` | 4096 — mô phỏng disk block 4KB |
| `TEST_SIZES[9]` | {100, 500, 1K, 5K, 10K, 50K, 100K, 500K, 1M} |
| `NUM_LOOKUPS_HIT` | 1000 lookup/test |
| `NUM_LOOKUPS_MISS` | 100 lookup miss/test |
| `NUM_DELETES` | 500 delete/test |
| `TIMER_*` macros | `clock_gettime(CLOCK_MONOTONIC)` — nanosecond |
| `BenchmarkResult` | Struct chứa toàn bộ metrics đo được |

```c
// Timer macros — đo chính xác nanosecond
#define TIMER_DECLARE()  struct timespec _ts_start, _ts_end
#define TIMER_START()    clock_gettime(CLOCK_MONOTONIC, &_ts_start)
#define TIMER_END()      clock_gettime(CLOCK_MONOTONIC, &_ts_end)
#define TIMER_ELAPSED_NS() ((uint64_t)(...))
```

### 3.2. dir_entry — Mô phỏng ext4 Directory Entry

**Cấu trúc DirEntry** (mô phỏng `ext4_dir_entry_2`):

```c
typedef struct {
    uint32_t inode;                       // Số inode
    uint16_t rec_len;                     // Độ dài record
    uint8_t  name_len;                    // Độ dài tên
    uint8_t  file_type;                   // FT_REG_FILE, FT_DIR, ...
    char     name[MAX_FILENAME_LEN + 1];  // Tên file
} DirEntry;
```

**Data Generator:**
- 32 prefixes × 30 extensions → filename đa dạng
- Fisher-Yates shuffle → random hóa thứ tự
- Seed cố định (42) → kết quả reproducible

### 3.3. Linear Search — Baseline O(n)

```
┌──────────────────────────────────────────────┐
│ LinearDir                                     │
│ ┌─────┬─────┬─────┬─────┬─────┬──────────┐  │
│ │ E₀  │ E₁  │ E₂  │ ... │ Eₙ₋₁│ (empty)  │  │
│ └─────┴─────┴─────┴─────┴─────┴──────────┘  │
│ count=n              capacity (auto ×2)       │
└──────────────────────────────────────────────┘
```

| Operation | Complexity | Cơ chế |
|-----------|-----------|--------|
| Insert | O(1) amortized | Append cuối + auto-resize ×2 |
| Lookup | O(n) | Scan tuần tự từ đầu |
| Delete | O(n) scan + O(1) xóa | Tìm → swap-with-last → giảm count |

### 3.4. Hash Table — O(1) Average

```
┌─────────────────────────────────────────────────┐
│ HashTable (num_buckets, count)                   │
│                                                  │
│ buckets[0] → [Node] → [Node] → NULL             │
│ buckets[1] → NULL                                │
│ buckets[2] → [Node] → NULL                       │
│ buckets[3] → [Node] → [Node] → [Node] → NULL    │
│   ...                                            │
│ buckets[m-1] → NULL                              │
│                                                  │
│ Hash: djb2(name) = hash * 33 + c                 │
│ Rehash: khi load_factor > 0.7 → ×2 buckets      │
└─────────────────────────────────────────────────┘
```

| Operation | Complexity | Cơ chế |
|-----------|-----------|--------|
| Insert | O(1) avg | djb2 hash → prepend vào chain |
| Lookup | O(1) avg | djb2 hash → duyệt chain |
| Delete | O(1) avg | djb2 hash → tìm & xóa node |
| Rehash | O(n) | Nhân đôi buckets, re-distribute |

**Collision Resolution:** Separate Chaining (Singly Linked List)

### 3.5. B-Tree — O(log n)

```
                    ┌─────────────────────┐
                    │  [K₁ | K₂ | ... ]   │  ← Root (max 99 keys)
                    └──┬────┬────┬────┬───┘
                       │    │    │    │
              ┌────────┘    │    │    └────────┐
              ▼             ▼    ▼             ▼
        ┌──────────┐  ┌──────────┐      ┌──────────┐
        │ [keys...]│  │ [keys...]│      │ [keys...]│  ← Internal
        └──┬──┬──┬─┘  └──┬──┬──┬─┘      └──┬──┬──┬─┘
           │  │  │       │  │  │            │  │  │
           ▼  ▼  ▼       ▼  ▼  ▼            ▼  ▼  ▼
         [leaf] [leaf] [leaf] [leaf]      [leaf] [leaf]

  t = 50 (min degree)
  Keys/node: 49 ≤ k ≤ 99 (root: 1 ≤ k ≤ 99)
  Children/node: 50 ≤ c ≤ 100
  Height ở 1M entries: ~3 levels
```

| Operation | Complexity | Cơ chế |
|-----------|-----------|--------|
| Insert | O(log n) | Proactive split (tách trước khi xuống) |
| Lookup | O(log n) | Linear search trong node → đệ quy xuống con |
| Delete | O(log n) | Predecessor/Successor + Borrow/Merge |

**Các hàm delete quan trọng:**
- `find_key_index()` — tìm vị trí key
- `get_predecessor()` / `get_successor()` — thay thế key internal
- `fill_child()` — borrow từ sibling trái/phải
- `merge_children()` — gộp 2 node con + key cha

### 3.6. HTree — ext4 Hashed B-Tree O(log b + k)

```
┌──────────────────────────────────────────────────────────────┐
│ HTree (dx_root)                                              │
│                                                              │
│ dx_entries[] (sorted by hash, binary search):                │
│ ┌──────────┬──────────┬──────────┬──────────┬─────────┐     │
│ │ hash=0   │ hash=A   │ hash=B   │ hash=C   │  ...    │     │
│ │ →block₀  │ →block₁  │ →block₂  │ →block₃  │         │     │
│ └────┬─────┴────┬─────┴────┬─────┴────┬─────┴─────────┘     │
│      ▼          ▼          ▼          ▼                      │
│ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐                 │
│ │LeafBlk │ │LeafBlk │ │LeafBlk │ │LeafBlk │  (64 entries    │
│ │E₀..E₆₃│ │E₀..E₆₃│ │E₀..E₆₃│ │E₀..E₆₃│   max/block)    │
│ └────────┘ └────────┘ └────────┘ └────────┘                 │
│                                                              │
│ Hash: Half-MD4 (3 rounds: F, G, H)                          │
│ Split: khi block đầy → sort by hash → tách đôi              │
└──────────────────────────────────────────────────────────────┘

Lookup flow:
  filename → Half-MD4(name) → 32-bit hash
    → binary search dx_entries → tìm leaf block
      → linear scan trong block (max 64 entries)
```

| Operation | Complexity | Cơ chế |
|-----------|-----------|--------|
| Insert | O(log b) | Hash → binary search dx → append leaf, split nếu đầy |
| Lookup | O(log b + k) | Hash → binary search → linear scan leaf block |
| Delete | O(log b + k) | Hash → binary search → scan + swap-with-last |

**3 Hash Versions được hỗ trợ:**

| Version | Thuật toán | Nguồn gốc |
|---------|-----------|-----------|
| `HTREE_HASH_HALF_MD4` | Half-MD4 (3 rounds) | ext4 default (`fs/ext4/hash.c`) |
| `HTREE_HASH_TEA` | Tiny Encryption Algorithm | ext4 alternative |
| `HTREE_HASH_DJB2` | djb2 (hash×33+c) | Daniel J. Bernstein |

### 3.7. Benchmark Framework

```
run_all_benchmarks()
  │
  ├── for size in [100, 500, 1K, 5K, 10K, 50K, 100K, 500K, 1M]:
  │     ├── generate_random_entries(size)
  │     ├── benchmark_linear(size, entries)  → BenchmarkResult
  │     ├── benchmark_hash(size, entries)    → BenchmarkResult
  │     ├── benchmark_btree(size, entries)   → BenchmarkResult
  │     └── benchmark_htree(size, entries)   → BenchmarkResult
  │
  ├── print_results_table()  → ASCII table
  ├── export_csv()           → results/benchmark_results.csv
  └── auto_visualize()       → python3 scripts/visualize.py
                                → 6 charts PNG (300 DPI)
```

**Metrics đo cho mỗi test case:**

| Metric | Đơn vị | Cách đo |
|--------|--------|---------|
| `insert_time_ns` | ns | Tổng thời gian insert N entries |
| `avg_lookup_hit_ns` | ns | Trung bình 1000 lookup (entries tồn tại) |
| `avg_lookup_miss_ns` | ns | Trung bình 100 lookup (entries không tồn tại) |
| `avg_comparisons_hit` | count | Trung bình comparisons khi lookup hit |
| `avg_comparisons_miss` | count | Trung bình comparisons khi lookup miss |
| `avg_delete_time_ns` | ns | Trung bình 500 deletes |
| `avg_delete_comparisons` | count | Trung bình comparisons khi delete |
| `memory_usage_bytes` | bytes | Tổng bộ nhớ cấp phát (tính bằng code) |

---

## 4. Build System

```makefile
CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -Iinclude
LDFLAGS  = -lm

# Targets chính:
make            # Build → ./benchmark
make run        # Build + chạy full benchmark
make demo       # Build + chạy interactive mode
make valgrind   # Memory leak check
make plot       # Generate charts từ CSV
make clean      # Dọn dẹp
```

---

## 5. Terminal UI

```
  ██████╗ ██╗██████╗    ██╗      ██████╗  ██████╗ ██╗  ██╗
  ██╔══██╗██║██╔══██╗   ██║     ██╔═══██╗██╔═══██╗██║ ██╔╝
  ██║  ██║██║██████╔╝   ██║     ██║   ██║██║   ██║█████╔╝
  ██║  ██║██║██╔══██╗   ██║     ██║   ██║██║   ██║██╔═██╗
  ██████╔╝██║██║  ██║   ███████╗╚██████╔╝╚██████╔╝██║  ██╗
  ╚═════╝ ╚═╝╚═╝  ╚═╝   ╚══════╝ ╚═════╝  ╚═════╝ ╚═╝  ╚═╝

  ┌────────────────────────────────────────────────────────────┐
  │   Directory Lookup Performance Benchmark                   │
  │   Operating Systems Project                                │
  ├────────────────────────────────────────────────────────────┤
  │   [1] Run full benchmark          (all 4 methods)          │
  │   [2] Run single method benchmark                          │
  │   [3] Interactive demo            (compare live)            │
  │   [4] View results summary                                 │
  │   [5] Export results to CSV                                 │
  │   [0] Exit                                                 │
  └────────────────────────────────────────────────────────────┘
```

- ANSI color: Cyan border, Green/Blue/Magenta/Orange/Yellow/Red options
- CLI mode: `./benchmark --full` (skip menu), `./benchmark --help`

---

## 6. Kết Quả Benchmark (Tóm tắt)

### Lookup @ 1,000,000 entries

| Method | Time (ns) | Comparisons | So với Linear |
|--------|-----------|-------------|---------------|
| Linear Search | 5,311,564 | 494,913 | baseline |
| Hash Table | 203 | 1 | **26,164× nhanh hơn** |
| B-Tree | 1,028 | 100 | **5,167× nhanh hơn** |
| HTree | 795 | 37 | **6,680× nhanh hơn** |

### Delete @ 1,000,000 entries

| Method | Time (ns) | Comparisons |
|--------|-----------|-------------|
| Linear Search | 11,335,248 | 999,750 |
| Hash Table | 39 | 1 |
| B-Tree | 2,727 | 4 |
| HTree | 911 | 60 |

### Memory Safety
```
Valgrind: 0 bytes leaked, 0 errors, 3,611,188 allocs = 3,611,188 frees
```

---

## 7. Git History (15 commits)

```
a4df7e1 ui: make terminal menu border 100% aligned
67886de ui: fix ASCII art DIR LOOK
4558cd5 feat: auto-generate visualization charts
6edbb15 ui: redesign terminal menu + dark theme charts
d0c6563 feat: week7 - benchmark visualization
50aa93f fea/Htree
d51e8f3 fea/Implement Btree
6d29005 Merge PR #2: feature/hash-table
7e89809 feat: implement benchmark updates
643aa66 feat: implement hash table
a74a67c Merge PR #1: feature/linear-search
f14ecc4 docs: add valgrind results
f3b5fe3 feat: add linear_delete + delete metrics
4b8443b Fix: Remove docs/
31fe435 feat: Linear Search implementation
```

**Branching strategy:** Feature branches → Pull Request → Merge to main

---

## 8. Tài Liệu Tham Khảo

1. Silberschatz — *"Operating System Concepts"*, Ch. 14-15
2. [Linux ext4 Documentation](https://www.kernel.org/doc/html/latest/filesystems/ext4/)
3. Linux kernel: `fs/ext4/dir.c`, `fs/ext4/namei.c`, `fs/ext4/hash.c`
4. CLRS — *"Introduction to Algorithms"*, Ch. 11 (Hash), Ch. 18 (B-Trees)
5. Daniel J. Bernstein — djb2 Hash Function
6. Daniel Phillips — *"A Directory Index for EXT2"*
7. RFC 1320 — MD4 Message-Digest Algorithm
