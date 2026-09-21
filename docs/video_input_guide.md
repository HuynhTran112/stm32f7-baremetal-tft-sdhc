# 🎬 HƯỚNG DẪN TẠO DỮ LIỆU VIDEO CHO STM32F746G-DISCO (FAT32 SDCARD)

Tài liệu hướng dẫn cách chuẩn bị thẻ nhớ MicroSD (SanDisk 8GB MicroSD HC I Class 4 hoặc tương đương), định dạng FAT32 và chuyển đổi video MP4 thành file nhị phân `.BIN` chuẩn RGB565 $480 \times 272$ cho trình phát Video Bare-Metal STM32F746G-DISCO.

---

## 💾 1. Định dạng Thẻ nhớ MicroSD sang FAT32

### Bước 1: Format thẻ với Cluster Size lớn (32KB / 64KB)
1. Cắm thẻ nhớ SanDisk 8GB vào máy tính qua đầu đọc thẻ USB.
2. Mở **File Explorer** $\rightarrow$ Chuột phải vào ổ đĩa thẻ nhớ $\rightarrow$ chọn **Format...**
3. Cấu hình các thông số:
   - **File system:** `FAT32` (Default).
   - **Allocation unit size:** `32 kilobytes` hoặc `64 kilobytes`.  
     *(Giải thích kỹ thuật: Cluster size lớn giúp giảm số lần tra cứu bảng FAT, tăng tốc độ đọc tuần tự đa khối của DMA SDMMC lên trên $15\text{ MB/s}$, loại bỏ hoàn toàn độ trễ).*
   - **Format options:** Tích chọn *Quick Format*.
4. Nhấn **Start** để hoàn tất định dạng trong 3 giây.

---

## 🎞️ 2. Chuyển đổi Video sang Chuẩn RGB565 ($480 \times 272$)

File video nạp vào thẻ nhớ là dạng nhị phân thô (`.BIN` hoặc `.RAW`), không cần header phức tạp:
- **Độ phân giải:** Đúng chuẩn màn hình Rocktech RK043FN48H ($480 \times 272$ pixels).
- **Định dạng màu:** 16-bit RGB565 Little-Endian (2 bytes/pixel).
- **Dung lượng 1 khung hình:** $480 \times 272 \times 2 = 261,120\text{ bytes}$ (khoảng $255\text{ KB}$).
- **Tốc độ khung hình (FPS):** Khuyên dùng $30\text{ FPS}$ (tiêu tốn $\approx 7.8\text{ MB/s}$ băng thông đọc thẻ) hoặc $60\text{ FPS}$ (tiêu tốn $\approx 15.6\text{ MB/s}$).

### Bước 1: Cài đặt thư viện Python (Chỉ làm 1 lần)
Mở PowerShell hoặc Command Prompt và chạy:
```bash
pip install opencv-python numpy
```

### Bước 2: Chạy công cụ chuyển đổi video
Trong thư mục dự án, chạy lệnh:
```bash
# Chuyển đổi video MP4 thành VIDEO1.BIN ở tốc độ 30 FPS:
python D:\Project\TFT_video_STM32F7\tools\convert_video.py "D:\Videos\my_clip.mp4" "D:\Project\TFT_video_STM32F7\VIDEO1.BIN" --fps 30
```

### Bước 3: Copy file vào thẻ nhớ
- Copy các file video (ví dụ: `VIDEO1.BIN`, `VIDEO2.BIN`, `VIDEO3.BIN`) vào **thư mục gốc (Root Directory)** của thẻ nhớ (ví dụ: `E:\VIDEO1.BIN`).
- Rút an toàn thẻ nhớ (Eject) và cắm vào khe cắm MicroSD ở mặt dưới board STM32F746G-DISCO.

---

## 🎮 3. Trải nghiệm trên Kit STM32F746G-DISCO

1. **Nạp Firmware:**
   ```powershell
   powershell -ExecutionPolicy Bypass -File D:\Project\TFT_video_STM32F7\build.ps1 -Action flash
   ```
2. **Khởi động:**
   - Khi cắm nguồn, board hiển thị màn hình Splash Screen với thanh tiến trình màu xanh.
   - Hệ thống tự động mount FAT32 và quét các file `.BIN` trong thẻ nhớ để phát mượt mà ở tốc độ cao không giật xé hình.
   - **Nút nhấn User Button (nút xanh dương PI11):** Bấm nút để chuyển luân phiên giữa các video `VIDEO1.BIN` $\rightarrow$ `VIDEO2.BIN` $\rightarrow$ `VIDEO3.BIN`.
   - Nếu rút thẻ nhớ hoặc không tìm thấy file video: Hệ thống tự động kích hoạt chế độ biểu diễn đồ họa Benchmark Chrom-ART 60 FPS (Color Bar & Bouncing Sprite).
