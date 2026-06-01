# 📑 BÁO CÁO TỔNG KẾT DỰ ÁN
## Cải thiện hiệu năng tra cứu thư mục bằng cấu trúc chỉ mục
### Môn: Hệ Điều Hành (Operating Systems)

---

## 1. Đặt Vấn Đề & Mục Tiêu

Trong các hệ điều hành Linux, cụ thể là các phiên bản filesystem ext2, ext3, ext4, việc tra cứu thư mục (directory lookup) đóng vai trò cực kỳ quan trọng đối với hiệu năng chung của hệ thống. 
- Filesystem thế hệ cũ (như ext2) sử dụng **Linear Search** (tìm kiếm tuyến tính) với độ phức tạp $O(N)$ để tìm file trong thư mục. Phương pháp này dẫn đến hiện tượng thắt cổ chai (bottleneck) nghiêm trọng khi số lượng file trong một thư mục lên tới hàng chục nghìn.
- Dự án này được thực hiện nhằm mục tiêu **mô phỏng, cài đặt và so sánh hiệu năng** của 4 cấu trúc chỉ mục: Linear Search, Hash Table, B-Tree và HTree (cấu trúc tối ưu đang được sử dụng trong ext4). Qua đó, chứng minh bằng thực nghiệm lý do ext4 quyết định chuyển sang sử dụng HTree.

---

## 2. Cơ Sở Lý Thuyết & Các Giải Pháp Triển Khai

Dự án đã phát triển một framework bằng ngôn ngữ C, mô phỏng cấu trúc `DirEntry` (Directory Entry) giống với nhân Linux và đánh giá 4 phương pháp quản lý:

### 2.1. Linear Search (Baseline - Tiêu chuẩn cơ sở)
- Lưu trữ các entry trong một mảng cấp phát động liên tục. Tra cứu bằng cách duyệt tuần tự từ đầu đến cuối mảng.
- **Độ phức tạp:** Lookup $O(N)$, Insert $O(1)$ (append ở cuối mảng), Delete $O(N)$.
- **Ưu/Nhược điểm:** Cài đặt rất đơn giản, overhead bộ nhớ cực thấp (không cần con trỏ phụ). Tuy nhiên, tốc độ lookup quá chậm với thư mục lớn (hàng vạn files).

### 2.2. Hash Table
- Bảng băm (Hash Table) sử dụng hàm băm **DJB2** nổi tiếng kết hợp kỹ thuật **Separate Chaining** (dùng danh sách liên kết để giải quyết đụng độ).
- **Độ phức tạp:** Lookup $O(1)$ (trung bình), Insert $O(1)$ (trung bình), Delete $O(1)$ (trung bình).
- **Ưu/Nhược điểm:** Tốc độ lookup cực nhanh (nhanh nhất về mặt thuật toán). Nhược điểm là tốn nhiều bộ nhớ phụ (con trỏ), gây phân mảnh bộ nhớ và hoàn toàn **không thân thiện với ổ đĩa (Disk-unfriendly)**.

### 2.3. B-Tree
- Cây tìm kiếm đa phân tự cân bằng. Các Node được thiết kế giả lập kích thước của một disk block để tối ưu số lần truy xuất (I/O) ổ đĩa.
- **Độ phức tạp:** Lookup $O(\log N)$, Insert $O(\log N)$, Delete $O(\log N)$.
- **Ưu/Nhược điểm:** Hiệu năng ổn định, giữ các tên file theo thứ tự (cho phép lấy danh sách file được sắp xếp). Nhược điểm là việc Insert tốn kém chi phí do phải chia node (split) liên tục. Chiều cao cây tăng nhanh khi độ dài tên file dài, làm giảm số lượng phần tử mỗi node (fan-out thấp).

### 2.4. HTree (Giải pháp của ext3/ext4)
- **Hash Tree** là cấu trúc kết hợp sức mạnh của Hash Table và B-Tree. Điểm mấu chốt là Index Node (node trung gian) **chỉ lưu giá trị băm (Hash 32-bit)** của tên file thay vì lưu chuỗi tên file dài. Dự án dùng hàm **Half-MD4** mô phỏng ext4.
- **Độ phức tạp:** Lookup $O(\log_b N + K)$ (Trong đó $b$ là fan-out, $K$ là số phần tử xung đột trong Leaf Node).
- **Ưu/Nhược điểm:** Khả năng **Disk-friendly** xuất sắc. Do chỉ lưu số nguyên 32-bit (hash key), một block 4KB chứa được tới hàng trăm entries, giữ chiều cao cây $h \le 2$ dù có hàng triệu file. Các node lá (Leaf) được thiết kế giống block ext2 cũ, giúp tương thích ngược dễ dàng.

---

## 3. Kết Quả Benchmark Toàn Diện

Sử dụng framework đo lường chuẩn xác nanosecond (`clock_gettime(CLOCK_MONOTONIC)`). 
Kịch bản test: Các kích thước $N$ từ 100 đến **1.000.000 entries**. 
Kiểm tra an toàn bộ nhớ (Valgrind): **0 errors, 0 memory leaks** trên tổng cộng hơn 3 triệu lượt cấp phát memory.

### Bảng So Sánh Hiệu Năng Ở Quy Mô 1.000.000 Entries

| Phương pháp | Lượt so sánh trung bình | Thời gian tra cứu - Lookup (ns) | Thời gian xóa - Delete (ns) | Memory Overhead | Speedup vs Linear |
|-------------|:---:|----------------:|----------------:|----------------:|-------------------|
| **Linear Search** | 494,913 | 87,705,671 | 182,582,981 | Baseline | 1x |
| **Hash Table** | 1 | 4,183 | 1,854 | +4.3% | ~20,967x |
| **B-Tree** | 100 | 55,602 | 72,459 | +42.9% | ~1,578x |
| **HTree** | 37 | **24,914** | **44,397** | **+37.2%** | **~3,519x** |

*(Biểu đồ phân tích dạng line-chart log-log scale đã được xuất ra trong thư mục `results/` để minh họa tốc độ vượt trội).*

---

## 4. Phân Tích Chuyên Sâu: Tại Sao ext4 Chọn HTree?

Dù Hash Table có $O(1)$ và chạy nhanh nhất trong môi trường RAM, nhưng trong thiết kế hệ điều hành, **tương tác với ổ cứng cứng (Disk I/O Block)** mới là nhân tố quyết định hiệu suất hệ thống:
1. **Hash Table** sinh ra vô số truy cập ngẫu nhiên (random memory access) do các con trỏ linked list phân mảnh. Khi load từ đĩa, nó sẽ bắt hệ thống đọc rất nhiều khối đĩa khác nhau dẫn đến thắt cổ chai I/O.
2. **B-Tree** tốt cho block I/O nhưng việc phải lưu trữ tên file (vốn có độ dài thay đổi, có thể lên tới 255 ký tự) ở các node trung gian làm giảm số lượng con của mỗi node (fan-out thấp). Chiều cao cây tăng lên đồng nghĩa với việc phải load nhiều block đĩa hơn vào bộ nhớ.
3. **HTree** giải quyết triệt để vấn đề:
   - Các Index Node **chỉ lưu mã Hash 32-bit**, không lưu tên file. Điều này giúp một Index block 4KB chứa tới hàng trăm lối rẽ (pointers), giữ độ cao của cây luôn ở mức 1 hoặc 2. (Chỉ cần 1-2 lần I/O read là tìm đến block lá).
   - Thiết kế block lá tương thích trực tiếp cấu trúc cũ, tiết kiệm chi phí nâng cấp cho hàng vạn máy chủ.

**Kết luận kỹ thuật:** HTree là kiến trúc hoàn hảo nhất để tối ưu I/O. Nó mang lại tốc độ vượt trội gấp ngàn lần so với thế hệ cũ, đồng thời giữ kiến trúc thân thiện với việc truy xuất theo các trang vật lý trên đĩa.

---

## 5. Tổng Kết Đánh Giá

Dự án đã mô phỏng thành công các hệ thống chỉ mục được ứng dụng trực tiếp trong mã nguồn hạt nhân Linux. Các kết quả định lượng thu được chứng minh rõ rệt tính đúng đắn trong các quyết định thiết kế của ext4. Qua dự án, nhóm đã thành thạo kỹ năng lập trình hệ thống bằng ngôn ngữ C, quản lý bộ nhớ an toàn (memory safety) và tạo framework benchmark chuyên nghiệp.
