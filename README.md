# Improving Directory Lookup Performance Using Indexing Structures

> **Dự án môn Hệ Điều Hành (Operating Systems)**  
> So sánh hiệu năng tra cứu thư mục giữa các cấu trúc chỉ mục.

## 📋 Tổng quan

Chương trình mô phỏng cách filesystem quản lý directory entries và đo hiệu năng các operations: **insert**, **lookup**, và **delete**.

### Tiến độ hiện tại: Tuần 5 — B-Tree

| Tuần | Nội dung | Trạng thái |
|------|----------|-----------|
| 3 | Project setup + Linear Search (insert/lookup/delete) | ✅ Hoàn thành |
| 4 | Hash Table (djb2 + Separate Chaining + Rehash) | ✅ Hoàn thành |
| 5 | B-Tree (proactive split + borrow/merge) | ✅ Hoàn thành |
| 6 | HTree (ext4) + Benchmark Framework | ⬜ Chưa bắt đầu |
| 7 | Benchmark + Visualization | ⬜ Chưa bắt đầu |
| 8 | Phân tích + Báo cáo | ⬜ Chưa bắt đầu |

## 🔧 Yêu cầu hệ thống

- **OS:** Linux (Ubuntu, Fedora, etc.)
- **Compiler:** GCC
- **valgrind** (cho memory check, tùy chọn)

## 🚀 Build & Run

### Build
```bash
make
```

### Chạy benchmark đầy đủ
```bash
make run
# hoặc
./benchmark --full
```

### Chạy chế độ tương tác (menu)
```bash
make demo
# hoặc
./benchmark
```

### Kiểm tra memory leak
```bash
make valgrind
```

### Dọn dẹp
```bash
make clean
```

## 📁 Cấu trúc dự án

```
linux-directory-indexing/
├── Makefile                 # Build system
├── README.md                # File này
├── include/                 # Header files
│   ├── common.h             # Types, macros, constants chung
│   ├── dir_entry.h          # Cấu trúc directory entry
│   ├── linear_search.h      # API Linear Search (create/insert/lookup/delete)
│   ├── hash_table.h         # API Hash Table (djb2 + Separate Chaining)
│   ├── btree.h              # API B-Tree (proactive split, borrow/merge)
│   └── benchmark.h          # API Benchmark framework
├── src/                     # Source files
│   ├── main.c               # Entry point + menu
│   ├── dir_entry.c          # Tạo directory entries ngẫu nhiên
│   ├── linear_search.c      # Triển khai Linear Search
│   ├── hash_table.c         # Triển khai Hash Table + Rehash
│   ├── btree.c              # Triển khai B-Tree (insert/lookup/delete)
│   └── benchmark.c          # Framework đo hiệu năng (insert/lookup/delete)
├── docs/                    # Báo cáo tiến độ
│   ├── week3_progress.md    # Báo cáo tuần 3
│   ├── week4_progress.md    # Báo cáo tuần 4
│   └── week5_progress.md    # Báo cáo tuần 5
├── results/                 # Kết quả benchmark (CSV)
└── scripts/                 # Scripts phụ trợ
```

## 📊 Operations đã triển khai

### Linear Search (Week 3)

| Operation | Hàm | Complexity | Mô tả |
|-----------|-----|------------|--------|
| **Create** | `linear_create()` | O(1) | Khởi tạo mảng động |
| **Insert** | `linear_insert()` | O(1) amortized | Append + auto-resize ×2 |
| **Lookup** | `linear_lookup()` | O(n) | Quét tuần tự, đếm comparisons |
| **Delete** | `linear_delete()` | O(n) | Quét + swap-with-last |
| **Destroy** | `linear_destroy()` | O(1) | Free bộ nhớ |

### Hash Table (Week 4)

| Operation | Hàm | Complexity | Mô tả |
|-----------|-----|------------|--------|
| **Create** | `hash_create()` | O(1) | Khởi tạo bảng băm 256 buckets |
| **Insert** | `hash_insert()` | O(1) avg | Prepend vào chain + auto-rehash |
| **Lookup** | `hash_lookup()` | O(1) avg | Hash → bucket → duyệt chain |
| **Delete** | `hash_delete()` | O(1) avg | Hash → bucket → xóa node |
| **Rehash** | (internal) | O(n) | Tự nhân đôi khi load > 70% |
| **Destroy** | `hash_destroy()` | O(n) | Free tất cả chains + buckets |

### B-Tree (Week 5)

| Operation | Hàm | Complexity | Mô tả |
|-----------|-----|------------|--------|
| **Create** | `btree_create()` | O(1) | Khởi tạo cây rỗng, root = leaf, t=50 |
| **Insert** | `btree_insert()` | O(log n) | Proactive split khi node đầy (2t-1 keys) |
| **Lookup** | `btree_lookup()` | O(log n) | Tìm tuần tự trong node → đệ quy xuống con |
| **Delete** | `btree_delete()` | O(log n) | Borrow/merge + predecessor/successor |
| **Height** | `btree_height()` | O(log n) | Duyệt từ root xuống leaf |
| **Destroy** | `btree_destroy()` | O(n) | Free đệ quy toàn bộ cây |

## 📊 Kết quả So Sánh (Linear Search vs Hash Table vs B-Tree)

### Lookup Performance

| N | Linear (ns) | Hash (ns) | B-Tree (ns) | Linear Comp | Hash Comp | B-Tree Comp |
|---|---|---|---|---|---|---|
| 100 | 531 | 75 | 341 | 51 | 1 | 25 |
| 1,000 | 2,943 | 97 | 458 | 516 | 1 | 38 |
| 10,000 | 49,043 | 104 | 1,006 | 4,995 | 1 | 74 |
| 100,000 | 623,183 | 229 | 1,556 | 50,077 | 1 | 88 |
| 1,000,000 | 5,475,223 | 346 | 1,479 | 494,913 | 1 | 100 |

### Delete Performance

| N | Linear (ns) | Hash (ns) | B-Tree (ns) | Linear Comp | Hash Comp | B-Tree Comp |
|---|---|---|---|---|---|---|
| 100 | 525 | 122 | 632 | 50 | 1 | 1 |
| 1,000 | 3,687 | 121 | 1,561 | 750 | 1 | 2 |
| 10,000 | 92,625 | 170 | 2,494 | 9,750 | 1 | 3 |
| 100,000 | 1,428,237 | 121 | 2,191 | 99,750 | 1 | 3 |
| 1,000,000 | 11,348,133 | 48 | 1,964 | 999,750 | 1 | 4 |

### 🛡️ Kiểm tra bộ nhớ (Memory Safety)
Kết quả chạy Valgrind cho cả 3 thuật toán:
```text
== HEAP SUMMARY:
==     in use at exit: 0 bytes in 0 blocks
==   total heap usage: 3,522,231 allocs, 3,522,231 frees, 4,675,558,280 bytes allocated
==
== All heap blocks were freed -- no leaks are possible
== ERROR SUMMARY: 0 errors from 0 contexts
```

### Nhận xét
- ✅ **Hash Table O(1)**: Lookup chỉ cần ~1 comparison bất kể N — nhanh nhất
- ✅ **B-Tree O(log n)**: Comparisons tăng rất chậm (25 → 100 khi N: 100 → 1M) — cân bằng tốt giữa tốc độ và cấu trúc
- ✅ **Linear O(n)**: comparisons tăng tuyến tính, dùng làm baseline
- ✅ **Valgrind**: 0 memory leaks, 0 errors cho cả 3 thuật toán
- B-Tree nhanh hơn Linear Search **3,700 lần** nhưng chậm hơn Hash Table ~4-7 lần ở 1M entries
- B-Tree tree height chỉ = 2 ở 1M entries (mỗi node chứa tối đa 99 keys)

## 📚 Tài liệu tham khảo

1. Silberschatz, "Operating System Concepts", Chapter 14-15
2. [Linux ext4 Documentation](https://www.kernel.org/doc/html/latest/filesystems/ext4/index.html)
3. Linux kernel source: `fs/ext4/dir.c`, `fs/ext4/namei.c`
4. Cormen (CLRS), "Introduction to Algorithms", Chapter 11 (Hash Tables), Chapter 18 (B-Trees)
5. Daniel J. Bernstein, "djb2 Hash Function"
