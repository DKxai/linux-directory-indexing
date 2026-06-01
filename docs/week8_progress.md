# 📋 BÁO CÁO TIẾN ĐỘ TUẦN 8
## Dự án: Improving Directory Lookup Performance Using Indexing Structures
### Môn: Hệ Điều Hành (Operating Systems)

---

## 1. Summary of Latest Progress

Tuần 8 là tuần cuối cùng của dự án. Mục tiêu trọng tâm của tuần này là **Tổng hợp dữ liệu, Viết Báo cáo Cuối kỳ, Tinh chỉnh mã nguồn (Final Polish)** và chuẩn bị cho buổi bảo vệ dự án.

Các công việc đã hoàn thành:
- ✅ **Viết Báo Cáo Tổng Kết (Final Report)**: Hoàn thiện tài liệu, tổng hợp toàn bộ lý thuyết, kiến trúc, và kết quả benchmark của 4 cấu trúc dữ liệu (Linear, Hash, B-Tree, HTree).
- ✅ **Terminal UI Polish**: Tinh chỉnh lại giao diện console khi chạy `./benchmark`. Sửa lỗi hiển thị ASCII art, cập nhật version và làm gọn menu để giao diện chuyên nghiệp hơn.
- ✅ **Chuẩn hóa Codebase**: Xóa các comment tiếng Việt có dấu trong source code C để tránh lỗi encoding trên các trình biên dịch khác nhau. Xóa các file rác và temporary files.
- ✅ **Giải quyết câu hỏi cốt lõi**: Hoàn thiện phần phân tích và trả lời chi tiết cho câu hỏi "Tại sao ext4 chọn HTree thay vì Hash Table hay B-Tree?" dựa trên kết quả Benchmark và kiến thức I/O block.
- ✅ **Version Control**: Gộp (merge) các nhánh cuối cùng, đảm bảo Git history sạch sẽ với các commit message rõ ràng.

---

## 2. Review Technical Activities

### 2.1 Hoàn thiện Báo cáo cuối kỳ

Báo cáo cuối kỳ đã được soạn thảo và lưu trong thư mục `docs/final_report.md`. Báo cáo được cấu trúc với các phần chính:
- **Đặt vấn đề**: Tầm quan trọng của Directory Lookup trong Filesystem.
- **Cơ sở lý thuyết**: Phân tích đặc điểm của 4 cấu trúc chỉ mục.
- **Thiết kế hệ thống**: Trình bày kiến trúc mô phỏng, benchmark framework và API.
- **Đánh giá Benchmark**: Bảng so sánh 10 metrics với N=1.000.000, tốc độ (speedup) và bộ nhớ (memory overhead).
- **Kết luận kiến trúc**: Đánh giá tính Disk-friendly và lý do ext4 ứng dụng HTree.

### 2.2 Tinh chỉnh UI & Chuẩn hóa Code

- Đã rà soát `main.c` và chỉnh sửa các dòng in ra màn hình console (loại bỏ chữ "Week 6/7", căn giữa menu).
- Đã kiểm tra lại `Makefile` để đảm bảo các lệnh `make run`, `make plot`, `make valgrind` hoạt động mượt mà không có cảnh báo.
- Dùng `valgrind` test lại phiên bản cuối cùng: Xác nhận vẫn giữ vững mốc **0 errors, 0 memory leaks**.

### 2.3 Quản lý Git & Thư mục

- Thư mục `results/` đã lưu trữ các biểu đồ PNG mới nhất và `benchmark_results.csv` cuối cùng.
- Thư mục `scripts/` đã được commit đầy đủ.

---

## 3. Khó Khăn & Kinh Nghiệm Rút Ra Trong Toàn Dự Án

- **Về Kỹ thuật**: Việc mô phỏng chính xác hoạt động của I/O disk bằng RAM (cấp phát động) đòi hỏi quản lý bộ nhớ cực kỳ chặt chẽ ở ngôn ngữ C. Việc tích hợp **Valgrind** từ những tuần đầu là quyết định đúng đắn nhất giúp dự án không bị vỡ (crash) ở giai đoạn cuối.
- **Về Phân tích**: Để đưa ra kết luận thuyết phục, việc chỉ dựa vào độ phức tạp Big-O là không đủ. Bắt buộc phải hiểu kiến trúc phần cứng (disk blocks, random vs sequential access) để thấy được sự ưu việt của HTree.
- **Về Quy trình**: Áp dụng quy trình phát triển phần mềm chuẩn với Makefile, CI/CD cơ bản (test script), và Git branches giúp nhóm dễ dàng track tiến độ và debug.

---

## 4. Personal Contributions

### Thành viên 1: [Tên thành viên 1]
- **Vai trò:** Core Developer / Algorithm Engineer
- **Công việc Tuần 8:**
  1. Hỗ trợ rà soát lại lý thuyết của B-Tree và HTree để đưa vào báo cáo tổng kết.
  2. Viết phần phân tích "Tại sao ext4 chọn HTree" dựa trên logs benchmark tuần 7.
  3. Dọn dẹp code, xóa comment tiếng Việt có dấu.

### Thành viên 2: [Tên thành viên 2]
- **Vai trò:** Benchmark Engineer / Documentation Lead
- **Công việc Tuần 8:**
  1. Trực tiếp soạn thảo `final_report.md` tổng hợp toàn bộ kết quả.
  2. Tinh chỉnh UI trong `main.c` và chạy Valgrind lần cuối.
  3. Kiểm tra lại Makefile và chuẩn bị các file cần thiết cho buổi nộp bài.

*(**Lưu ý:** Hai bạn hãy đổi tên thành viên 1 và 2 bằng tên thật hoặc mã số sinh viên vào báo cáo chính thức nhé).*

---

## 5. Kết Luận Tiến Độ

Dự án đã **HOÀN THÀNH 100%** các hạng mục đề ra từ đầu học kỳ. Mã nguồn ổn định, không có memory leak, tài liệu đầy đủ và chuyên nghiệp. Nhóm đã sẵn sàng cho việc nộp bài và bảo vệ dự án.
