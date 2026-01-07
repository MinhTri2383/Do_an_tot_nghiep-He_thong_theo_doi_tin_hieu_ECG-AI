# ECG Signal Acquisition and Analysis System

## 1. Giới thiệu
Dự án này xây dựng một hệ thống hoàn chỉnh để **thu thập, hiển thị, xử lý và phân tích tín hiệu điện tâm đồ (ECG)**, phục vụ cho nghiên cứu và thử nghiệm các thuật toán xử lý tín hiệu sinh học và mô hình trí tuệ nhân tạo (AI).

Hệ thống được thiết kế theo hướng **phân tách rõ phần cứng – phần mềm – xử lý dữ liệu – mô hình AI**, đảm bảo tính mở rộng và khả năng tái sử dụng trong các nghiên cứu tiếp theo.

---

## 2. Kiến trúc tổng thể hệ thống
Luồng xử lý chính của hệ thống bao gồm:

1. **Thu tín hiệu ECG**
   - Cảm biến Vernier EKG Sensor
   - Tín hiệu analog ECG từ cơ thể người

2. **Thu thập dữ liệu (DAQ)**
   - Thiết bị NI USB-6001
   - Chuyển đổi tín hiệu analog sang số

3. **Phần mềm LabVIEW**
   - Cấu hình DAQ Assistant
   - Hiển thị tín hiệu ECG theo thời gian thực
   - Lọc và tiền xử lý tín hiệu
   - Lưu dữ liệu ra file (CSV)

4. **Xử lý & phân tích bằng Python**
   - Chuẩn hóa tín hiệu
   - Phát hiện đỉnh R
   - Phân đoạn nhịp tim
   - Trích xuất đặc trưng

5. **Mô hình AI**
   - Huấn luyện và đánh giá mô hình phân loại ECG
   - Phân loại trạng thái tim (ví dụ: bình thường / bất thường)

---

## 3. Phần cứng sử dụng
- **Cảm biến ECG**: Vernier EKG Sensor  
- **Thiết bị DAQ**: NI USB-6001  
- **Máy tính**: PC/Laptop chạy Windows  
- **Điện cực**: theo chuẩn đo ECG 3 điện cực

---

## 4. Phần mềm sử dụng
- **Arduino IDE** (quản lý project phần cứng nếu cần)
- **NI LabVIEW**
  - DAQ Assistant
  - Các khối xử lý và hiển thị tín hiệu
- **Python**
  - NumPy, SciPy
  - Pandas
  - Matplotlib
  - Scikit-learn / TensorFlow / PyTorch (tùy mô hình)
- **Git & GitHub** (quản lý mã nguồn)

---

## 5. Tiền xử lý tín hiệu ECG
Quá trình tiền xử lý được thiết kế với các mục tiêu:
- Ổn định biên độ và mức nền tín hiệu bằng **chuẩn hóa thống kê**
- Xác định chính xác **vị trí đỉnh R**
- Phân đoạn tín hiệu theo từng chu kỳ tim
- Chuẩn bị dữ liệu đầu vào cho mô hình AI

Lưu ý: Trong giai đoạn này **không áp dụng lọc nhiễu số phức tạp**, nhằm giữ nguyên đặc trưng tín hiệu gốc thu từ phần cứng.

---

Do_an_tot_nghiep/
├── Circuit/ # Thiết kế mạch và PCB ECG
│ ├── AD8232_board_circuit.* # Project mạch AD8232
│ ├── ECG-N16R8.* # Project mạch ECG chính
│ ├── Mach_bao_cao.prn
│ ├── ECG-N16R8.pdf # Sơ đồ mạch xuất PDF
│ └── Project Backups/ # Các bản sao lưu thiết kế mạch
│
├── Data/ # Dữ liệu ECG thu thập được
│ ├── raw/ # Dữ liệu ECG thô
│ └── processed/ # Dữ liệu sau tiền xử lý
│
├── Programme/ # Mã nguồn và chương trình
│ ├── ArduinoIDE/ # Project Arduino IDE
│ ├── ECG-UNO/ # Chương trình thu ECG với Arduino Uno
│ ├── Python code/ # Code xử lý tín hiệu ECG bằng Python
│ ├── AI_model/ # Mô hình AI (CNN, ML)
│ └── Colab Notebook/ # Notebook chạy trên Google Colab
│
├── USB-6001--EKG-Vernier/ # Thu ECG bằng NI USB-6001 & Vernier EKG
│ ├── LabVIEW/ # File VI LabVIEW
│ └── Configuration/ # Cấu hình DAQ và thiết bị
│
├── Documents/ # Tài liệu tham khảo và báo cáo
│ ├── Datasheet/ # Datasheet linh kiện
│ └── Paper/ # Bài báo, tài liệu nghiên cứu
│
├── Picture/ # Hình ảnh minh họa
│ └── 1D-CNN/ # Hình kết quả mô hình AI
│
└── solidworks/ # Thiết kế cơ khí (vỏ thiết bị)

---

## 7. Mục tiêu và phạm vi
- Hiểu rõ chuỗi thu nhận và xử lý tín hiệu ECG thực tế
- Thử nghiệm các thuật toán phát hiện đặc trưng ECG
- Đánh giá khả năng ứng dụng AI trong phân tích ECG
- Làm nền tảng cho các nghiên cứu y sinh nâng cao

---

## 8. Hạn chế
- Chất lượng tín hiệu phụ thuộc nhiều vào điều kiện đo và người đo
- Chưa tối ưu chống nhiễu phần cứng
- Mô hình AI chỉ mang tính nghiên cứu, **không dùng cho chẩn đoán y khoa**

---

## 9. Ghi chú
Dự án được thực hiện với mục đích **nghiên cứu và học thuật**.  
Mọi kết quả phân tích **không thay thế ý kiến của bác sĩ hoặc chuyên gia y tế**.

---

## 10. Tác giả
- Người thực hiện: *(điền tên của bạn)*
- Lĩnh vực: Xử lý tín hiệu – Hệ thống nhúng – AI
