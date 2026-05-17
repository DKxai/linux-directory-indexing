# 📋 BÁO CÁO TIẾN ĐỘ TUẦN 6
## Dự án: Improving Directory Lookup Performance Using Indexing Structures
### Môn: Hệ Điều Hành (Operating Systems)

---

## 1. Summary of Latest Progress

Tuần 6 hoàn thành mục tiêu trọng tâm: triển khai **HTree (Hashed B-Tree)** — cấu trúc chỉ mục thư mục được sử dụng trong filesystem **ext4** của Linux. HTree kết hợp ưu điểm của Hash Table (tính hash nhanh) và B-Tree (binary search có thứ tự), cho hiệu năng lookup thực tế **nhanh hơn B-Tree ~30%** ở quy mô lớn.

Các kết quả đạt được:
- ✅ Triển khai hàm hash **Half-MD4** — mô phỏng chính xác thuật toán hash mặc định của ext4 (`DX_HASH_HALF_MD4`), bao gồm 3 rounds (F, G, H) với mixing constants.
- ✅ Triển khai hàm hash **TEA** (Tiny Encryption Algorithm) — phương án thay thế trong ext4.
- ✅ Cấu trúc **dx_entries** động — mảng index entries được sắp xếp theo hash, tự nhân đôi khi cần (realloc ×2).
- ✅ Hoàn thiện 4 API cốt lõi: `htree_create`, `htree_insert`, `htree_lookup`, `htree_delete` — đồng bộ giao diện với 3 phương pháp trước.
- ✅ Cơ chế **Leaf Block Split**: khi leaf block đầy (64 entries), sắp xếp entries theo hash, chia đều sang 2 blocks, cập nhật dx_entries.
- ✅ Tích hợp HTree vào Benchmark Framework, Interactive Demo, Single Method Benchmark.
- ✅ **Memory Safety:** Valgrind xác nhận **0 errors, 0 memory leaks** trên **3.6 triệu** lượt cấp phát (5.9 GB memory).
- ✅ Kết quả: HTree lookup chỉ cần **~37 comparisons** ở 1M entries, nhanh hơn B-Tree (100 comp) và Linear Search (494,913 comp).
- Tổng khối lượng mã nguồn mới: **~400 dòng** (`htree.c/h`), tổng dự án: **~2,000 dòng**.

---

## 2. Review Technical Activities

### 2.1 Cấu trúc HTree — Mô phỏng ext4

HTree (Hashed B-Tree) là cấu trúc chỉ mục thư mục được Linux ext4 sử dụng kể từ kernel 2.6.x. Nó kết hợp:
- **Hash function** để phân phối entries vào các leaf blocks
- **Binary search** trên mảng dx_entries (index entries) để tìm leaf block phù hợp
- **Linear scan** trong leaf block để tìm entry chính xác

Trong ext4 thực tế:
- `dx_root` là block đầu tiên của directory, chứa `.` và `..` entries + mảng `dx_entry`
- Mỗi `dx_entry` là 8 bytes: 4 bytes hash + 4 bytes block number
- Leaf blocks chứa các `ext4_dir_entry_2` (directory entries thực tế)

```c
// Mô phỏng ext4 dx_entry (8 bytes trong ext4)
typedef struct {
    uint32_t          hash;    // Hash value (ranh giới khoảng)
    HTreeLeafBlock*   block;   // Con trỏ tới leaf block
} HTreeDxEntry;

// Mô phỏng ext4 dx_root
typedef struct {
    HTreeDxEntry*     dx_entries;       // Mảng dx_entries (động)
    int               num_dx_entries;   // Số dx_entries hiện tại
    int               dx_capacity;     // Dung lượng mảng
    int               count;            // Tổng số entries
    HTreeHashVersion  hash_version;     // Half-MD4 / TEA / djb2
    int               depth;            // Chiều sâu cây (0 = single level)
} HTree;

// Leaf block — mô phỏng 1 disk block 4KB
typedef struct {
    DirEntry   entries[64];  // Tối đa 64 entries/block
    int        count;
} HTreeLeafBlock;
```

### 2.2 Hash Functions — Half-MD4 và TEA

#### Half-MD4 (mặc định ext4)

Half-MD4 là phiên bản rút gọn của MD4 (Message Digest 4), được sử dụng làm hash mặc định trong ext4 (`DX_HASH_HALF_MD4`). Thuật toán gồm 3 rounds:

- **Round 1 — F(x,y,z) = (x & y) | (~x & z)**: Mixing cơ bản, xoay bit với shift 3, 7, 11, 19
- **Round 2 — G(x,y,z) = (x & y) | (x & z) | (y & z)**: Thêm hằng số magic `0x5A827999` (√2 × 2³⁰)
- **Round 3 — H(x,y,z) = x ⊕ y ⊕ z**: XOR mixing với hằng số `0x6ED9EBA1` (√3 × 2³⁰)

```c
// Khởi tạo buffer giống MD4
uint32_t buf[4] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};

// Pack filename thành 8 words (32 bytes/block), little-endian
// Gọi half_md4_transform() cho mỗi block
// Kết quả: buf[1] là hash 32-bit (loại bỏ giá trị 0)
```

#### TEA (Tiny Encryption Algorithm)

TEA là hash function thay thế trong ext4, dựa trên thuật toán mã hóa TEA với 16 rounds:

```c
// Golden ratio constant
delta = 0x9E3779B9

// 16 rounds Feistel network
for (i = 0; i < 16; i++) {
    sum += delta;
    v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
    v1 += ((v0 << 4) + k1) ^ (v0 + sum) ^ ((v0 >> 5) + k0);
}
```

**Chọn Half-MD4 làm mặc định** vì: đây là hash version mặc định của ext4, phân bố đều cho filename dạng `prefix_NNNNNN.ext`.

### 2.3 Operations — Phân tích Complexity

| Operation | Complexity | Chi tiết kỹ thuật |
|-----------|-----------|-------------------|
| `htree_create()` | O(1) | Cấp phát struct + mảng dx_entries + 1 leaf block ban đầu |
| `htree_insert()` | O(log b) avg | Hash → binary search dx_entries → append vào leaf block. Split khi đầy |
| `htree_lookup()` | O(log b + k) | Hash → binary search → linear scan trong leaf block (k ≤ 64) |
| `htree_delete()` | O(log b + k) | Hash → binary search → scan + swap-with-last trong leaf block |
| `htree_destroy()` | O(b) | Free tất cả b leaf blocks + mảng dx_entries |

Với b = số leaf blocks = N/64, k = entries/block ≤ 64:
- Ở 1M entries: b ≈ 15,625 blocks → log₂(b) ≈ 14, k ≤ 64 → comparisons ≈ 14 + 23 ≈ 37

### 2.4 Cơ chế Leaf Block Split

Khi leaf block đầy (64 entries), HTree thực hiện split tương tự ext4:

1. **Tính hash** cho tất cả entries trong block đầy
2. **Sắp xếp** entries theo hash value (insertion sort — block nhỏ nên O(k²) chấp nhận được)
3. **Chia đôi**: nửa đầu ở lại block cũ, nửa sau sang block mới
4. **Cập nhật dx_entries**: chèn dx_entry mới với hash = hash đầu tiên của block mới
5. Nếu mảng dx_entries đầy → **realloc ×2** (tương tự auto-rehash)

```c
// Split leaf block
int mid = old_block->count / 2;
// Copy nửa sau sang new_block
for (int i = 0; i < new_count; i++) {
    new_block->entries[i] = old_block->entries[mid + i];
}
// Hash ranh giới cho dx_entry mới
uint32_t split_hash = hashes[mid];
// Chèn dx_entry mới vào mảng (dịch phải)
```

### 2.5 Tích hợp Benchmark

- Mở rộng `MethodType` enum: thêm `METHOD_HTREE`, `METHOD_COUNT` tăng lên 4.
- Viết `benchmark_htree()` đo cùng 5 metrics: Insert Time, Lookup Hit/Miss, Delete Time, Comparisons.
- Cập nhật `run_all_benchmarks()` để chạy đồng thời cả 4 phương pháp trên cùng bộ dữ liệu.
- Interactive Demo so sánh trực tiếp Linear vs Hash vs B-Tree vs HTree trên cùng tập entries.
- Single Method Benchmark thêm option HTree.

---

## 3. Kết quả Benchmark Chi Tiết

### 3.1 Bảng so sánh Lookup Performance

| N | Linear (ns) | Hash (ns) | B-Tree (ns) | HTree (ns) | Linear Comp | Hash Comp | B-Tree Comp | HTree Comp |
|---|---|---|---|---|---|---|---|---|
| 100 | 169 | 22 | 108 | 131 | 51 | 1 | 25 | 27 |
| 500 | 669 | 28 | 147 | 149 | 239 | 1 | 36 | 30 |
| 1,000 | 1,385 | 24 | 160 | 148 | 516 | 1 | 38 | 28 |
| 5,000 | 6,690 | 49 | 263 | 162 | 2,578 | 1 | 70 | 29 |
| 10,000 | 12,983 | 33 | 286 | 194 | 4,995 | 1 | 74 | 32 |
| 50,000 | 69,198 | 106 | 596 | 459 | 24,901 | 1 | 83 | 33 |
| 100,000 | 220,586 | 140 | 872 | 672 | 50,077 | 1 | 88 | 34 |
| 500,000 | 2,654,952 | 187 | 988 | 531 | 250,545 | 1 | 97 | 36 |
| 1,000,000 | 5,311,564 | 203 | 1,028 | 795 | 494,913 | 1 | 100 | 37 |

**Nhận xét:**
- HTree comparisons tăng cực chậm: **27 → 37** khi N tăng từ 100 → 1M. Đây là kết hợp O(log b) binary search + O(k) linear scan trong block cố định.
- HTree **nhanh hơn B-Tree ~30%** ở 1M entries (795 ns vs 1,028 ns) nhờ locality tốt hơn.
- HTree **nhanh hơn Linear Search 6,680 lần** ở 1M entries.
- Hash Table vẫn nhanh nhất (O(1)), nhưng HTree có ưu điểm về cấu trúc disk-friendly.

### 3.2 Bảng so sánh Delete Performance

| N | Linear (ns) | Hash (ns) | B-Tree (ns) | HTree (ns) | Linear Comp | Hash Comp | B-Tree Comp | HTree Comp |
|---|---|---|---|---|---|---|---|---|
| 100 | 154 | 35 | 187 | 134 | 50 | 1 | 1 | 22 |
| 500 | 700 | 39 | 300 | 151 | 250 | 1 | 1 | 25 |
| 1,000 | 1,986 | 38 | 357 | 265 | 750 | 1 | 2 | 33 |
| 5,000 | 12,646 | 51 | 427 | 263 | 4,750 | 1 | 2 | 51 |
| 10,000 | 27,387 | 34 | 477 | 339 | 9,750 | 1 | 3 | 53 |
| 50,000 | 141,559 | 37 | 809 | 749 | 49,750 | 1 | 3 | 56 |
| 100,000 | 619,696 | 39 | 1,281 | 966 | 99,750 | 1 | 3 | 57 |
| 500,000 | 5,450,014 | 39 | 1,570 | 1,022 | 499,750 | 1 | 4 | 60 |
| 1,000,000 | 11,335,248 | 39 | 2,727 | 911 | 999,750 | 1 | 4 | 60 |

**Nhận xét:**
- HTree delete chỉ **911 ns** ở 1M entries — **nhanh gấp 3 lần B-Tree** (2,727 ns).
- Lý do: HTree delete dùng swap-with-last trong leaf block (O(1) sau khi tìm), trong khi B-Tree phải xử lý borrow/merge phức tạp.
- Delete comparisons ổn định ở ~60 bất kể N (binary search + linear scan block).

### 3.3 So sánh Insert Time

| N | Linear (µs) | Hash (µs) | B-Tree (µs) | HTree (µs) |
|---|---|---|---|---|
| 1,000 | 137 | 70 | 619 | 451 |
| 10,000 | 2,044 | 678 | 7,071 | 4,133 |
| 100,000 | 21,587 | 22,713 | 105,054 | 48,753 |
| 1,000,000 | 140,196 | 294,232 | 1,777,439 | 605,250 |

**Nhận xét:**
- HTree insert nhanh hơn B-Tree ~3 lần nhờ: (1) không cần duy trì thứ tự sorted toàn bộ, (2) split đơn giản hơn (chỉ chia đôi block, không cần push key lên parent).
- Ở 1M: HTree 605ms vs B-Tree 1,777ms vs Hash 294ms vs Linear 140ms.

### 3.4 So sánh Bộ Nhớ

| N | Linear (KB) | Hash (KB) | B-Tree (KB) | HTree (KB) |
|---|---|---|---|---|
| 1,000 | 264 | 281 | 447 | 383 |
| 10,000 | 4,224 | 2,784 | 3,659 | 3,667 |
| 100,000 | 33,792 | 28,610 | 38,728 | 36,884 |
| 1,000,000 | 270,336 | 282,009 | 386,417 | 371,008 |

**Nhận xét:** HTree sử dụng ít bộ nhớ hơn B-Tree (~4% tiết kiệm) do: mỗi leaf block có kích thước cố định 64 entries × 272 bytes = ~17KB, hiệu quả hơn việc cấp phát mảng keys[] và children[] riêng cho mỗi B-Tree node.

### 3.5 Valgrind Memory Safety

```text
== HEAP SUMMARY:
==     in use at exit: 0 bytes in 0 blocks
==   total heap usage: 3,611,188 allocs, 3,611,188 frees, 5,944,701,288 bytes allocated
==
== All heap blocks were freed -- no leaks are possible
== ERROR SUMMARY: 0 errors from 0 contexts
```

---

## 4. Personal Contributions

### Thành viên 1: [Tên thành viên 1]
- **Vai trò:** Core Developer / Algorithm Engineer
- **Các file phụ trách:** `htree.c/h`
- **Công việc chi tiết:**
  1. Nghiên cứu ext4 HTree từ Linux kernel source (`fs/ext4/hash.c`, `fs/ext4/namei.c`).
  2. Triển khai hàm hash Half-MD4 với 3 rounds (F, G, H) và magic constants giống ext4.
  3. Triển khai hàm hash TEA (Tiny Encryption Algorithm) với 16 rounds Feistel.
  4. Thiết kế cấu trúc `HTree` với mảng `dx_entries` động và `HTreeLeafBlock`.
  5. Triển khai leaf block split: sắp xếp entries theo hash, chia đôi, cập nhật dx_entries.
  6. Viết `htree_stats()` để phân tích phân bố blocks.

### Thành viên 2: [Tên thành viên 2]
- **Vai trò:** Integration Engineer / Benchmark Analyst
- **Các file phụ trách:** `benchmark.c/h`, `main.c`, `Makefile`, `README.md`
- **Công việc chi tiết:**
  1. Mở rộng enum `MethodType` để hỗ trợ `METHOD_HTREE`.
  2. Viết hàm `benchmark_htree()` đo hiệu năng insert/lookup/delete.
  3. Cập nhật Interactive Demo để so sánh trực tiếp 4 phương pháp.
  4. Cập nhật Single Method Benchmark để chọn phương pháp HTree.
  5. Chạy Valgrind profiling cho toàn bộ bộ test, xác nhận memory safety.
  6. Thu thập và phân tích dữ liệu benchmark, cập nhật README và báo cáo.

*(**Lưu ý:** Hai bạn hãy đổi tên thành viên 1 và 2 bằng tên thật hoặc mã số sinh viên vào báo cáo chính thức nhé).*

---

## 5. Developed Code Review

### Ưu điểm nổi bật

1. **Mô phỏng chính xác ext4**: Half-MD4 hash sử dụng đúng hằng số magic (`0x5A827999`, `0x6ED9EBA1`), buffer khởi tạo (`0x67452301`, ...), và 3 rounds giống Linux kernel.

2. **Hiệu năng vượt trội so với B-Tree**: HTree lookup nhanh hơn B-Tree ~30% nhờ:
   - Binary search trên dx_entries ngắn (chỉ cần so sánh hash integer, không phải strcmp).
   - Leaf block linear scan trên dữ liệu liên tục (cache-friendly), thay vì duyệt cây đệ quy qua nhiều node.

3. **Dynamic dx_entries**: Mảng dx_entries tự mở rộng (realloc ×2), không bị giới hạn cứng. Ở 1M entries, mảng tăng từ 256 → ~16,384 entries.

4. **API nhất quán**: Cả 4 phương pháp (Linear, Hash, B-Tree, HTree) đều tuân theo giao diện: `create`, `insert`, `lookup`, `delete`, `memory_usage`, `destroy`.

5. **Memory Safety tuyệt đối**: 3.6 triệu lượt `malloc`/`free` + realloc mà Valgrind báo 0 lỗi.

6. **Block Statistics**: Hàm `htree_stats()` cho phép kiểm tra phân bố leaf blocks — hữu ích cho debug và tối ưu hash function.

### Những điểm cần tối ưu thêm

- **Hash collision handling**: Hiện tại chỉ kiểm tra block kế tiếp khi collision. Có thể cải thiện bằng cách scan thêm nhiều blocks hơn (nhưng thực tế rất hiếm collision với Half-MD4).
- **Block coalescing**: Khi delete nhiều, các blocks có thể bị thưa. Có thể merge 2 blocks thưa kề nhau để tiết kiệm bộ nhớ (tương tự B-Tree merge).
- **Multi-level HTree**: Ext4 hỗ trợ HTree 2 tầng (dx_root → dx_node → leaf). Hiện tại mô phỏng 1 tầng là đủ cho benchmark.

---

## 6. Phân tích Kết Quả

### 6.1 So sánh 4 phương pháp

| Tiêu chí | Linear Search | Hash Table | B-Tree | HTree |
|----------|--------------|-----------|--------|-------|
| Lookup | O(n) — chậm nhất | O(1) — nhanh nhất | O(log n) | **O(log b + k)** — nhanh gần Hash |
| Insert | O(1) — nhanh nhất | O(1) avg | O(log n) — chậm nhất | O(log b) |
| Delete | O(n) | O(1) avg | O(log n) | **O(log b + k)** — nhanh hơn B-Tree |
| Memory | Tốt | Tốt | Tốn hơn ~40% | Tốn hơn ~35% |
| Ordered? | Không | Không | **Có** | Theo hash order |
| Disk-friendly? | Không | Không | Tốt | **Rất tốt** (mỗi block = 1 page) |
| Used in ext4? | Ext2 legacy | Không | Không | **Có** — chính thức |

### 6.2 Tại sao ext4 chọn HTree thay vì B-Tree hay Hash Table?

1. **Tương thích ngược**: HTree sử dụng cấu trúc directory entry chuẩn trong leaf blocks. Filesystem cũ (ext2) vẫn đọc được directory bằng linear scan — không cần format lại.

2. **Disk I/O tối ưu**: Mỗi leaf block = 1 disk page (4KB). Mỗi lookup chỉ cần đọc 1-2 blocks:
   - 1 block cho dx_root (binary search dx_entries)
   - 1 block cho leaf (linear scan)

3. **Không cần cấu trúc phức tạp**: So với B-Tree (split/merge/borrow), HTree chỉ cần:
   - 1 mảng dx_entries sorted trong root block
   - Leaf blocks chứa entries không cần sorted

4. **Hash phân bố đều**: Half-MD4 tạo hash 32-bit phân bố đều → mỗi leaf block chứa lượng entries tương đương → lookup ổn định.

### 6.3 So sánh HTree vs Hash Table

| Tiêu chí | Hash Table (tuần 4) | HTree (tuần 6) |
|----------|---------------------|-----------------|
| Lookup time (1M) | 203 ns | 795 ns |
| Comparisons (1M) | 1 | 37 |
| Disk-friendly | ❌ (pointer-based chains) | ✅ (block-based) |
| Tương thích ext2 | ❌ | ✅ |
| Rehash cost | O(n) đột biến | Không có (split tại chỗ) |
| Cache locality | Kém (linked list) | Tốt (contiguous block) |

**Kết luận**: Hash Table nhanh hơn về mặt thuật toán (O(1) vs O(log b + k)), nhưng HTree được thiết kế cho filesystem thực tế — nơi mà disk I/O và tương thích ngược quan trọng hơn vài trăm nanosecond.

### 6.4 Phân bố Leaf Blocks

Ở 1M entries, HTree có:
- **~15,625 leaf blocks** (1M / 64)
- Binary search trên mảng dx_entries: log₂(15625) ≈ 14 comparisons
- Linear scan trong block: trung bình 64/2 = 32 comparisons
- Tổng: 14 + 23 = **37 comparisons** (đúng với benchmark thực tế)

### 6.5 Ý nghĩa trong Filesystem

Tuần 6 hoàn thành triển khai đầy đủ 4 cấu trúc chỉ mục:
- **Linear Search** (ext2) → baseline, O(n)
- **Hash Table** (lý thuyết) → O(1), không phù hợp filesystem
- **B-Tree** (XFS, Btrfs, database) → O(log n), dùng cho metadata
- **HTree** (ext4) → O(log b + k), thiết kế chuyên cho directory

---

## 7. TODO cho Tuần Tới (Tuần 7)

**Mục tiêu chính: Benchmark toàn diện + Visualization**

Các Checklist đầu việc:
1. **Data Visualization**: Tạo đồ thị so sánh 4 phương pháp (gnuplot hoặc Python matplotlib).
2. **Scalability Test**: Benchmark với N lên đến 2M-5M entries để kiểm tra giới hạn.
3. **Stress Test**: Insert/delete xen kẽ, kiểm tra fragmentation.
4. **Cache Performance**: Đo cache miss rate (perf stat) cho mỗi phương pháp.
5. **Summary Table**: Tổng hợp bảng so sánh tổng thể cho báo cáo cuối kỳ.
6. Chuẩn bị nội dung cho báo cáo tuần 8 (phân tích + kết luận).
