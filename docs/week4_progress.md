# 📋 BÁO CÁO TIẾN ĐỘ TUẦN 4
## Dự án: Improving Directory Lookup Performance Using Indexing Structures
### Môn: Hệ Điều Hành (Operating Systems)

---

## 1. Summary of Latest Progress

Tuần 4 hoàn thành mục tiêu trọng tâm: triển khai **Hash Table** — cấu trúc dữ liệu đầu tiên đạt hiệu năng lookup **O(1)** để so sánh với baseline Linear Search O(n) của Tuần 3. Hiện tại, dự án đã đạt **100%** các mục tiêu đề ra cho tuần này.

Các kết quả đạt được:
- ✅ Triển khai hàm băm **djb2** (Daniel J. Bernstein) — hash function phổ biến và hiệu quả cho chuỗi.
- ✅ Xây dựng Hash Table với cơ chế **Separate Chaining** (Singly Linked List) xử lý collision.
- ✅ Hoàn thiện 4 API cốt lõi: `hash_create`, `hash_insert`, `hash_lookup`, `hash_delete` — đồng bộ giao diện với Linear Search.
- ✅ Cơ chế **Auto-Rehash**: tự nhân đôi số buckets khi `load_factor > 0.7`, đảm bảo chiều dài chain luôn ngắn.
- ✅ Tích hợp Hash Table vào Benchmark Framework và Interactive Demo.
- ✅ **Memory Safety:** Valgrind xác nhận **0 errors, 0 memory leaks** trên hơn **3.3 triệu** lượt cấp phát bộ nhớ.
- ✅ Kết quả so sánh: Hash Table **nhanh hơn Linear Search lên đến 25,000 lần** ở quy mô 1 triệu entries.
- Tổng khối lượng mã nguồn mới: **~230 dòng** (`hash_table.c/h`), tổng dự án: **~1,200 dòng**.

---

## 2. Review Technical Activities

### 2.1 Thuật toán Hash — djb2

Hàm băm djb2 được chọn vì:
- **Đơn giản**: chỉ gồm phép nhân 33 và cộng ký tự.
- **Phân bố đều**: giảm thiểu collision cho chuỗi có độ dài thay đổi (tên file).
- **Hiệu suất cao**: chỉ cần 1 vòng lặp qua chuỗi, không cần phép chia phức tạp.

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

### 2.2 Cấu trúc dữ liệu Hash Table

- **Separate Chaining**: Mỗi bucket là một Singly Linked List chứa các `HashNode`.
- **HashNode** chứa trọn vẹn `DirEntry` (dữ liệu giống ext4) và con trỏ `next`.
- **HashTable** quản lý: mảng buckets, tổng số entries (`count`), kích thước bảng (`num_buckets`).

```c
typedef struct HashNode {
    DirEntry          entry;
    struct HashNode*  next;
} HashNode;

typedef struct {
    HashNode**  buckets;       // Mang con tro toi cac chain
    int         num_buckets;   // Kich thuoc mang buckets
    int         count;         // Tong so entries
} HashTable;
```

### 2.3 Operations — Phân tích Complexity

| Operation | Complexity | Chi tiết kỹ thuật |
|-----------|-----------|-------------------|
| `hash_create()` | O(1) | `calloc` 256 bucket pointers, tất cả khởi tạo `NULL` |
| `hash_insert()` | O(1) avg | Hash → index → prepend node đầu chain. Gọi rehash nếu cần |
| `hash_lookup()` | O(1) avg | Hash → index → duyệt chain (chiều dài trung bình ≈ load_factor) |
| `hash_delete()` | O(1) avg | Hash → index → duyệt chain → unlink + `free()` node |
| `hash_rehash()` | O(n) | Nhân đôi mảng, di chuyển từng node sang bucket mới (không cần `malloc` node mới) |
| `hash_destroy()` | O(n) | Duyệt từng bucket, `free` từng node, cuối cùng `free` mảng buckets |

### 2.4 Cơ chế Rehash

Rehash là điểm mấu chốt giữ hiệu năng O(1):
- **Ngưỡng kích hoạt**: khi `load_factor = count / num_buckets > 0.7`
- **Hành vi**: tạo mảng buckets mới gấp đôi, duyệt toàn bộ bảng cũ, di chuyển từng node sang vị trí mới (tính lại index bằng `djb2_hash(name) % new_size`)
- **Ưu điểm**: không cần `malloc`/`free` node — chỉ thay đổi con trỏ, tiết kiệm overhead
- **Kết quả**: từ 256 buckets ban đầu, sau khi insert 1M entries, bảng tự mở rộng đến ~1.5M buckets, giữ chiều dài chain trung bình ≈ 0.67

```c
// Kiem tra load factor truoc khi insert
double lf = (double)(ht->count + 1) / (double)ht->num_buckets;
if (lf > HASH_LOAD_FACTOR_THRESHOLD) {
    hash_rehash(ht);  // Tu dong nhan doi so buckets
}
```

### 2.5 Tích hợp Benchmark

- Mở rộng `MethodType` enum: thêm `METHOD_HASH`.
- Viết `benchmark_hash()` đo cùng 5 metrics: Insert Time, Lookup Hit/Miss, Delete Time, Comparisons.
- Cập nhật `run_all_benchmarks()` để chạy song song cả Linear Search và Hash Table trên cùng bộ dữ liệu.
- Interactive Demo so sánh trực tiếp cả 2 phương pháp trên cùng tập entries.

---

## 3. Kết quả Benchmark Chi Tiết

### 3.1 Bảng so sánh Lookup Performance

| N | Linear Lookup (ns) | Hash Lookup (ns) | Tốc độ gấp | Linear Comp | Hash Comp |
|---|---|---|---|---|---|
| 100 | 205 | 24 | **8.5×** | 51 | 1 |
| 500 | 757 | 22 | **34.4×** | 239 | 1 |
| 1,000 | 1,369 | 23 | **59.5×** | 516 | 1 |
| 5,000 | 6,714 | 31 | **216.6×** | 2,578 | 1 |
| 10,000 | 13,841 | 46 | **300.9×** | 4,995 | 1 |
| 50,000 | 82,500 | 54 | **1,527.8×** | 24,901 | 1 |
| 100,000 | 205,280 | 120 | **1,710.7×** | 50,077 | 1 |
| 500,000 | 2,590,208 | 178 | **14,552.3×** | 250,545 | 1 |
| 1,000,000 | 5,010,810 | 199 | **25,179.4×** | 494,913 | 1 |

### 3.2 Bảng so sánh Delete Performance

| N | Linear Delete (ns) | Hash Delete (ns) | Tốc độ gấp | Linear Del Comp | Hash Del Comp |
|---|---|---|---|---|---|
| 100 | 156 | 49 | **3.2×** | 50 | 1 |
| 1,000 | 1,970 | 53 | **37.2×** | 750 | 1 |
| 10,000 | 29,113 | 35 | **831.8×** | 9,750 | 1 |
| 100,000 | 538,468 | 37 | **14,553.2×** | 99,750 | 1 |
| 1,000,000 | 11,146,644 | 39 | **285,812×** | 999,750 | 1 |

### 3.3 So sánh Bộ Nhớ

| N | Linear Memory (KB) | Hash Memory (KB) | Tỷ lệ |
|---|---|---|---|
| 100 | 33 | 28 | Hash nhỏ hơn |
| 1,000 | 264 | 281 | Tương đương |
| 10,000 | 4,224 | 2,784 | Hash nhỏ hơn |
| 100,000 | 33,792 | 28,610 | Hash nhỏ hơn |
| 1,000,000 | 270,336 | 282,009 | Tương đương |

**Nhận xét bộ nhớ**: Hash Table sử dụng bộ nhớ tương đương hoặc thậm chí ít hơn Linear Search ở nhiều mức N. Mặc dù mỗi `HashNode` có thêm con trỏ `next` (8 bytes), nhưng Linear Search phải cấp phát mảng theo `capacity` (luôn là lũy thừa 2), dẫn đến lãng phí khoảng trống.

### 3.4 Valgrind Memory Safety

```text
== HEAP SUMMARY:
==     in use at exit: 0 bytes in 0 blocks
==   total heap usage: 3,361,582 allocs, 3,361,582 frees, 3,357,513,367 bytes allocated
==
== All heap blocks were freed -- no leaks are possible
== ERROR SUMMARY: 0 errors from 0 contexts
```

---

## 4. Personal Contributions

### Thành viên 1: [Tên thành viên 1]
- **Vai trò:** Core Developer / Algorithm Engineer
- **Các file phụ trách:** `hash_table.c/h`, `common.h`
- **Công việc chi tiết:**
  1. Nghiên cứu và triển khai hàm băm djb2 cho filename hashing.
  2. Thiết kế cấu trúc `HashNode` và `HashTable` với Separate Chaining.
  3. Xây dựng thuật toán Rehash tự động khi load_factor > 0.7.
  4. Triển khai các hàm `hash_insert`, `hash_lookup`, `hash_delete` với đếm comparisons.
  5. Viết hàm `hash_stats()` để phân tích phân bố buckets (debug utility).

### Thành viên 2: [Tên thành viên 2]
- **Vai trò:** Integration Engineer / Benchmark Analyst
- **Các file phụ trách:** `benchmark.c/h`, `main.c`, `Makefile`, `README.md`
- **Công việc chi tiết:**
  1. Mở rộng enum `MethodType` để hỗ trợ `METHOD_HASH`.
  2. Viết hàm `benchmark_hash()` đo hiệu năng insert/lookup/delete.
  3. Cập nhật Interactive Demo để so sánh trực tiếp Linear vs Hash.
  4. Cập nhật Single Method Benchmark để chọn phương pháp Hash Table.
  5. Chạy Valgrind profiling cho toàn bộ bộ test, xác nhận memory safety.
  6. Thu thập và phân tích dữ liệu benchmark, cập nhật README và báo cáo.

*(** Lưu ý:** Hai bạn hãy đổi tên thành viên 1 và 2 bằng tên thật hoặc mã số sinh viên vào báo cáo chính thức nhé).*

---

## 5. Developed Code Review

### Ưu điểm nổi bật

1. **Hiệu năng vượt trội**: Hash Table đạt O(1) với chỉ 1 comparison — nhanh hơn Linear Search lên đến **25,000 lần** ở 1M entries. Đây là minh chứng thực tế rõ ràng nhất cho lý thuyết.

2. **Cơ chế Rehash thông minh**: Không cần `malloc`/`free` node khi rehash — chỉ di chuyển con trỏ. Giảm overhead đáng kể so với cách tạo lại toàn bộ bảng.

3. **API nhất quán**: Tất cả các phương pháp (Linear, Hash) đều tuân theo cùng giao diện: `create`, `insert`, `lookup`, `delete`, `memory_usage`, `destroy` — dễ dàng mở rộng cho B-Tree và HTree tuần sau.

4. **Memory Safety tuyệt đối**: Hơn 3.3 triệu lượt `malloc` + `free` mà Valgrind báo 0 lỗi. Quản lý Linked List (`HashNode`) với con trỏ `next` phức tạp hơn mảng thuần nhưng vẫn đảm bảo zero-leak.

5. **Hash Stats Utility**: Hàm `hash_stats()` cho phép kiểm tra load factor, max chain length, phần trăm empty buckets — hữu ích cho việc debug và tối ưu.

### Những điểm cần tối ưu thêm

- **Duplicate key detection**: Hiện tại `hash_insert()` không kiểm tra entry trùng tên. Trong filesystem thực tế, tên file trong cùng thư mục phải duy nhất. Có thể bổ sung tùy chọn check duplicate.
- **Shrink khi xóa nhiều**: Khi `load_factor` giảm xuống quá thấp (< 0.2) sau nhiều lần `delete`, có thể cân nhắc thu nhỏ bảng để tiết kiệm bộ nhớ.

---

## 6. Phân tích Kết Quả

### 6.1 Tại sao Hash Table nhanh hơn?

- **Linear Search** phải quét tuần tự mảng: mỗi lookup cần trung bình N/2 comparisons.
- **Hash Table** tính index trực tiếp từ tên file: chỉ cần duyệt chain ngắn tại 1 bucket.
- Với `load_factor ≤ 0.7`, trung bình mỗi bucket chỉ chứa 0.7 entries → gần như luôn tìm thấy ngay.

### 6.2 Trade-off: Insert Time

Ở N lớn (500K - 1M), Hash Table có Insert Time **cao hơn** Linear Search:
- **Linear**: Insert O(1) amortized — chỉ append cuối mảng.
- **Hash**: Insert kèm theo nhiều lần rehash O(n) khi bảng đầy → tổng thời gian insert tích lũy cao hơn.
- Tuy nhiên, đây là chi phí trả trước một lần để đổi lấy lookup O(1) suốt vòng đời bảng.

### 6.3 Ý nghĩa thực tế trong Filesystem

Trong Linux ext4:
- Thư mục chứa **hàng ngàn files** → Linear Search trở nên rất chậm.
- Hash-based indexing (như HTree) giúp `open()`, `stat()`, `unlink()` hoạt động nhanh hằng số.
- Kết quả benchmark này chứng minh rõ tầm quan trọng của indexing trong OS.

---

## 7. TODO cho Tuần Tới (Tuần 5)

**Mục tiêu chính: Triển khai B-Tree**

B-Tree là cấu trúc cây cân bằng dùng trong cơ sở dữ liệu và nhiều filesystem:

Các Checklist đầu việc:
1. **B-Tree Struct**: Tạo struct `BTreeNode` với mảng keys, mảng children, và thuộc tính `is_leaf`.
2. **B-Tree Create**: Khởi tạo cây với bậc tối thiểu `t` (minimum degree).
3. **B-Tree Insert**: Triển khai insert với cơ chế **split** khi node đầy (2t-1 keys).
4. **B-Tree Lookup**: Tìm kiếm theo cơ chế nhị phân trong từng node, rồi đệ quy xuống con thích hợp.
5. **B-Tree Delete**: Xóa entry với xử lý các trường hợp: merge, borrow từ sibling.
6. Tích hợp vào benchmark framework và menu CLI. Chạy Valgrind.
