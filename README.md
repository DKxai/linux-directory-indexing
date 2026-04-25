# Improving Directory Lookup Performance Using Indexing Structures

> **Dự án môn Hệ Điều Hành (Operating Systems)**  
> So sánh hiệu năng tra cứu thư mục giữa các cấu trúc chỉ mục.

## 📋 Tổng quan

Chương trình mô phỏng cách filesystem quản lý directory entries và đo hiệu năng các operations: **insert**, **lookup**, và **delete**.

### Tiến độ hiện tại: Tuần 4 — Hash Table

| Tuần | Nội dung | Trạng thái |
|------|----------|-----------|
| 3 | Project setup + Linear Search (insert/lookup/delete) | ✅ Hoàn thành |
| 4 | Hash Table (djb2 + Separate Chaining + Rehash) | ✅ Hoàn thành |
| 5 | B-Tree | ⬜ Chưa bắt đầu |
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
│   └── benchmark.h          # API Benchmark framework
├── src/                     # Source files
│   ├── main.c               # Entry point + menu
│   ├── dir_entry.c          # Tạo directory entries ngẫu nhiên
│   ├── linear_search.c      # Triển khai Linear Search
│   ├── hash_table.c         # Triển khai Hash Table + Rehash
│   └── benchmark.c          # Framework đo hiệu năng (insert/lookup/delete)
├── docs/                    # Báo cáo tiến độ
│   ├── week3_progress.md    # Báo cáo tuần 3
│   └── week4_progress.md    # Báo cáo tuần 4
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

## 📊 Kết quả So Sánh (Linear Search vs Hash Table)

| N | Linear Lookup (ns) | Hash Lookup (ns) | Tốc độ gấp | Linear Comp | Hash Comp |
|---|---|---|---|---|---|
| 100 | ~205 | ~24 | **8.5×** | 51 | 1 |
| 1,000 | ~1,369 | ~23 | **59.5×** | 516 | 1 |
| 10,000 | ~13,841 | ~46 | **301×** | 4,995 | 1 |
| 100,000 | ~205,280 | ~120 | **1,711×** | 50,077 | 1 |
| 1,000,000 | ~5,010,810 | ~199 | **25,179×** | 494,913 | 1 |

### 🛡️ Kiểm tra bộ nhớ (Memory Safety)
Kết quả chạy Valgrind cho cả 2 thuật toán:
```text
== HEAP SUMMARY:
==     in use at exit: 0 bytes in 0 blocks
==   total heap usage: 3,361,582 allocs, 3,361,582 frees, 3,357,513,367 bytes allocated
==
== All heap blocks were freed -- no leaks are possible
== ERROR SUMMARY: 0 errors from 0 contexts
```

### Nhận xét
- ✅ **Hash Table O(1)**: Lookup chỉ cần ~1 comparison bất kể N — khẳng định lý thuyết
- ✅ **Linear O(n)**: comparisons tăng tuyến tính, dùng làm baseline để so sánh
- ✅ **Rehash hoạt động đúng**: Load factor luôn ≤ 0.7, collision rất thấp
- ✅ **Valgrind**: 0 memory leaks, 0 errors cho cả 2 thuật toán
- Hash Table nhanh hơn Linear Search **lên đến 25,000 lần** ở quy mô 1 triệu entries

## 📚 Tài liệu tham khảo

1. Silberschatz, "Operating System Concepts", Chapter 14-15
2. [Linux ext4 Documentation](https://www.kernel.org/doc/html/latest/filesystems/ext4/index.html)
3. Linux kernel source: `fs/ext4/dir.c`, `fs/ext4/namei.c`
4. Cormen (CLRS), "Introduction to Algorithms", Chapter 11 (Hash Tables), Chapter 18 (B-Trees)
5. Daniel J. Bernstein, "djb2 Hash Function"
