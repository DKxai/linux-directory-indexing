# Chương 4: Kết Quả Thực Nghiệm và Phân Tích

## 4.1. Môi Trường Thực Nghiệm

**Cấu hình hệ thống:**
- Hệ điều hành: Linux (Ubuntu)
- Trình biên dịch: GCC với cờ `-Wall -Wextra -O2`
- Bộ đếm thời gian: `clock_gettime(CLOCK_MONOTONIC)` — chính xác nanosecond
- Kiểm tra bộ nhớ: Valgrind `--leak-check=full`

**Tham số benchmark:**
- Kích thước test: N ∈ {100, 500, 1.000, 5.000, 10.000, 50.000, 100.000, 500.000, 1.000.000}
- Mỗi mức kích thước thực hiện: 1.000 lookup hit + 100 lookup miss + 500 delete
- Seed ngẫu nhiên cố định (`srand(42)`) đảm bảo tính lặp lại

---

## 4.2. Kết Quả Tổng Hợp Tại Quy Mô 1.000.000 Entries

Bảng dưới đây tổng hợp các metrics chính tại quy mô lớn nhất (N = 1.000.000):

| Phương pháp | Lookup Hit (ns) | Lookup Miss (ns) | Comp Hit | Comp Miss | Delete (ns) | Del Comp | Memory (KB) |
|-------------|----------------:|------------------:|---------:|----------:|------------:|---------:|------------:|
| **Linear Search** | 86.489.554 | 173.427.321 | 494.913 | 1.000.000 | 178.826.904 | 999.750 | 270.336 |
| **Hash Table** | 4.002 | 2.363 | 1 | 0 | 1.970 | 1 | 282.009 |
| **B-Tree** | 49.160 | 757 | 100 | 4 | 63.094 | 4 | 386.418 |
| **HTree** | 24.189 | 80.908 | 37 | 105 | 40.988 | 60 | 371.004 |

### Speedup so với Linear Search (Lookup Hit):

| Phương pháp | Thời gian (ns) | Speedup |
|-------------|---------------:|--------:|
| Linear Search | 86.489.554 | 1× (baseline) |
| Hash Table | 4.002 | **~21.611×** |
| B-Tree | 49.160 | **~1.759×** |
| HTree | 24.189 | **~3.575×** |

---

## 4.3. Phân Tích Chi Tiết Hiệu Năng Lookup

### 4.3.1. Lookup Hit — Tra Cứu Entries Tồn Tại

Dữ liệu lookup hit theo từng mức kích thước N:

| N | Linear (ns) | Hash (ns) | B-Tree (ns) | HTree (ns) |
|--:|------------:|----------:|------------:|-----------:|
| 100 | 19.969 | 4.660 | 6.628 | 9.650 |
| 500 | 62.517 | 5.229 | 13.748 | 12.849 |
| 1.000 | 134.924 | 5.377 | 16.069 | 12.028 |
| 5.000 | 638.349 | 2.067 | 30.429 | 12.057 |
| 10.000 | 1.322.921 | 1.264 | 19.454 | 9.520 |
| 50.000 | 2.379.757 | 980 | 15.820 | 5.701 |
| 100.000 | 6.324.988 | 1.135 | 19.025 | 8.017 |
| 500.000 | 42.407.102 | 4.929 | 38.472 | 19.479 |
| 1.000.000 | 86.489.554 | 4.002 | 49.160 | 24.189 |

**Nhận xét:**
- **Linear Search** tăng tuyến tính rõ rệt: từ ~20μs (N=100) lên ~86ms (N=1M) — đúng như lý thuyết O(n).
- **Hash Table** gần như **không đổi** (~1–5μs) bất kể kích thước N — xác nhận O(1) trung bình.
- **B-Tree** tăng chậm theo logarit: từ ~7μs lên ~49μs — phản ánh O(log n).
- **HTree** ổn định hơn B-Tree: từ ~10μs lên ~24μs. Đặc biệt, tại N ≥ 10.000, **HTree nhanh hơn B-Tree** do fan-out cao hơn (index chỉ lưu hash 32-bit thay vì tên file dài).

### 4.3.2. Lookup Miss — Tra Cứu Entries Không Tồn Tại

| N | Linear (ns) | Hash (ns) | B-Tree (ns) | HTree (ns) |
|--:|------------:|----------:|------------:|-----------:|
| 100 | 34.506 | 13.308 | 8.174 | 29.255 |
| 1.000 | 231.836 | 1.539 | 1.293 | 26.748 |
| 10.000 | 2.459.599 | 884 | 949 | 21.778 |
| 100.000 | 14.102.214 | 661 | 564 | 20.746 |
| 1.000.000 | 173.427.321 | 2.363 | 757 | 80.908 |

**Nhận xét:**
- **Linear Search** phải duyệt toàn bộ N entries khi miss → thời gian gần gấp đôi lookup hit.
- **B-Tree** rất hiệu quả khi miss: chỉ cần đi đến leaf rồi kết luận không tồn tại (757ns tại N=1M).
- **Hash Table** miss nhanh: bucket rỗng hoặc chain ngắn → kết luận ngay.
- **HTree** miss chậm hơn B-Tree vì phải scan toàn bộ leaf block (tối đa 64 entries) rồi thêm block kế tiếp (xử lý hash collision).

### 4.3.3. Số Phép So Sánh (Comparisons)

| N | Linear | Hash | B-Tree | HTree |
|--:|-------:|-----:|-------:|------:|
| 100 | 51 | 1 | 25 | 27 |
| 1.000 | 516 | 1 | 38 | 28 |
| 10.000 | 4.995 | 1 | 74 | 32 |
| 100.000 | 50.077 | 1 | 88 | 34 |
| 1.000.000 | 494.913 | 1 | 100 | 37 |

**Nhận xét:**
- **Linear**: Trung bình N/2 comparisons — đúng lý thuyết O(n).
- **Hash Table**: Luôn **1 comparison** — hàm DJB2 phân bố đều, gần như không có đụng độ khi load factor ≤ 0.7.
- **B-Tree**: Tăng logarit, ~100 comparisons tại N=1M. Mỗi level cần quét tối đa 99 keys → tổng ~2-3 levels × ~35 comparisons/level.
- **HTree**: Chỉ **37 comparisons** tại N=1M — ít hơn B-Tree gần 3 lần. Gồm: ~15 comparisons (binary search dx_entries) + ~22 comparisons (linear scan leaf block).

---

## 4.4. Phân Tích Hiệu Năng Insert

| N | Linear (μs) | Hash (μs) | B-Tree (μs) | HTree (μs) |
|--:|------------:|----------:|------------:|-----------:|
| 100 | 3.422 | 14.661 | 16.839 | 15.589 |
| 1.000 | 3.376 | 6.524 | 55.995 | 35.917 |
| 10.000 | 37.173 | 35.882 | 438.881 | 267.082 |
| 100.000 | 214.380 | 419.614 | 3.563.859 | 1.444.162 |
| 1.000.000 | 5.249.186 | 10.360.651 | 82.816.878 | 23.007.689 |

**Nhận xét:**
- **Linear Search** insert nhanh nhất ở quy mô nhỏ (chỉ append cuối mảng O(1)).
- **B-Tree** insert chậm nhất do chi phí proactive split và duy trì cây cân bằng — đặc biệt tốn kém ở quy mô lớn (~83 giây cho 1M entries).
- **HTree** insert nhanh hơn B-Tree **~3.6 lần** tại N=1M (23s vs 83s) vì chỉ cần: hash → binary search → append vào leaf block, split đơn giản hơn split B-Tree.
- **Hash Table** insert tốn thời gian do chi phí malloc cho mỗi node + rehash khi load factor vượt ngưỡng.

---

## 4.5. Phân Tích Hiệu Năng Delete

| N | Linear (ns) | Hash (ns) | B-Tree (ns) | HTree (ns) |
|--:|------------:|----------:|------------:|-----------:|
| 100 | 47.817 | 32.262 | 92.752 | 37.958 |
| 1.000 | 195.185 | 4.139 | 67.612 | 8.478 |
| 10.000 | 1.967.506 | 2.301 | 58.963 | 16.420 |
| 100.000 | 14.408.591 | 1.319 | 35.469 | 12.976 |
| 1.000.000 | 178.826.904 | 1.970 | 63.094 | 40.988 |

**Nhận xét:**
- **Linear Search** delete rất chậm ở quy mô lớn (~179ms/delete tại N=1M) vì phải scan toàn bộ mảng.
- **Hash Table** delete nhanh nhất (~2μs) — hash trực tiếp tới bucket, xóa node O(1).
- **HTree** delete nhanh hơn B-Tree (~41μs vs ~63μs tại N=1M) vì chỉ cần swap-with-last trong leaf block, không cần borrow/merge phức tạp.
- **B-Tree** delete phải duy trì tính cân bằng: borrow từ sibling hoặc merge nodes → chi phí cao hơn.

---

## 4.6. Phân Tích Bộ Nhớ (Memory Usage)

| N | Linear (KB) | Hash (KB) | B-Tree (KB) | HTree (KB) |
|--:|------------:|----------:|------------:|-----------:|
| 100 | 33 | 29 | 79 | 37 |
| 1.000 | 264 | 282 | 448 | 384 |
| 10.000 | 4.224 | 2.784 | 3.660 | 3.668 |
| 100.000 | 33.792 | 28.612 | 38.728 | 36.884 |
| 1.000.000 | 270.336 | 282.009 | 386.418 | 371.004 |

**Memory Overhead so với Linear Search (tại N=1M):**

| Phương pháp | Memory (KB) | Overhead |
|-------------|------------:|----------|
| Linear Search | 270.336 | Baseline |
| Hash Table | 282.009 | **+4,3%** |
| B-Tree | 386.418 | **+42,9%** |
| HTree | 371.004 | **+37,2%** |

**Nhận xét:**
- **Linear Search** và **Hash Table** có memory usage gần tương đương. Hash Table chỉ tốn thêm ~4.3% cho mảng buckets và con trỏ linked list.
- **B-Tree** tốn nhiều nhất (+42.9%) vì phải cấp phát mảng keys (tối đa 99 × sizeof(DirEntry)) và mảng children (100 con trỏ) cho mỗi node, kể cả khi node chưa đầy.
- **HTree** tiết kiệm hơn B-Tree (~15KB ít hơn tại N=1M) vì leaf blocks có kích thước cố định và dx_entries chỉ lưu hash + con trỏ (8 bytes/entry thay vì toàn bộ DirEntry).

---

## 4.7. Phân Tích Chuyên Sâu: Tại Sao ext4 Chọn HTree?

Từ kết quả benchmark, có thể thấy **Hash Table nhanh nhất** trong môi trường RAM thuần. Tuy nhiên, trong thiết kế hệ điều hành thực tế, **Disk I/O** mới là nhân tố quyết định:

### 4.7.1. Hash Table — Nhanh nhưng không phù hợp cho Disk

- Mỗi node trong linked list được cấp phát **phân tán** trên heap → khi load từ đĩa, hệ thống phải đọc **nhiều block đĩa ngẫu nhiên** (random I/O).
- Random I/O trên HDD chậm gấp **100–1000 lần** so với sequential I/O.
- Không thể biểu diễn con trỏ (pointer) trên đĩa một cách hiệu quả — con trỏ chỉ có ý nghĩa trong RAM.
- Khi rehash, toàn bộ bảng băm phải được ghi lại → tốn I/O cực lớn.

### 4.7.2. B-Tree — Tốt nhưng chưa tối ưu

- B-Tree thân thiện với disk vì mỗi node tương ứng 1 block → chỉ cần O(log n) lần đọc đĩa.
- **Nhược điểm:** Mỗi key là **tên file** có độ dài thay đổi (1–255 bytes). Tên file dài chiếm nhiều không gian trong node → **giảm fan-out** (số con của mỗi node).
- Ví dụ: Nếu tên file trung bình 20 bytes + overhead → một block 4KB chỉ chứa ~100 keys. Nhưng nếu tên file dài 100 bytes → chỉ còn ~30 keys/block → **chiều cao cây tăng** → cần nhiều lần đọc đĩa hơn.

### 4.7.3. HTree — Giải Pháp Tối Ưu

HTree giải quyết triệt để các vấn đề trên:

1. **Index node chỉ lưu hash 32-bit (4 bytes)** — không lưu tên file. Một block 4KB chứa tới **~500 index entries** (4 bytes hash + 4 bytes pointer). Fan-out cực cao → **chiều cao cây luôn ≤ 2** kể cả với hàng triệu file.

2. **Chỉ cần 1–2 lần đọc đĩa** để tìm bất kỳ file nào:
   - Lần 1: Đọc root block (dx_entries) → binary search tìm leaf block.
   - Lần 2: Đọc leaf block → linear scan tìm entry chính xác.

3. **Leaf block tương thích cấu trúc ext2 cũ** → hỗ trợ tương thích ngược (backward compatibility) mà không cần chuyển đổi dữ liệu.

4. **Kết quả benchmark xác nhận:**
   - HTree chỉ cần **37 comparisons** (vs B-Tree 100, Linear 494.913) tại N=1M.
   - Lookup time HTree **24μs** — nhanh gấp 2× so với B-Tree (49μs), gấp 3.575× so với Linear (86ms).
   - Memory overhead chỉ +37.2% — tiết kiệm hơn B-Tree (+42.9%).

### 4.7.4. Bảng So Sánh Tổng Hợp Theo Tiêu Chí Filesystem

| Tiêu chí | Linear | Hash Table | B-Tree | **HTree** |
|----------|--------|-----------|--------|-----------|
| Lookup speed | ✗ Rất chậm | ✓ Nhanh nhất (RAM) | ✓ Nhanh | **✓ Rất nhanh** |
| Disk I/O friendly | ✓ Sequential | ✗ Random I/O | ✓ Block-aligned | **✓✓ Tối ưu nhất** |
| Fan-out (entries/block) | N/A | N/A | ~100 (phụ thuộc tên file) | **~500 (hash cố định 4B)** |
| Chiều cao tại 1M files | N/A | N/A | ~3 levels | **≤ 2 levels** |
| Disk reads cho 1 lookup | N blocks | Nhiều random | 3 blocks | **1–2 blocks** |
| Tương thích ngược | ✓ (ext2) | ✗ | ✗ | **✓ (leaf = ext2 block)** |
| Memory overhead | Baseline | +4.3% | +42.9% | **+37.2%** |

---

## 4.8. Kiểm Tra An Toàn Bộ Nhớ (Valgrind)

Toàn bộ hệ thống qua kiểm tra Valgrind với kết quả hoàn hảo:

```
==PID== HEAP SUMMARY:
==PID==     in use at exit: 0 bytes in 0 blocks
==PID==   total heap usage: 3,611,188 allocs, 3,611,188 frees
==PID==
==PID== All heap blocks were freed -- no leaks are possible
==PID==
==PID== ERROR SUMMARY: 0 errors from 0 contexts
```

- **0 bytes leaked** — tất cả bộ nhớ cấp phát đều được giải phóng.
- **0 errors** — không có truy cập bộ nhớ bất hợp lệ (buffer overflow, use-after-free, ...).
- **3.611.188 allocs = 3.611.188 frees** — mỗi `malloc`/`calloc` đều có `free` tương ứng.

---

## 4.9. Kết Luận Chương

Kết quả thực nghiệm trên 9 mức kích thước (100 → 1.000.000 entries) đã chứng minh rõ ràng:

1. **Linear Search** hoàn toàn không khả thi cho thư mục lớn — thời gian tra cứu tăng tuyến tính, đạt ~86ms/lookup tại N=1M.

2. **Hash Table** nhanh nhất trong RAM nhưng **không phù hợp cho filesystem** vì cấu trúc con trỏ phân tán gây random I/O nghiêm trọng trên đĩa.

3. **B-Tree** cải thiện đáng kể (~1.759× nhanh hơn Linear) và thân thiện với disk, nhưng bị hạn chế bởi fan-out thấp do phải lưu tên file trong index node.

4. **HTree là lựa chọn tối ưu nhất** cho filesystem: kết hợp tốc độ hash O(1) với cấu trúc cây tối ưu I/O, fan-out cực cao nhờ chỉ lưu hash 32-bit, giữ chiều cao cây ≤ 2 → chỉ cần 1–2 lần đọc đĩa cho mọi thao tác. Đây chính xác là lý do ext4 đã chọn HTree làm cấu trúc chỉ mục thư mục mặc định.
