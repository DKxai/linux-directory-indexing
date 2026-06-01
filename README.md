# Improving Directory Lookup Performance Using Indexing Structures

> **Dự án môn Hệ Điều Hành (Operating Systems)**  
> Trạng thái: **Hoàn thành (Tuần 8/8)**  
> Mô phỏng, triển khai và so sánh hiệu năng tra cứu thư mục giữa các cấu trúc chỉ mục (Linear Search, Hash Table, B-Tree, HTree).

## 📋 Tổng quan

Dự án phát triển một framework bằng ngôn ngữ C, mô phỏng cách hệ điều hành Linux quản lý directory entries, từ đó thực hiện đánh giá hiệu năng (benchmark) các thao tác: **insert**, **lookup**, và **delete**.

Đặc biệt, dự án đi sâu vào phân tích cấu trúc **HTree** (Hash B-Tree) hiện đang được sử dụng trong các hệ thống file hiện đại như **ext3/ext4**, chứng minh lý do tại sao kiến trúc này tối ưu nhất cho truy xuất ổ đĩa (Disk I/O).

### Tiến độ: Hoàn thành toàn bộ dự án

| Tuần | Nội dung | Trạng thái |
|------|----------|-----------| 
| 3 | Project setup + Linear Search (insert/lookup/delete) | ✅ Hoàn thành |
| 4 | Hash Table (djb2 + Separate Chaining + Rehash) | ✅ Hoàn thành |
| 5 | B-Tree (proactive split + borrow/merge) | ✅ Hoàn thành |
| 6 | HTree (ext4 Half-MD4 + binary search dx_entries) | ✅ Hoàn thành |
| 7 | Benchmark + Visualization (Python/Matplotlib) | ✅ Hoàn thành |
| 8 | Báo cáo tổng kết + Kiến trúc hệ thống | ✅ Hoàn thành |

## 🔧 Yêu cầu hệ thống

- **OS:** Linux (Ubuntu, Fedora, etc.)
- **Compiler:** GCC
- **Công cụ phụ trợ:** `make`, `python3` (kèm thư viện `matplotlib` và `pandas` để vẽ biểu đồ), `valgrind` (cho memory check).

## 🚀 Build & Run

### Build dự án
```bash
make
```

### Chạy benchmark đầy đủ (All sizes & methods)
```bash
make run
# hoặc
./benchmark --full
```

### Chạy chế độ tương tác (Menu CLI)
```bash
make demo
# hoặc
./benchmark
```

### Vẽ biểu đồ từ dữ liệu Benchmark (cần Python)
```bash
make plot
```

### Kiểm tra an toàn bộ nhớ
```bash
make valgrind
```

### Dọn dẹp build files
```bash
make clean
```

## 📁 Cấu trúc dự án

```
linux-directory-indexing/
├── Makefile                 # Build system
├── README.md                # Tài liệu dự án (File này)
├── include/                 # Header files (Định nghĩa API và Structs)
│   ├── common.h             
│   ├── dir_entry.h          
│   ├── linear_search.h      
│   ├── hash_table.h         
│   ├── btree.h              
│   ├── htree.h              
│   └── benchmark.h          
├── src/                     # Source files (Triển khai thuật toán)
│   ├── main.c               # Entry point + Menu giao diện
│   ├── dir_entry.c          # Trình tạo directory entries ngẫu nhiên
│   ├── linear_search.c      
│   ├── hash_table.c         
│   ├── btree.c              
│   ├── htree.c              
│   └── benchmark.c          # Framework đo đạc hiệu năng
├── docs/                    # Tài liệu & Báo cáo
│   ├── week*_progress.md    # Báo cáo tiến độ từng tuần (3-8)
│   ├── architecture_overview.md # Tổng quan kiến trúc hệ thống
│   └── final_report.md      # Báo cáo tổng kết dự án
├── results/                 # Kết quả (CSV + Biểu đồ PNG)
└── scripts/                 # Scripts phụ trợ
    └── visualize.py         # Script Python vẽ biểu đồ từ file CSV
```

## 📊 Operations đã triển khai

### Linear Search (Baseline)
Lưu trữ các entry trong một mảng cấp phát động. Duyệt tuần tự từ đầu đến cuối. (Độ phức tạp $O(N)$).

### Hash Table
Sử dụng hàm băm **djb2**, kết hợp kỹ thuật **Separate Chaining** để xử lý đụng độ. Tự động Rehash x2 khi load factor > 70%. Tốc độ truy xuất nhanh nhất ($O(1)$) nhưng phân mảnh bộ nhớ nhiều.

### B-Tree
Cây tìm kiếm đa phân tự cân bằng với kỹ thuật Proactive Split. Node được cấu hình mô phỏng kích thước disk block. (Độ phức tạp $O(\log N)$).

### HTree (ext4 Hashed B-Tree)
Sử dụng hàm băm **Half-MD4** (chuẩn ext4). Index Node chỉ lưu mã băm (32-bit integer) thay vì chuỗi tên file, giúp số lượng con của mỗi node (fan-out) cực lớn, độ cao cây thấp. Giải pháp hoàn hảo cho truy cập khối ổ đĩa. (Độ phức tạp $O(\log_b N + K)$).

## 🏆 Tóm tắt Kết quả Benchmark

*(Số liệu đo lường ở quy mô 1.000.000 entries.`)*

| Phương pháp | Lượt so sánh (Hit) | Thời gian Lookup (ns) | Thời gian Delete (ns) | Tốc độ Lookup so với Linear |
|-------------|:---:|----------------:|----------------:|-------------------|
| **Linear Search** | 494,913 | 87,705,671 | 182,582,981 | Tiêu chuẩn cơ sở (1x) |
| **Hash Table** | 1 | 4,183 | 1,854 | ~20,967x |
| **B-Tree** | 100 | 55,602 | 72,459 | ~1,578x |
| **HTree** | 37 | **24,914** | **44,397** | **~3,519x** |

### 🛡️ Memory Safety (Valgrind)
Hệ thống quản lý bộ nhớ an toàn tuyệt đối. Đã cấp phát và giải phóng hơn 3,6 triệu khối lượng bộ nhớ động:
```text
== HEAP SUMMARY:
==     in use at exit: 0 bytes in 0 blocks
==   total heap usage: 3,611,188 allocs, 3,611,188 frees, 5,944,701,288 bytes allocated
==
== All heap blocks were freed -- no leaks are possible
== ERROR SUMMARY: 0 errors from 0 contexts
```

## 📚 Tài liệu tham khảo

1. Silberschatz, "Operating System Concepts", Chapter 14-15
2. [Linux ext4 Documentation](https://www.kernel.org/doc/html/latest/filesystems/ext4/index.html)
3. Linux kernel source: `fs/ext4/dir.c`, `fs/ext4/namei.c`, `fs/ext4/hash.c`
4. Cormen (CLRS), "Introduction to Algorithms", Chapter 11 (Hash Tables), Chapter 18 (B-Trees)
5. Daniel J. Bernstein, "djb2 Hash Function"
6. Daniel Phillips, "A Directory Index for EXT2" (HTree design paper)
7. Rivest, R., "The MD4 Message-Digest Algorithm", RFC 1320
