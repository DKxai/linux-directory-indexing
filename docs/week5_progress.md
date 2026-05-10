# 📋 BÁO CÁO TIẾN ĐỘ TUẦN 5
## Dự án: Improving Directory Lookup Performance Using Indexing Structures
### Môn: Hệ Điều Hành (Operating Systems)

---

## 1. Summary of Latest Progress

Tuần 5 hoàn thành mục tiêu trọng tâm: triển khai **B-Tree** — cấu trúc cây cân bằng đa nhánh, được sử dụng rộng rãi trong database và filesystem để tối ưu truy cập theo block. B-Tree đạt hiệu năng lookup **O(log n)**, nằm giữa Linear Search O(n) và Hash Table O(1).

Các kết quả đạt được:
- ✅ Triển khai B-Tree với bậc tối thiểu **t = 50** (mỗi node chứa tối đa **99 keys**, phù hợp mô phỏng filesystem block 4KB).
- ✅ Cơ chế **Proactive Split**: tách node đầy trước khi đi xuống con, đảm bảo parent luôn có chỗ trống.
- ✅ Hoàn thiện 4 API cốt lõi: `btree_create`, `btree_insert`, `btree_lookup`, `btree_delete` — đồng bộ giao diện với Linear Search và Hash Table.
- ✅ Thuật toán **Delete** đầy đủ: borrow từ sibling trái/phải, merge nodes khi thiếu keys, xử lý predecessor/successor cho internal node.
- ✅ Tích hợp B-Tree vào Benchmark Framework, Interactive Demo, và Single Method Benchmark.
- ✅ **Memory Safety:** Valgrind xác nhận **0 errors, 0 memory leaks** trên hơn **3.5 triệu** lượt cấp phát bộ nhớ.
- ✅ Kết quả: B-Tree lookup chỉ cần **~100 comparisons** ở 1M entries (so với 494,913 của Linear Search).
- Tổng khối lượng mã nguồn mới: **~350 dòng** (`btree.c/h`), tổng dự án: **~1,600 dòng**.

---

## 2. Review Technical Activities

### 2.1 Cấu trúc B-Tree

B-Tree bậc t (minimum degree) là cây tìm kiếm cân bằng đa nhánh:
- Mỗi node chứa **t-1 đến 2t-1 keys** (trừ root có thể chứa 1 key).
- Mỗi internal node có **t đến 2t children**.
- Tất cả leaf nằm cùng một mức → cây luôn cân bằng.

Chọn **t = 50** vì:
- Mỗi node chứa tối đa **99 DirEntry** → phù hợp mô phỏng 1 disk block.
- Với 1M entries, chiều cao cây chỉ = **2** (root + 1 level of leaves hoặc root + internal + leaves).
- Số comparisons per lookup ≈ t × height ≈ 100.

```c
typedef struct BTreeNode {
    DirEntry*           keys;       // Mang keys (DirEntry), toi da 2t-1
    struct BTreeNode**  children;   // Mang con tro den cac con, toi da 2t
    int                 num_keys;   // So keys hien tai trong node
    bool                is_leaf;    // Co phai la node la (leaf) khong
} BTreeNode;

typedef struct {
    BTreeNode*  root;          // Con tro den node goc
    int         min_degree;    // Bac toi thieu t
    int         count;         // Tong so entries trong cay
} BTree;
```

### 2.2 Operations — Phân tích Complexity

| Operation | Complexity | Chi tiết kỹ thuật |
|-----------|-----------|-------------------|
| `btree_create()` | O(1) | Tạo root node rỗng (is_leaf = true), cấp phát arrays cho 2t-1 keys và 2t children |
| `btree_insert()` | O(log n) | Proactive split: nếu node đầy (2t-1 keys) thì split TRƯỚC khi đi xuống con |
| `btree_lookup()` | O(log n) | Tìm tuần tự trong keys[] → nếu không có thì đệ quy xuống con thích hợp |
| `btree_delete()` | O(log n) | 3 cases: leaf (xóa trực tiếp), internal (thay bằng predecessor/successor), merge/borrow |
| `btree_height()` | O(log n) | Duyệt từ root theo children[0] xuống leaf |
| `btree_destroy()` | O(n) | Post-order traversal, free từng node (keys[], children[], node) |

### 2.3 Cơ chế Insert — Proactive Split

Thay vì insert rồi xử lý tràn (bottom-up split), B-Tree dùng **proactive split**:
1. Khi đi từ root xuống, nếu gặp node con **đầy** (2t-1 keys), **split ngay** trước khi đi xuống.
2. Key giữa (median) được đẩy lên parent.
3. Node đầy chia thành 2 node, mỗi node chứa t-1 keys.
4. Đảm bảo khi insert vào leaf, leaf **luôn có chỗ trống** → không cần quay lại.

```c
// Proactive split: neu con day thi split truoc khi di xuong
if (node->children[i]->num_keys == 2 * t - 1) {
    split_child(node, i, t);
    if (strcmp(entry->name, node->keys[i].name) > 0) {
        i++;
    }
}
insert_nonfull(node->children[i], entry, t);
```

**Trường hợp đặc biệt**: khi root đầy, tạo root mới, root cũ trở thành con → chiều cao cây tăng 1.

### 2.4 Cơ chế Delete — Borrow & Merge

Delete trong B-Tree phức tạp nhất, với 3 trường hợp chính:

**Case 1: Key nằm trong node hiện tại**
- 1a: Nếu là leaf → xóa trực tiếp, dịch trái các key sau.
- 1b: Nếu là internal → thay bằng **predecessor** (key lớn nhất cây con trái) hoặc **successor** (key nhỏ nhất cây con phải), rồi đệ quy xóa bản gốc.
- 1c: Nếu cả 2 con chỉ có t-1 keys → **merge** 2 con và key trung gian, rồi xóa từ node merged.

**Case 2: Key không nằm trong node hiện tại**
- Xác định con thích hợp để đi xuống.
- Nếu con có < t keys → **fill** bằng cách borrow từ sibling hoặc merge.
- Đệ quy xuống con.

### 2.5 Tích hợp Benchmark

- Mở rộng `MethodType` enum: thêm `METHOD_BTREE`, `METHOD_COUNT` tăng lên 3.
- Viết `benchmark_btree()` đo cùng 5 metrics: Insert Time, Lookup Hit/Miss, Delete Time, Comparisons.
- Cập nhật `run_all_benchmarks()` để chạy đồng thời cả 3 phương pháp trên cùng bộ dữ liệu.
- Interactive Demo so sánh trực tiếp Linear vs Hash vs B-Tree trên cùng tập entries.
- Single Method Benchmark thêm option B-Tree.

---

## 3. Kết quả Benchmark Chi Tiết

### 3.1 Bảng so sánh Lookup Performance

| N | Linear (ns) | Hash (ns) | B-Tree (ns) | Linear Comp | Hash Comp | B-Tree Comp |
|---|---|---|---|---|---|---|
| 100 | 531 | 75 | 341 | 51 | 1 | 25 |
| 500 | 2,220 | 79 | 485 | 239 | 1 | 36 |
| 1,000 | 2,943 | 97 | 458 | 516 | 1 | 38 |
| 5,000 | 22,755 | 118 | 1,145 | 2,578 | 1 | 70 |
| 10,000 | 49,043 | 104 | 1,006 | 4,995 | 1 | 74 |
| 50,000 | 261,771 | 194 | 1,342 | 24,901 | 1 | 83 |
| 100,000 | 623,183 | 229 | 1,556 | 50,077 | 1 | 88 |
| 500,000 | 4,167,910 | 242 | 1,141 | 250,545 | 1 | 97 |
| 1,000,000 | 5,475,223 | 346 | 1,479 | 494,913 | 1 | 100 |

**Nhận xét:**
- B-Tree comparisons tăng rất chậm: **25 → 100** khi N tăng từ 100 → 1M (logarithmic growth).
- Hash Table vẫn nhanh nhất (O(1)), nhưng B-Tree nhanh hơn Linear Search **3,700 lần** ở 1M entries.
- B-Tree lookup time ổn định ở mức ~1,000-1,500 ns, không phụ thuộc nhiều vào N.

### 3.2 Bảng so sánh Delete Performance

| N | Linear (ns) | Hash (ns) | B-Tree (ns) | Linear Comp | Hash Comp | B-Tree Comp |
|---|---|---|---|---|---|---|
| 100 | 525 | 122 | 632 | 50 | 1 | 1 |
| 500 | 2,341 | 120 | 1,022 | 250 | 1 | 1 |
| 1,000 | 3,687 | 121 | 1,561 | 750 | 1 | 2 |
| 5,000 | 45,171 | 128 | 1,505 | 4,750 | 1 | 2 |
| 10,000 | 92,625 | 170 | 2,494 | 9,750 | 1 | 3 |
| 50,000 | 576,335 | 128 | 2,008 | 49,750 | 1 | 3 |
| 100,000 | 1,428,237 | 121 | 2,191 | 99,750 | 1 | 3 |
| 500,000 | 5,837,320 | 66 | 1,710 | 499,750 | 1 | 4 |
| 1,000,000 | 11,348,133 | 48 | 1,964 | 999,750 | 1 | 4 |

**Nhận xét:**
- B-Tree delete chỉ cần **3-4 comparisons** ở mức lớn — rất hiệu quả nhờ cấu trúc cây thấp.
- Delete time ổn định ~1,500-2,500 ns bất kể N.

### 3.3 So sánh Insert Time

| N | Linear (µs) | Hash (µs) | B-Tree (µs) |
|---|---|---|---|
| 1,000 | 282 | 250 | 1,692 |
| 10,000 | 5,429 | 2,174 | 25,014 |
| 100,000 | 55,369 | 42,463 | 293,419 |
| 1,000,000 | 141,302 | 354,555 | 1,911,695 |

**Nhận xét:**
- B-Tree insert chậm hơn Linear và Hash do: (1) phải duy trì thứ tự sorted, (2) cơ chế split tốn thời gian.
- Tuy nhiên, chi phí insert trả trước để đổi lấy lookup O(log n) nhanh hơn rất nhiều.

### 3.4 So sánh Bộ Nhớ

| N | Linear (KB) | Hash (KB) | B-Tree (KB) |
|---|---|---|---|
| 1,000 | 264 | 281 | 447 |
| 10,000 | 4,224 | 2,784 | 3,659 |
| 100,000 | 33,792 | 28,610 | 38,728 |
| 1,000,000 | 270,336 | 282,009 | 386,417 |

**Nhận xét:** B-Tree sử dụng nhiều bộ nhớ hơn do mỗi node cấp phát mảng keys[] và children[] với kích thước cố định (2t-1 keys, 2t pointers) dù không phải tất cả đều được sử dụng. Đây là trade-off điển hình: bộ nhớ đổi lấy tốc độ.

### 3.5 Valgrind Memory Safety

```text
== HEAP SUMMARY:
==     in use at exit: 0 bytes in 0 blocks
==   total heap usage: 3,522,231 allocs, 3,522,231 frees, 4,675,558,280 bytes allocated
==
== All heap blocks were freed -- no leaks are possible
== ERROR SUMMARY: 0 errors from 0 contexts
```

---

## 4. Personal Contributions

### Thành viên 1: [Tên thành viên 1]
- **Vai trò:** Core Developer / Algorithm Engineer
- **Các file phụ trách:** `btree.c/h`
- **Công việc chi tiết:**
  1. Nghiên cứu thuật toán B-Tree từ CLRS Chapter 18.
  2. Thiết kế cấu trúc `BTreeNode` và `BTree` phù hợp với DirEntry.
  3. Triển khai insert với proactive split — đảm bảo top-down insert không cần backtrack.
  4. Triển khai delete đầy đủ 3 cases: leaf, internal (predecessor/successor), merge/borrow.
  5. Viết các hàm tiện ích: `btree_height()`, `btree_stats()`, `btree_memory_usage()`.

### Thành viên 2: [Tên thành viên 2]
- **Vai trò:** Integration Engineer / Benchmark Analyst
- **Các file phụ trách:** `benchmark.c/h`, `main.c`, `Makefile`, `README.md`
- **Công việc chi tiết:**
  1. Mở rộng enum `MethodType` để hỗ trợ `METHOD_BTREE`.
  2. Viết hàm `benchmark_btree()` đo hiệu năng insert/lookup/delete.
  3. Cập nhật Interactive Demo để so sánh trực tiếp 3 phương pháp.
  4. Cập nhật Single Method Benchmark để chọn phương pháp B-Tree.
  5. Chạy Valgrind profiling cho toàn bộ bộ test, xác nhận memory safety.
  6. Thu thập và phân tích dữ liệu benchmark, cập nhật README và báo cáo.

*(** Lưu ý:** Hai bạn hãy đổi tên thành viên 1 và 2 bằng tên thật hoặc mã số sinh viên vào báo cáo chính thức nhé).*

---

## 5. Developed Code Review

### Ưu điểm nổi bật

1. **Logarithmic Performance thực tế**: Comparisons tăng từ 25 → 100 khi N tăng 10,000 lần (100 → 1M). Đúng O(log n) như lý thuyết.

2. **Proactive Split thông minh**: Insert top-down — không cần quay lại parent để split. Giảm độ phức tạp implementation và tránh lỗi khi backtrack.

3. **Delete đầy đủ**: Triển khai đầy đủ cả 3 trường hợp phức tạp nhất của B-Tree delete, bao gồm borrow từ sibling trái/phải và merge nodes.

4. **API nhất quán**: Cả 3 phương pháp (Linear, Hash, B-Tree) đều tuân theo giao diện: `create`, `insert`, `lookup`, `delete`, `memory_usage`, `destroy` — dễ dàng mở rộng cho HTree tuần sau.

5. **Memory Safety tuyệt đối**: Hơn 3.5 triệu lượt `malloc` + `free` (bao gồm cả node splitting/merging phức tạp) mà Valgrind báo 0 lỗi.

6. **Tree Statistics**: Hàm `btree_stats()` cho phép kiểm tra height, node count, memory — hữu ích cho debug và báo cáo.

### Những điểm cần tối ưu thêm

- **Binary Search trong node**: Hiện tại dùng linear scan trong node (O(t) per node). Có thể cải thiện thành binary search O(log t), nhưng với t=50 thì ảnh hưởng không lớn.
- **Memory efficiency**: Mỗi node cấp phát arrays cố định 2t-1 keys dù có thể chỉ dùng t-1. Có thể dùng dynamic array nhưng sẽ phức tạp hóa code.

---

## 6. Phân tích Kết Quả

### 6.1 So sánh 3 phương pháp

| Tiêu chí | Linear Search | Hash Table | B-Tree |
|----------|--------------|-----------|--------|
| Lookup | O(n) — chậm nhất | O(1) — nhanh nhất | O(log n) — trung bình |
| Insert | O(1) — nhanh nhất | O(1) avg | O(log n) — chậm nhất |
| Delete | O(n) | O(1) avg | O(log n) |
| Memory | Tốt | Tốt | Tốn hơn ~40% |
| Ordered? | Không | Không | **Có** — keys luôn sorted |
| Range query? | Không | Không | **Có** |

### 6.2 Tại sao cần B-Tree khi đã có Hash Table?

B-Tree có những ưu điểm mà Hash Table không có:
1. **Dữ liệu có thứ tự**: B-Tree giữ keys sorted → hỗ trợ range query, prefix search.
2. **Hiệu năng ổn định**: Worst case vẫn O(log n), trong khi Hash Table worst case O(n) nếu hash function kém.
3. **Disk-friendly**: Mỗi node tương ứng 1 disk block → giảm số I/O operations.
4. **Không cần rehash**: B-Tree tự cân bằng qua split/merge, không có chi phí rehash đột biến.

### 6.3 Ý nghĩa trong Filesystem

- **ext4 HTree** (tuần 6) kết hợp ưu điểm của cả Hash Table và B-Tree.
- **XFS, Btrfs** sử dụng B-Tree trực tiếp cho metadata indexing.
- **Database** (MySQL InnoDB, PostgreSQL) dùng B+Tree — biến thể của B-Tree.

---

## 7. TODO cho Tuần Tới (Tuần 6)

**Mục tiêu chính: Triển khai HTree (ext4 Directory Index)**

HTree là cấu trúc lai giữa Hash và B-Tree, được dùng trong ext4 filesystem:

Các Checklist đầu việc:
1. **HTree Struct**: Tạo struct `HTreeNode` mô phỏng ext4 dx_root/dx_node.
2. **Half-MD4 Hash**: Triển khai hash function giống ext4 (half_md4, tea hash).
3. **HTree Insert**: Insert vào leaf block, split khi block đầy.
4. **HTree Lookup**: Hash → dx_entry → leaf block → linear scan trong block.
5. **HTree Delete**: Xóa entry từ leaf block.
6. Tích hợp vào benchmark framework, so sánh đầy đủ 4 phương pháp.
7. Chạy Valgrind, cập nhật README và báo cáo.
