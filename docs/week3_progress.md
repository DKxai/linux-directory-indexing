# Tuần 3: Thiết lập dự án & Triển khai Linear Search

## 📅 Thời gian: 27/04 – 03/05/2026

---

## 1. Công việc đã hoàn thành

### ✅ Thiết lập cấu trúc dự án
- Tạo project structure hoàn chỉnh với các thư mục: `include/`, `src/`, `scripts/`, `results/`, `docs/`
- Viết `Makefile` với các targets: `all`, `clean`, `run`, `demo`, `plot`, `valgrind`, `help`
- Viết `common.h`: định nghĩa timer macros (`TIMER_START/END` dùng `clock_gettime(CLOCK_MONOTONIC)`), struct `BenchmarkResult`, các hằng số
- Viết `dir_entry.h/c`: struct `DirEntry` mô phỏng `ext4_dir_entry_2`, hàm tạo entries ngẫu nhiên

### ✅ Triển khai Linear Search (Baseline)
- Cấu trúc `LinearDir`: mảng động (dynamic array) chứa `DirEntry`
- Các hàm: `linear_create()`, `linear_insert()`, `linear_lookup()`, `linear_destroy()`, `linear_memory_usage()`
- Đếm số comparisons trong hàm lookup để phân tích complexity
- Hỗ trợ auto-resize khi mảng đầy

### ✅ Unit Test & Kiểm tra
- **Biên dịch**: `make` — 0 warnings, 0 errors
- **Valgrind**: 0 memory leaks, 0 errors
- **Correctness**: lookup trả đúng kết quả cho cả hit và miss

---

## 2. Kết quả đo baseline (Linear Search)

| Số entries (N) | Thời gian lookup trung bình (ns) | Số comparisons trung bình | Bộ nhớ (KB) |
|----------------|-----------------------------------|---------------------------|-------------|
| 100            | 247                               | 51                        | 33          |
| 500            | 679                               | 239                       | 132         |
| 1,000          | 1,411                             | 516                       | 264         |
| 5,000          | 7,548                             | 2,578                     | 2,112       |
| 10,000         | 13,578                            | 4,995                     | 4,224       |
| 100,000        | 172,670                           | 50,077                    | 33,792      |

### Nhận xét
- ✅ **Xác nhận O(n) behavior**: thời gian lookup tăng tuyến tính theo N
- ✅ **Comparisons trung bình ≈ N/2**: đúng với lý thuyết (random search trong mảng)
- ✅ **Comparisons miss = N**: đúng vì phải quét hết mảng mới biết không tìm thấy
- Đây là kết quả **baseline** để so sánh với Hash Table, B-Tree, HTree ở các tuần sau

---

## 3. Vấn đề gặp phải & Giải pháp

| Vấn đề | Giải pháp |
|--------|----------|
| IDE báo lỗi `'benchmark.h' file not found` | Đây là lỗi IDE config (không ảnh hưởng build). Tạo file `.clangd` + `compile_commands.json` để sửa |
| `srand()` cần seed cố định | Dùng `srand(42)` để kết quả reproducible giữa các lần chạy |

---

## 4. Cấu trúc file đã tạo

```
HDH_Project/
├── Makefile                     ✅
├── README.md                    ✅
├── .gitignore                   ✅
├── include/
│   ├── common.h                 ✅ Timer macros, BenchmarkResult, constants
│   ├── dir_entry.h              ✅ DirEntry struct, generators
│   ├── linear_search.h          ✅ Linear Search API
│   ├── hash_table.h             (chuẩn bị cho Tuần 4)
│   ├── btree.h                  (chuẩn bị cho Tuần 5)
│   ├── htree.h                  (chuẩn bị cho Tuần 6)
│   └── benchmark.h              (chuẩn bị cho Tuần 6)
├── src/
│   ├── main.c                   ✅ Entry point + menu
│   ├── dir_entry.c              ✅ Random entry generators
│   ├── linear_search.c          ✅ Linear Search implementation
│   ├── hash_table.c             (Tuần 4)
│   ├── btree.c                  (Tuần 5)
│   ├── htree.c                  (Tuần 6)
│   └── benchmark.c              (Tuần 6)
├── scripts/
│   └── plot_results.py          ✅
└── results/
    └── benchmark_results.csv    ✅ Baseline data
```

---

## 5. Kế hoạch Tuần 4 (04/05 – 10/05)

**Mục tiêu: Triển khai Hash Table**

- [ ] Implement DJB2 hash function
- [ ] Implement `HashDir` với separate chaining (linked list per bucket)
- [ ] Implement auto-resize khi load factor > 0.75
- [ ] Các hàm: `hash_create()`, `hash_insert()`, `hash_lookup()`, `hash_destroy()`
- [ ] Unit test: correctness, collision handling, auto-resize
- [ ] Valgrind: kiểm tra 0 memory leaks
- [ ] So sánh sơ bộ: Linear Search vs Hash Table
