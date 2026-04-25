# 📋 BÁO CÁO TIẾN ĐỘ TUẦN 3
## Dự án: Improving Directory Lookup Performance Using Indexing Structures
### Môn: Hệ Điều Hành (Operating Systems)

---

## 1. Summary of Latest Progress

Tuần 3 là cột mốc phân phối đầu tiên của dự án, tập trung vào hoàn thiện **Project Setup** và thuật toán **Linear Search Baseline**. Hiện tại, dự án đã đạt 100% các mục tiêu đề ra cho tuần này.

Các kết quả đạt được:
- ✅ Xây dựng thành công kiến trúc mô-đun vững chắc (cấu trúc thư mục, Makefile chuẩn bị cho 4 phương án).
- ✅ Thiết kế Data Types (struct `DirEntry`) bám sát chuẩn `ext4` của Linux.
- ✅ Triển khai **Linear Search** hoàn chỉnh với cả 3 thao tác: `insert`, `lookup`, và **đặc biệt là `delete`** (đáp ứng đúng yêu cầu "update ops" của đề tài).
- ✅ Xây dựng hệ thống do lường hiệu suất (Benchmark framework) có chức năng đo thời gian bằng nanosecond, đếm số phép so sánh, tính bộ nhớ, và xuất ra file CSV tự động.
- ✅ **Memory Safety:** Chương trình đã được rà soát cực kì khắt khe với **Valgrind**, xác nhận **0 errors, 0 memory leaks** sau hơn 14 ngàn lượt cấp phát bộ nhớ.
- Tổng khối lượng mã nguồn đạt **884 dòng**, biên dịch 0 cảnh báo.

---

## 2. Review Technical Activities

### 2.1 Kiến trúc cốt lõi và Dữ liệu
- **Mô phỏng `ext4_dir_entry_2`:** Để tăng tính thực tế, `DirEntry` được thiết kế có chứa `inode`, loại file (`file_type`), độ dài và tên file ngẫu nhiên (từ đuôi gốc như `.c`, `.so`, `.md`...).
  ```c
  typedef struct {
      uint32_t inode;
      uint16_t rec_len;
      uint8_t  name_len;
      uint8_t  file_type;
      char     name[256]; 
  } DirEntry;
  ```
- **Linear Search Algorithm:** 
  - `linear_insert()`: O(1) Amortized, cơ chế mảng động nhân đôi kích thước khi đầy.
  - `linear_lookup()`: O(n), quét tuần tự để tìm chính xác tên thư mục. Đưa ra được kết quả `hit` và `miss` đúng kỳ vọng.
  - `linear_delete()`: **[Nâng cấp mới]** O(n) tìm kiếm, O(1) thao tác xóa bằng kỹ thuật "Swap with last element" nhằm tránh chi phí dồn mảng O(n).
    ```c
    if (strcmp(dir->entries[i].name, name) == 0) {
        // Swap với phần tử cuối để xóa O(1) thay vì shift O(n)
        dir->entries[i] = dir->entries[dir->count - 1];
        dir->count--;
        return 0; // Xóa thành công
    }
    ```

### 2.2 Framework Đo Lường Benchmark
- Công cụ sử dụng `clock_gettime(CLOCK_MONOTONIC)` để xử lý triệt để bài toán đồng hồ gián đoạn, cho độ nhạy thời gian nano-giây.
  ```c
  #define TIMER_START()    clock_gettime(CLOCK_MONOTONIC, &_ts_start)
  #define TIMER_END()      clock_gettime(CLOCK_MONOTONIC, &_ts_end)
  #define TIMER_ELAPSED_NS() \
      ((uint64_t)(_ts_end.tv_sec - _ts_start.tv_sec) * 1000000000ULL + \
       (uint64_t)(_ts_end.tv_nsec - _ts_start.tv_nsec))
  ```
- Benchmark chạy trên quy mô từ N=100 đến N=1,000,000, thu thập các metric: Insert Time, Lookup Average (ns), Delete Average (ns), Lookup Comparisons, Delete Comparisons.
- Ghi nhận thành công chi phí Delete O(n) với số lượng comparisons dao động sát giá trị $\approx N$, giúp khẳng định rõ rệt giới hạn của cơ chế Linear.

### 2.3 Quản trị Dự An & Bảo mật bộ nhớ
- Dùng công cụ **Valgrind** để profiling toàn bộ Heap memory usage.
- Định hình cấu hình Git repo chuẩn xác (.gitignore loại bỏ file object cache).

---

## 3. Personal Contributions

Dưới đây là bảng phân công và đóng góp chi tiết của 2 thành viên trong nhóm:

### Thành viên 1: [Tên thành viên 1]
- **Vai trò:** Core Developer / System Architect
- **Các file phụ trách:** `linear_search.c/h`, `dir_entry.c/h`, `common.h`
- **Công việc chi tiết:**
  1. Thiếp lập Struct mô phỏng cấu trúc Ext4 (`DirEntry`).
  2. Viết Macro tính toán độ trễ thời gian nanosecond (`TIMER_START/END`).
  3. Hoàn thiện Logic Linear Search bao gồm Khởi tạo, Thêm, Tìm kiếm.
  4. Cập nhật và triển khai thuật toán `linear_delete` với tối ưu O(1) swap.
  5. Phát triển trình Auto-generator tạo hàng triệu Test Cases, Lookup Targets tránh chồng chéo.

### Thành viên 2: [Tên thành viên 2]
- **Vai trò:** Test & Benchmark Engineer / UI Dev
- **Các file phụ trách:** `main.c`, `benchmark.c/h`, `Makefile`, `README.md`
- **Công việc chi tiết:**
  1. Viết hệ thống Build code (Makefile tự động biên dịch và dọn dẹp thư mục `obj`).
  2. Phát triển Framework Benchmark, xử lý logic vòng lặp tính thời gian trung bình.
  3. Xây dựng Data Exporter đẩy kết quả Benchmark sang file `benchmark_results.csv`.
  4. Thực hiện Memory Profiling bằng Valgrind, phát hiện và sửa các bug tràn bộ nhớ tiềm ẩn.
  5. Lên giao diện tương tác (Interactive Demo Menu ở `main.c`) và cập nhật báo cáo tiến độ lên README.

*(**Lưu ý:** Hai bạn hãy đổi tên thành viên 1 và 2 bằng tên thật hoặc mã số sinh viên vào báo cáo chính thức nhé).*

---

## 4. Developed Code Review

### Ưu điểm nổi bật
1. **Khả năng mở rộng (Extensibility):** `benchmark.c/h` thiết kế theo dạng Interface (Menu Enum). Tuần tới tích hợp thuật toán mới Hash Table hay B-Tree chỉ việc cắm các struct hàm vào trong Enum mà không cần làm vỡ Logic UI.
2. **Quản lý vùng nhớ tốt (Memory Safe):** Code C rất dễ bị rò rỉ nếu dùng `malloc()` mà giải phóng (`free()`) sai vị trí. Báo cáo Valgrind `in use at exit: 0 bytes` là lời khẳng định tuyệt đối cho một nền tảng chắc chắn trước khi chuyển sang các cấu trúc con trỏ phức tạp tuần tới (Hash Chaining/Trees).
3. **Thao tác nhanh gọn:** Kỹ năng khắc phục độ trễ `delete()` bằng "swap-with-last" rất thông minh.

### Những điểm cần tối ưu thêm (Bugs/To-dos)
- Benchmark hiện tại đang chạy **1 hiệp duy nhất** cho mỗi node size (Size N). Điều này có thể khiến dữ liệu bị ảnh hưởng nhẹ từ Cache của OS CPU. Có thể bổ sung tính năng chạy 3 Rounds và lấy tính Trung Bình thay vì lấy vòng đơn (Biến `BENCHMARK_ROUNDS` đã khai báo ở Header nhưng chưa thật sự vận hành).
- Mã lỗi trả về từ `malloc` hiện chỉ báo `NULL`, nên đi kèm với dòng log in ra thiết bị ngoại vi hệ thống `fprintf(stderr, ...)` để dò lỗi nhanh.

---

## 5. TODO cho Tuần Tới (Tuần 4)

**Mục tiêu chính: Triển khai Cấu Trúc Khóa Hash Table** 

Để so sánh với O(n) của Linear, Tuần 4 tập trung giải bài toán mục tiêu "Fast lookup" với độ phức tạp lý thuyết là O(1).

Các Checklist đầu việc:
1. **Hash Algorithm:** Viết hàm `djb2_hash` (hoặc nhân tố hash phù hợp) để xử lý String của tên file.
2. **Hash Table Struct:** Tạo struct Hash Bucket kết hợp "Separate Chaining" (Sử dụng Singly Linked List) nhằm hạn chế va vấp (Collision).
3. **Operations:** Phát triển tương thích đồng bộ của cấu trúc Hash bao trọn 4 mặt trận API giống mảng tuyến tính: `hash_create`, `hash_insert`, `hash_lookup`, và `hash_delete`.
4. **Data Threshold Constraint:** Thuật toán rehash (Tự khởi tạo lại kích thước x2 khi `load_factor` vượt ngưỡng trên 70%).
5. Cắm thuật toán `hash_` mới vào menu CLI và `benchmark.c` để ra thông số kiểm định. Chạy lại Valgrind.
