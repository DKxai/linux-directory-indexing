# Improving Directory Lookup Performance Using Indexing Structures

> **Dự án môn Hệ Điều Hành (Operating Systems)**  
> So sánh hiệu năng tra cứu thư mục giữa các cấu trúc chỉ mục.

## 📋 Tổng quan

Chương trình mô phỏng cách filesystem quản lý directory entries và đo hiệu năng các operations: **insert**, **lookup**, và **delete**.

### Tiến độ hiện tại: Tuần 3 — Linear Search Baseline

| Tuần | Nội dung | Trạng thái |
|------|----------|-----------|
| 3 | Project setup + Linear Search (insert/lookup/delete) | ✅ Hoàn thành |
| 4 | Hash Table | ⬜ Chưa bắt đầu |
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
│   └── benchmark.h          # API Benchmark framework
├── src/                     # Source files
│   ├── main.c               # Entry point + menu
│   ├── dir_entry.c          # Tạo directory entries ngẫu nhiên
│   ├── linear_search.c      # Triển khai Linear Search
│   └── benchmark.c          # Framework đo hiệu năng (insert/lookup/delete)
├── results/                 # Kết quả benchmark (CSV)
└── scripts/                 # Scripts phụ trợ
```

## 📊 Operations đã triển khai (Week 3)

| Operation | Hàm | Complexity | Mô tả |
|-----------|-----|------------|--------|
| **Create** | `linear_create()` | O(1) | Khởi tạo mảng động |
| **Insert** | `linear_insert()` | O(1) amortized | Append + auto-resize ×2 |
| **Lookup** | `linear_lookup()` | O(n) | Quét tuần tự, đếm comparisons |
| **Delete** | `linear_delete()` | O(n) | Quét + swap-with-last |
| **Destroy** | `linear_destroy()` | O(1) | Free bộ nhớ |

## 📊 Kết quả Baseline (Linear Search)

| N | Lookup (ns) | Comparisons | Delete (ns) | Del Comp | Memory (KB) |
|---|------------|-------------|-------------|----------|-------------|
| 100 | ~215 | ~51 | ~159 | ~50 | 33 |
| 1,000 | ~1,372 | ~516 | ~2,342 | ~750 | 264 |
| 10,000 | ~13,119 | ~4,995 | ~25,966 | ~9,750 | 4,224 |
| 100,000 | ~220,156 | ~50,077 | ~623,296 | ~99,750 | 33,792 |
| 1,000,000 | ~5,332,360 | ~494,913 | ~10,906,672 | ~999,750 | 270,336 |

### 🛡️ Kiểm tra bộ nhớ (Memory Safety)
Kết quả chạy Valgrind xác nhận chương trình không có lỗi quản lý bộ nhớ:
```text
== HEAP SUMMARY:
==     in use at exit: 0 bytes in 0 blocks
==   total heap usage: 14,199 allocs, 14,199 frees, 2,336,214,521 bytes allocated
==
== All heap blocks were freed -- no leaks are possible
== ERROR SUMMARY: 0 errors from 0 contexts
```

### Nhận xét
- ✅ **Lookup O(n)**: comparisons trung bình ≈ N/2, thời gian tăng tuyến tính
- ✅ **Delete O(n)**: comparisons trung bình ≈ N (worst case quét hết mảng)
- ✅ **Valgrind**: 0 memory leaks, 0 errors
- Đây là **baseline** để so sánh với Hash Table, B-Tree, HTree ở các tuần sau

## 📚 Tài liệu tham khảo

1. Silberschatz, "Operating System Concepts", Chapter 14-15
2. [Linux ext4 Documentation](https://www.kernel.org/doc/html/latest/filesystems/ext4/index.html)
3. Linux kernel source: `fs/ext4/dir.c`, `fs/ext4/namei.c`
4. Cormen (CLRS), "Introduction to Algorithms", Chapter 18 (B-Trees)
