# 🏆 [CHUYÊN ĐỀ LƯU TRỮ] CẨM NANG TOÀN DIỆN BARE-METAL SDMMC1 4-BIT BUS, CHUẨN HÓA THẺ SDHC CHO DỰ ÁN & HỆ THỐNG TỆP TIN FATFS

## Lộ trình 4 Bước: Nguyên Lý Phần Cứng ➔ Tra Cứu RM0385 ➔ Gõ Code Driver Modular ➔ Phỏng Vấn Chuyên Sâu

> [!IMPORTANT]
> **TÀI LIỆU THAM CHIẾU NỀN TẢNG:**
> * Kiến trúc Bare-Metal nền tảng: [`day00_baremetal_foundations.md`](file:///d:/Project/STM32F7/day00_baremetal_foundations.md)
> * Cấu hình xung nhịp 216MHz: [`day01_learning_guide.md`](file:///d:/Project/STM32F7/day01_learning_guide.md)
> * Bộ nhớ ngoài FMC SDRAM & LTDC: [`day04_learning_guide.md`](file:///d:/Project/STM32F7/day04_learning_guide.md)
> 
> **MỤC TIÊU CHUYÊN ĐỀ (CHUẨN HÓA THẺ SDHC CHO DỰ ÁN):**
> 1. **Dự Án Chuẩn Hóa 100% Thẻ Nhớ MicroSD SDHC (4GB - 32GB):** Thẻ SDHC là chuẩn công nghiệp thông dụng nhất hiện nay trong các thiết bị nhúng và ô tô. Tài liệu này tập trung mổ xẻ cơ chế **Block/Sector Addressing (LBA)** của thẻ SDHC (truyền thẳng số thứ tự Sector, tuyệt đối không nhân 512), quá trình đàm phán cờ `HCS = 1` trong `ACMD41` và nhận diện cờ `CCS = 1` trong thanh ghi `OCR`.
> 2. **Toàn Bộ Lý Thuyết Tập Trung (Single Source of Truth):** Không phân mảnh lý thuyết ở nhiều nơi. Nắm trọn vẹn bản chất giao thức thẻ SDHC, cơ chế bus 4-bit tần số 48 MHz, và kiến trúc 4 phân vùng của hệ thống tệp tin FAT32.
> 3. **Sơ Đồ Tuần Tự Chuẩn Mực:** Mô hình hóa 8 bước bắt tay khởi tạo thẻ SDHC và luồng đọc ghi dữ liệu tốc độ cao kèm giải quyết xung đột L1 D-Cache Cortex-M7 bằng Mermaid Sequence Diagram.
> 4. **Mã Nguồn Nguyên Khối (Có Đầu Có Đuôi):** Module driver `sdmmc.h` / `sdmmc.c` tối ưu cho SDHC, tầng cầu nối `diskio.c` cho ChaN FatFs và ứng dụng phát video 60 FPS / Hộp đen ghi dữ liệu CAN Logger.
> 5. **CẤM TUYỆT ĐỐI DÙNG LATEX:** 100% công thức toán học và đơn vị đo được viết bằng plain text / code block (`48 MHz`, `400 kHz`, `512 bytes`, `15.66 MB/s`).

---

```text
┌─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                      LỘ TRÌNH 4 BƯỚC CHINH PHỤC SDMMC1 & THẺ SDHC BARE-METAL                    │
├───────────────────┬───────────────────┬────────────────────────────┬────────────────────────────┤
│ BƯỚC 1: NGUYÊN LÝ │ BƯỚC 2: TRA CỨU   │ BƯỚC 3: THIẾT KẾ DRIVER    │ BƯỚC 4: PHỎNG VẤN          │
│ • Giao thức SDHC  │ • RM0385 Chap 29  │ • Gắn nhãn file cụ thể     │ • Bộ 5 câu hỏi vặn Lưu Trữ │
│ • LBA Addressing  │ • Sơ đồ chân      │ • Khối 1 [sdmmc.h / c]     │   & File System Nhúng      │
│ • 4 Vùng thẻ FAT32│ • Công thức Clock │ • Khối 2 [diskio.c]        │ • Bẫy Block Address SDHC   │
│ • Sector vs Cluster│   CLKDIV & Timer  │ • Khối 3 [media_player.c]  │ • D-Cache Coherency Trap   │
│ • Băng thông 60FPS│ • Sơ đồ tuần tự   │ • Mổ xẻ 5 Bug phần cứng    │ • Kịch bản trả lời 60s     │
│ • Kiến trúc 3 tầng│   Init & Dataflow │   thực chiến SDHC          │   (Elevator Pitch)         │
└───────────────────┴───────────────────┴────────────────────────────┴────────────────────────────┘
```

---

## 🛠️ RÀ SOÁT 7 QUY TẮC AN TOÀN VÀ LƯU TRỮ CỐT LÕI (STORAGE CORE ESSENTIALS)

| STT | Quy tắc Kỹ thuật Lưu trữ | Thể hiện cụ thể trong SDMMC & FatFs |
| :---: | :--- | :--- |
| **1** | **Frequency Staging Rule** | Quá trình nhận diện thẻ SD BẮT BUỘC khởi động ở xung nhịp thấp (`f_OD <= 400 kHz`) để mọi dòng thẻ tương thích điện áp. Chỉ sau khi thẻ trả về địa chỉ RCA mới được tăng tốc lên chế độ Data Transfer (`24 MHz` hoặc `48 MHz`). |
| **2** | **SDSC vs SDHC Addressing** | Thẻ SDSC (dung lượng nhỏ hơn hoặc bằng 2GB) dùng **Byte Addressing** (địa chỉ nhân với 512). Thẻ SDHC/SDXC (từ 4GB đến 32GB/2TB) dùng **Block/Sector Addressing** (địa chỉ chính là số thứ tự Sector). Gửi nhầm sẽ gây lỗi đọc sai vùng nhớ hoặc tràn số 32-bit! |
| **3** | **W1C Interrupt Flags** | Các cờ ngắt và trạng thái trong thanh ghi `SDMMC_STA` phải được xóa bằng thanh ghi `SDMMC_ICR` bằng phép gán trực tiếp: `SDMMC1->ICR = 0x00FFFFFF;` (Tuyệt đối không dùng `|=`). |
| **4** | **D-Cache 32-Byte Alignment** | Bộ đệm đọc/ghi khối thẻ nhớ phải được căn lề 32 bytes (`__attribute__((aligned(32)))`) và gọi `SCB_InvalidateDCache_by_Addr()` để chống lỗi đọc dữ liệu rác từ L1 Cache Cortex-M7. |
| **5** | **Power-Fail Resilience** | Khi ghi dữ liệu vào tệp tin (CAN Logger / Hộp đen EDR), bắt buộc phải định kỳ gọi `f_sync(&fil)` để đẩy toàn bộ dữ liệu từ RAM Cache xuống Sector vật lý của thẻ, chống mất mát dữ liệu khi xe tắt khóa điện đột ngột. |
| **6** | **Card Busy Timeout** | Sau mỗi lệnh ghi (CMD24/CMD25) hoặc đọc khối lớn, thẻ nhớ kéo đường `DAT0` xuống LOW (Busy state). Driver phải có bộ đếm Timeout phần mềm để thoát vòng lặp an toàn, tránh treo vi điều khiển nếu thẻ bị rút đột ngột. |
| **7** | **Zero-Copy Streaming** | Trong bài toán phát video LCD 60 FPS, hàm `f_read()` phải nạp trực tiếp vào địa chỉ Framebuffer của SDRAM ngoài (`0xC0000000`), không thông qua bộ đệm trung gian RAM nội (Zero-Copy Architecture). |

---

# 🧠 BƯỚC 1: NGUYÊN LÝ VẬT LÝ SD CARD & HỆ THỐNG TỆP TIN FAT32 (GOM 1 NƠI DUY NHẤT)

---

### 1.1. Bản Chất Phân Loại Thẻ Nhớ: Tại Sao Dự Án Chuẩn Hóa Dùng Thẻ SDHC?

> [!NOTE]
> **🎯 LÝ DO DỰ ÁN CHỌN VÀ CHUẨN HÓA 100% TRÊN THẺ MICROSD SDHC (4GB - 32GB):**
> 1. **Tính sẵn có ngoài thị trường:** Thẻ SDSC cổ điển (<= 2GB) hiện nay đã ngừng sản xuất. Thẻ SDHC là dòng thẻ phổ biến nhất, giá rẻ và tương thích hoàn hảo với khe cắm thẻ microSD trên bo mạch STM32F746G-Discovery.
> 2. **Tương thích gốc với FAT32:** Thẻ SDHC mặc định được định dạng chuẩn FAT32, khớp 100% với kiến trúc của thư viện ChaN FatFs mà không cần cấu hình exFAT phức tạp.
> 3. **Dung lượng tối ưu cho Automotive & Media:** Dung lượng 8GB - 32GB đủ chứa hàng triệu bản tin CAN ghi sự cố (Blackbox EDR) hoặc hàng chục video nén dung lượng cao phát 60 FPS.
> 4. **Cơ chế Block Addressing (LBA):** SDHC chuyển từ đánh địa chỉ Byte sang đánh địa chỉ Block (512 bytes/block), loại bỏ hoàn toàn nguy cơ tràn biến 32-bit khi truy cập dung lượng lớn hơn 4GB.

Hiệp hội Thẻ nhớ Quốc tế (SD Association) phân định rõ các thế hệ thẻ nhớ:

| Tiêu Chí Kỹ Thuật | 1. SDSC (Standard Capacity) | 2. SDHC (High Capacity) — [DỰ ÁN SỬ DỤNG] | 3. SDXC (eXtended Capacity) |
| :--- | :--- | :--- | :--- |
| **Dung lượng hỗ trợ** | Nhỏ hơn hoặc bằng **2 GB** (Đã lỗi thời) | **Từ 4 GB đến 32 GB (Chuẩn của dự án)** | Từ **64 GB đến 2 TB** (Cần exFAT) |
| **Chuẩn SD Version** | SD Version 1.0 đến 1.10 | **SD Version 2.00** | SD Version 3.00 |
| **Chế độ đánh địa chỉ (Addressing Mode)** | **Byte Addressing** (Địa chỉ Byte) | **Block / Sector Addressing (LBA)** | **Block / Sector Addressing (LBA)** |
| **Tham số lệnh CMD17/CMD18/CMD24** | `Argument = Sector_Number * 512` | `Argument = Sector_Number (Truyền thẳng)` | `Argument = Sector_Number` |
| **Hệ thống tệp tin chuẩn** | FAT12 / FAT16 | **FAT32 (100% khớp với FatFs)** | exFAT (hoặc format lại FAT32) |
| **Bit CCS trong phản hồi ACMD41** | `Bit 30 (CCS) = 0` | **Bit 30 (CCS) = 1 (Xác nhận SDHC)** | `Bit 30 (CCS) = 1` |

#### Bẫy Bug Địa Chỉ Kinh Điển Giữa SDSC và SDHC:
* Giả sử bạn muốn đọc **Sector số 100**:
  * Nếu thẻ là **SDSC**: Lệnh `CMD17` yêu cầu tham số là địa chỉ byte: `100 * 512 = 51200` (`0x0000C800`).
  * Nếu thẻ là **SDHC**: Lệnh `CMD17` yêu cầu tham số là số thứ tự block: `100` (`0x00000064`).
* **Hậu quả nếu code không phân biệt:** Nếu bạn cắm thẻ SDHC 16GB vào mà code driver vẫn nhân với 512, khi đọc sector lớn (ví dụ Sector số 10,000,000), phép tính `10,000,000 * 512 = 5,120,000,000` sẽ **tràn biến 32-bit (`uint32_t`)**, dẫn đến việc vi điều khiển đọc sai sector hoặc thẻ báo lỗi `ADDR_OUT_OF_RANGE` ngay lập tức!
* **Quy trình nhận diện tự động của Driver:**
  1. Gửi lệnh `CMD8` với tham số `0x000001AA` (Kiểm tra xem thẻ có hỗ trợ chuẩn SD Version 2.0 hay không).
  2. Gửi lệnh `ACMD41` với bit `HCS (Host Capacity Support - Bit 30) = 1`.
  3. Đọc thanh ghi `OCR` (Operation Conditions Register) do thẻ trả về:
     * Nếu **Bit 30 (`CCS - Card Capacity Status`) = 1**: Lưu cờ `CardType = SDHC` (Đánh địa chỉ Block).
     * Nếu **Bit 30 (`CCS`) = 0**: Lưu cờ `CardType = SDSC` (Đánh địa chỉ Byte).

---

### 1.2. Kiến Trúc 3 Tầng Của Hệ Thống Tệp Tin Nhúng (FatFs Architecture)

Hệ thống lưu trữ chuyên nghiệp trong thiết bị nhúng luôn phân tách độc lập 3 tầng:

```text
┌──────────────────────────────────────────────────────────────────────────────────┐
│  TẦNG 1: ỨNG DỤNG NGƯỜI DÙNG (Application Layer)                                │
│  • Hàm gọi: f_mount(), f_open(), f_read(), f_write(), f_sync(), f_close()         │
│  • Quản lý: Danh sách video phát nhạc, file log CAN EDR, file thông số cấu hình  │
└────────────────────────────────────────┬─────────────────────────────────────────┘
                                         │ (Chuẩn ANSI C POSIX-like API)
┌────────────────────────────────────────▼─────────────────────────────────────────┐
│  TẦNG 2: BỘ NÃO QUẢN LÝ TỆP TIN FATFS (ChaN Core Engine: ff.c & ffconf.h)       │
│  • Thuật toán quản lý bảng FAT1 / FAT2, giải mã chuỗi liên kết Cluster           │
│  • Phân tích Boot Sector (BPB), quản lý bảng thư mục Root Directory 32-byte     │
│  • Không phụ thuộc phần cứng (100% phần mềm thuần túy)                           │
└────────────────────────────────────────┬─────────────────────────────────────────┘
                                         │ (Giao diện chuẩn diskio.h)
┌────────────────────────────────────────▼─────────────────────────────────────────┐
│  TẦNG 3: CẦU NỐI PHẦN CỨNG BARE-METAL (Hardware Glue Layer: diskio.c & sdmmc.c)  │
│  • disk_initialize() ──> Gọi hàm Bare-Metal SDMMC_Init() (Nhận diện SDSC/SDHC)  │
│  • disk_read()       ──> Gọi hàm Bare-Metal SDMMC_ReadMultiBlocks()              │
│  • disk_write()      ──> Gọi hàm Bare-Metal SDMMC_WriteMultiBlocks()             │
│  • disk_ioctl()      ──> Lấy kích thước sector (512 bytes), đồng bộ CTRL_SYNC    │
└──────────────────────────────────────────────────────────────────────────────────┘
```

---

### 1.3. Bản Chất Bốn Vùng Nhớ Của Hệ Thống Tệp Tin FAT32

Khi một thẻ nhớ MicroSD được định dạng (format) chuẩn FAT32 trên máy tính, không gian lưu trữ vật lý được phân chia thành 4 vùng độc lập:

```text
┌──────────────┬──────────────┬──────────────┬──────────────────────────────────────────┐
│ 1. BOOT      │ 2. BẢNG FAT1 │ 3. BẢNG FAT2 │ 4. VÙNG DỮ LIỆU THỰC TẾ (DATA REGION)    │
│    SECTOR    │ (Mảng index) │ (Bản sao lưu)│    (Chia thành các Cụm / Clusters)       │
├──────────────┼──────────────┼──────────────┼──────────────────────────────────────────┤
│ Sector 0     │ Danh sách    │ Giống hệt    │ Chứa nội dung file video, log CAN, ảnh...│
│ Chứa bảng    │ liên kết     │ FAT1 để dự   │ Cluster 2, Cluster 3, Cluster 4...       │
│ tham số BPB  │ chuỗi cụm    │ phòng sự cố  │ (Mỗi cluster = 32 KB = 64 sectors)       │
└──────────────┴──────────────┴──────────────┴──────────────────────────────────────────┘
```

1. **Boot Sector (VBR - Volume Boot Record)**:
   - Nằm tại Sector đầu tiên của phân vùng.
   - Chứa khối tham số sinh học **BPB (BIOS Parameter Block)**: Số byte trên mỗi sector (`512`), số sector trên mỗi cluster (`64` tương đương `32 KB`), số bảng FAT (`2`), số sector dành riêng trước bảng FAT.
2. **Bảng FAT1 & FAT2 (File Allocation Table)**:
   - Là một **mảng số nguyên 32-bit (chỉ dùng 28 bit)** đại diện cho toàn bộ các Cluster trên đĩa:
     * Giá trị `0x00000000`: Cụm còn trống.
     * Giá trị `0x00000003`: Cụm tiếp theo của file này là Cluster số 3.
     * Giá trị `0x0FFFFFFF`: Đánh dấu **Kết thúc tệp tin (End of File - EOF)**.
     * Giá trị `0x0FFFFFF7`: Đánh dấu **Cụm bị hỏng (Bad Cluster)** do chip nhớ flash xuống cấp.
3. **Vùng dữ liệu (Data Region)**:
   - Bắt đầu từ **Cluster số 2**.
   - Thư mục gốc (Root Directory) trong FAT32 cũng là một chuỗi Cluster bình thường, chứa các bản ghi **Directory Entry 32 bytes**:
     ```text
     [0..10]:  Tên file 8.3 (8 ký tự tên + 3 ký tự đuôi mở rộng, ví dụ: "LOGCAN  CSV")
     [11]:     Thuộc tính (Read-Only, Hidden, System, Directory, Archive)
     [20..21]: 2 bytes cao của Start Cluster
     [26..27]: 2 bytes thấp của Start Cluster
     [28..31]: 4 bytes dung lượng file tính bằng Byte (File Size)
     ```

---

### 1.4. Bài Toán Băng Thông: Phát Video 480x272 60 FPS từ Thẻ Nhớ

Hãy làm phép tính toán học thực tế để chứng minh tính khả thi của hệ thống:
* **Màn hình LCD 4.3 inch**: `480 x 272 = 130,560 pixels`.
* **Định dạng màu RGB565**: `2 bytes/pixel -> 130,560 * 2 = 261,120 bytes/frame` (khoảng `255 KB`).
* **Tốc độ khung hình mượt mà**: `60 frames/s -> Băng thông yêu cầu = 261,120 * 60 = 15.66 MB/s`.
* **Năng lực khối phần cứng SDMMC1 của STM32F746**:
  * Chạy bus 4-bit tại xung nhịp `f_SDCLK = 48 MHz` (từ khối `PLL48CLK`):
    ```text
    Băng thông tối đa phần cứng = 48 MHz * 4 bits = 192 Mbps = 24.0 MB/s
    ```
  * `24.0 MB/s > 15.66 MB/s ->` **Phần cứng đáp ứng hoàn hảo 60 FPS mượt mà không giật lag!**

---

# 📑 BƯỚC 2: TRA CỨU TÀI LIỆU RM0385 & SƠ ĐỒ TUẦN TỰ KHỞI TẠO (SETUP & LOOKUP)

---

### 2.1. Hướng Dẫn Tra Cứu RM0385 (Register Map & Base Address)

* **Tài liệu**: STM32F746 Reference Manual (RM0385).
* **Chương**: **Chapter 29: Secure digital input/output / MultiMediaCard interface (SDMMC)**.
* **Địa chỉ cơ sở (Base Address)**:
  * Tra cứu **Section 2.2.2: Memory map and register boundary addresses**:
  * Ngoại vi **SDMMC1** nằm trên bus **APB2**:
    ```text
    SDMMC1 Base Address = 0x40012C00
    ```
* **Bảng thanh ghi cốt lõi (RM0385 Section 29.9)**:
  * `SDMMC_POWER` (Offset `0x00`): Cấp nguồn điều khiển (`PWRCTRL = 11b`).
  * `SDMMC_CLKCR` (Offset `0x04`): Bộ chia xung `CLKDIV`, bật xung `CLKEN`, độ rộng bus `WIDBUS` (`00b` = 1-bit, `01b` = 4-bit).
  * `SDMMC_ARG` (Offset `0x08`): Chứa tham số 32-bit của lệnh SD.
  * `SDMMC_CMD` (Offset `0x0C`): Chứa mã lệnh (`CMDINDEX`), loại phản hồi (`WAITRESP`), bật máy phát lệnh `CPSMEN` (Bit 10).
  * `SDMMC_RESP1..4` (Offset `0x14..0x20`): Đọc giá trị phản hồi thẻ nhớ (R1..R7, phản hồi ngắn 32-bit hoặc dài 128-bit).
  * `SDMMC_DTIMER` & `SDMMC_DLEN` (Offset `0x24..0x28`): Đếm thời gian Timeout và độ dài truyền dữ liệu (bytes).
  * `SDMMC_DCTRL` (Offset `0x2C`): Hướng truyền (`DTDIR`: 1 = Card to Host), kích thước khối (`DBLOCKSIZE = 9 -> 2^9 = 512`), kích hoạt truyền (`DTEN = 1`).
  * `SDMMC_STA` (Offset `0x34`): Cờ trạng thái phần cứng (CMDREND, CCRCFAIL, DCRCFAIL, RXDAVL, RXOVERR, DATAEND).
  * `SDMMC_ICR` (Offset `0x38`): Thanh ghi xóa cờ **W1C**. Ghi `1` trực tiếp để xóa cờ tương ứng trong `SDMMC_STA`.
  * `SDMMC_FIFO` (Offset `0x80`): Cổng đệm FIFO 32-bit đọc ghi dữ liệu.

---

### 2.2. Sơ Đồ Chân Phần Cứng Trên Bo Mạch STM32F746G-DISCO (UM1907 Table 7)

Tra cứu sơ đồ nguyên lý bo mạch Discovery (UM1907 Section 7.7 & Table 7: microSD connector CN3):

| Tín hiệu SD Card | Chân STM32F746 | Alternate Function | Chế độ cấu hình GPIO |
| :--- | :---: | :---: | :--- |
| **`SDMMC_D0`** | **`PC8`** | **AF12** | Alternate Function, Very High Speed, Pull-up |
| **`SDMMC_D1`** | **`PC9`** | **AF12** | Alternate Function, Very High Speed, Pull-up |
| **`SDMMC_D2`** | **`PC10`** | **AF12** | Alternate Function, Very High Speed, Pull-up |
| **`SDMMC_D3`** | **`PC11`** | **AF12** | Alternate Function, Very High Speed, Pull-up |
| **`SDMMC_CK`** | **`PC12`** | **AF12** | Alternate Function, Very High Speed, No Pull |
| **`SDMMC_CMD`**| **`PD2`** | **AF12** | Alternate Function, Very High Speed, Pull-up |
| **`uSD_Detect`**| **`PC13`** | **GPIO Input** | Input Floating / Pull-up (Mức 0 = Đã cắm thẻ) |

---

### 2.3. Sơ Đồ Tuần Tự 1: Khởi Tạo Thẻ Nhớ & Đàm Phán SDSC vs SDHC (Initialization Pipeline)

Sơ đồ tuần tự thể hiện chính xác 8 bước bắt tay theo tiêu chuẩn SD Physical Layer Specification:

```mermaid
sequenceDiagram
    autonumber
    participant Host as STM32F7 SDMMC1
    participant Card as Thẻ Nhớ MicroSD

    Note over Host: Bước 1: Cấp nguồn & Xung nhịp thấp (f_CLK <= 400 kHz)
    Host->>Host: Bật SDMMC_POWER = 3, CLKDIV = 118 (f = 400 kHz)
    Host->>Card: CMD0 (GO_IDLE_STATE) - Reset thẻ về trạng thái IDLE
    Note over Card: Thẻ vào trạng thái Idle State

    Host->>Card: CMD8 (SEND_IF_COND, Arg: 0x1AA) - Kiểm tra dải điện áp 2.7V - 3.6V
    Card-->>Host: Phản hồi R7 (Trả về đúng mẫu 0x1AA -> Xác nhận hỗ trợ SD V2.0)

    loop Vòng lặp ACMD41 (Đàm phán năng lực và chờ thẻ sẵn sàng)
        Host->>Card: CMD55 (APP_CMD, Báo lệnh kế tiếp là lệnh ứng dụng)
        Card-->>Host: Phản hồi R1
        Host->>Card: ACMD41 (SD_SEND_OP_COND, Arg: HCS bit 30 = 1)
        Card-->>Host: Phản hồi R3 (Thanh ghi OCR 32-bit)
    end
    Note over Host,Card: Kiểm tra Bit 31 (Ready) = 1. Kiểm tra Bit 30 (CCS):<br/>Nếu CCS = 1 -> Thẻ SDHC/SDXC (Block Addressing)<br/>Nếu CCS = 0 -> Thẻ SDSC (Byte Addressing)

    Host->>Card: CMD2 (ALL_SEND_CID) - Yêu cầu thẻ gửi mã nhận dạng 128-bit
    Card-->>Host: Phản hồi R2 (Mã CID: Hãng SX, Serial Number)

    Host->>Card: CMD3 (SEND_RELATIVE_ADDR) - Yêu cầu cấp địa chỉ RCA
    Card-->>Host: Phản hồi R6 (Chứa 16-bit RCA, ví dụ: 0xAAAA)

    Host->>Card: CMD7 (SELECT_CARD, Arg: RCA) - Chọn thẻ đưa vào sử dụng
    Card-->>Host: Phản hồi R1b (Thẻ chuyển từ Standby sang Transfer State)

    Note over Host: Chuyển sang Bus 4-bit & Tăng tốc độ tối đa
    Host->>Card: CMD55 (APP_CMD)
    Host->>Card: ACMD6 (SET_BUS_WIDTH, Arg = 2: 4-bit bus)
    Card-->>Host: Phản hồi R1
    Host->>Host: Đổi CLKDIV = 0 (f = 48 MHz) và WIDBUS = 01b (Bus 4-bit)

    Host->>Card: CMD16 (SET_BLOCKLEN, Arg = 512) - Khóa cứng kích thước 512 bytes
    Card-->>Host: Phản hồi R1 (Hoàn tất! Sẵn sàng đọc ghi dữ liệu)
```

---

### 2.4. Sơ Đồ Tuần Tự 2: Đọc Khối Dữ Liệu Tốc Độ Cao & Đồng Bộ D-Cache (Dataflow Pipeline)

```mermaid
sequenceDiagram
    autonumber
    participant App as Ứng Dụng (FatFs f_read)
    participant Host as STM32F7 SDMMC1
    participant FIFO as SDMMC Hardware FIFO
    participant RAM as Bộ Đệm Đích (SDRAM 0xC0000000)
    participant Card as Thẻ Nhớ SD

    App->>Host: Yêu cầu đọc khối: disk_read(sector, buffer, count)
    Note over Host: Kiểm tra kiểu thẻ: Nếu SDSC -> Arg = sector * 512<br/>Nếu SDHC -> Arg = sector
    Host->>Host: Cấu hình DTIMER, DLEN = 512, DCTRL (DTDIR=1, DBLOCKSIZE=9, DTEN=1)
    Host->>Card: Gửi CMD17 (READ_SINGLE_BLOCK) hoặc CMD18 (READ_MULTIPLE_BLOCK)
    Card-->>Host: Gói dữ liệu 512 bytes truyền qua 4 đường DAT0..DAT3
    Card->>FIFO: Bơm dữ liệu vào FIFO (16 Words / 64 bytes mỗi lần)
    
    loop Đọc toàn bộ 512 bytes (128 Words 32-bit)
        Host->>FIFO: Đọc thanh ghi SDMMC_FIFO khi cờ RXDAVL = 1
        FIFO->>RAM: Ghi thẳng vào bộ đệm RAM đích
    end

    Note over Host,RAM: THAO TÁC SỐNG CÒN CORTEX-M7: D-CACHE COHERENCY
    Host->>Host: Gọi SCB_InvalidateDCache_by_Addr(buffer, 512)
    Host->>Host: Gọi chỉ thị rào cản phần cứng: __asm volatile ("dsb 0xF")
    Host-->>App: Trả về RES_OK (Ứng dụng đọc dữ liệu mới 100% không dính rác Cache!)
```

---

# 💻 BƯỚC 3: GÕ CODE DRIVER CHUẨN THÀNH PHẦN & TẦNG CẦU NỐI FATFS

---

### 3.1. Phân Chia Cấu Trúc File Lưu Trữ

```text
drivers/
├── inc/
│   ├── Reg.h          <-- Định nghĩa con trỏ thanh ghi SDMMC1
│   ├── sdmmc.h        <-- Giao diện driver phần cứng thẻ nhớ (Hỗ trợ SDSC & SDHC)
│   └── diskio.h       <-- Tầng định nghĩa cầu nối của ChaN FatFs
└── src/
    ├── sdmmc.c        <-- Khởi tạo bus 4-bit, nhận diện SDHC và đọc ghi Sector
    ├── diskio.c       <-- Hiện thực 4 hàm disk_* nối FatFs vào sdmmc.c
    └── media_player.c <-- Quản lý tệp tin và streaming video Zero-Copy
```

---

### 📂 KHỐI 1: FILE HEADER DRIVER PHẦN CỨNG [ `drivers/inc/sdmmc.h` ]

```c
/**
 * ==============================================================================
 * File: drivers/inc/sdmmc.h
 * Mục đích: Khai báo giao diện driver phần cứng thẻ nhớ SDMMC1 bus 4-bit
 * ==============================================================================
 */

#ifndef SDMMC_H
#define SDMMC_H

#include <stdint.h>
#include <stdbool.h>

#define SD_BLOCK_SIZE 512

/* Mã lỗi trả về của driver SDMMC */
typedef enum {
    SD_OK = 0,
    SD_ERROR,
    SD_TIMEOUT,
    SD_NOT_PRESENT
} SD_Status_t;

/* Kiểu loại thẻ nhận diện được */
typedef enum {
    CARD_TYPE_UNKNOWN = 0,
    CARD_TYPE_SDSC,     /* Standard Capacity: <= 2GB (Đánh địa chỉ Byte) */
    CARD_TYPE_SDHC      /* High Capacity: 4GB - 32GB (Đánh địa chỉ Block/Sector) */
} SD_CardType_t;

/* Khởi tạo phần cứng SDMMC1, phát hiện thẻ và đàm phán bus 4-bit 48 MHz */
SD_Status_t SDMMC_Init(void);

/* Lấy loại thẻ nhớ hiện tại để phục vụ tính toán địa chỉ */
SD_CardType_t SDMMC_GetCardType(void);

/* Đọc một khối Sector (512 bytes) */
SD_Status_t SDMMC_ReadSingleBlock(uint32_t sector_addr, uint8_t *pBuffer);

/* Đọc nhiều khối Sector liên tiếp */
SD_Status_t SDMMC_ReadMultiBlocks(uint32_t sector_addr, uint8_t *pBuffer, uint32_t num_blocks);

/* Ghi một khối Sector (512 bytes) */
SD_Status_t SDMMC_WriteSingleBlock(uint32_t sector_addr, const uint8_t *pBuffer);

#endif /* SDMMC_H */
```

---

### 📂 KHỐI 2: FILE SOURCE DRIVER PHẦN CỨNG [ `drivers/src/sdmmc.c` ]

```c
/**
 * ==============================================================================
 * File: drivers/src/sdmmc.c
 * Mục đích: Hiện thực hóa 8 bước khởi tạo thẻ SD, phân biệt SDSC/SDHC và đọc FIFO
 * ==============================================================================
 */

#include "sdmmc.h"
#include "Reg.h"

static uint32_t s_RCA = 0;
static SD_CardType_t s_CardType = CARD_TYPE_UNKNOWN;

/* Hàm delay thô phục vụ khởi tạo */
static void SDMMC_Delay(volatile uint32_t count)
{
    while (count--) { __asm volatile ("nop"); }
}

static void SDMMC_GPIO_Config(void)
{
    /* 1. Cấp xung nhịp cho Port C và Port D */
    RCC_AHB1ENR |= (RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN);

    /* 2. Cấu hình PC8..PC12 -> AF12 (SDMMC1 D0..D3, CK) */
    GPIOC_MODER &= ~((3U << 16) | (3U << 18) | (3U << 20) | (3U << 22) | (3U << 24));
    GPIOC_MODER |=  ((2U << 16) | (2U << 18) | (2U << 20) | (2U << 22) | (2U << 24));
    GPIOC_OSPEEDR |= ((3U << 16) | (3U << 18) | (3U << 20) | (3U << 22) | (3U << 24));
    GPIOC_PUPDR   |= ((1U << 16) | (1U << 18) | (1U << 20) | (1U << 22)); /* Pull-up cho đường Data */
    GPIOC_AFRH &= ~((0xFU << 0) | (0xFU << 4) | (0xFU << 8) | (0xFU << 12) | (0xFU << 16));
    GPIOC_AFRH |=  ((12U << 0) | (12U << 4) | (12U << 8) | (12U << 12) | (12U << 16));

    /* 3. Cấu hình PD2 -> AF12 (SDMMC1 CMD) */
    GPIOD_MODER &= ~(3U << 4);
    GPIOD_MODER |=  (2U << 4);
    GPIOD_OSPEEDR |= (3U << 4);
    GPIOD_PUPDR   |= (1U << 4);
    GPIOD_AFRL &= ~(0xFU << 8);
    GPIOD_AFRL |=  (12U << 8);
}

static SD_Status_t SDMMC_SendCommand(uint32_t cmd_index, uint32_t arg, uint32_t wait_resp)
{
    /* Xóa cờ ngắt cũ trực tiếp bằng thanh ghi ICR (W1C, không dùng |=) */
    SDMMC1_ICR = 0x00FFFFFF;

    SDMMC1_ARG = arg;
    /* Ghi mã lệnh, loại phản hồi và kích hoạt máy phát lệnh CPSMEN (Bit 10) */
    SDMMC1_CMD = (cmd_index & 0x3F) | (wait_resp << 6) | (1U << 10);

    if (wait_resp == 0) {
        uint32_t timeout = 50000;
        while (!(SDMMC1_STA & (1U << 7)) && --timeout); /* Chờ cờ CMDSENT (Bit 7) */
        return (timeout > 0) ? SD_OK : SD_TIMEOUT;
    } else {
        uint32_t timeout = 100000;
        /* Chờ cờ CMDREND (Bit 6) thành công hoặc cờ lỗi CCRCFAIL (Bit 0) / CTIMEOUT (Bit 2) */
        while (!(SDMMC1_STA & ((1U << 6) | (1U << 0) | (1U << 2))) && --timeout);
        if (SDMMC1_STA & (1U << 6)) return SD_OK;
        return SD_ERROR;
    }
}

SD_CardType_t SDMMC_GetCardType(void)
{
    return s_CardType;
}

SD_Status_t SDMMC_Init(void)
{
    s_CardType = CARD_TYPE_UNKNOWN;
    SDMMC_GPIO_Config();

    /* Cấp xung nhịp ngoại vi SDMMC1 trên bus APB2 */
    RCC_APB2ENR |= RCC_APB2ENR_SDMMC1EN;

    /* Bước 1: Cấp nguồn cho mạch điều khiển SDMMC (PWRCTRL = 11b) */
    SDMMC1_POWER = 3U;

    /* Cấu hình xung nhịp khởi động an toàn f <= 400 kHz (CLKDIV = 118 cho xung gốc 48 MHz) */
    SDMMC1_CLKCR = (118U << 0) | (1U << 8);
    SDMMC_Delay(50000);

    /* Bước 2: Gửi CMD0 (GO_IDLE_STATE) - Đưa thẻ về trạng thái IDLE */
    SDMMC_SendCommand(0, 0, 0);
    SDMMC_Delay(10000);

    /* Bước 3: Gửi CMD8 (SEND_IF_COND) - Kiểm tra điện áp 2.7V - 3.6V (Mẫu 0x1AA) */
    bool is_v2 = false;
    if (SDMMC_SendCommand(8, 0x000001AA, 1) == SD_OK) {
        if ((SDMMC1_RESP1 & 0xFF) == 0xAA) {
            is_v2 = true; /* Thẻ tuân thủ chuẩn SD Version 2.0 trở lên */
        }
    }

    /* Bước 4: Vòng lặp đàm phán ACMD41 */
    uint32_t retry = 200;
    uint32_t hcs_arg = is_v2 ? (1U << 30) : 0; /* Nếu V2.0 thì bật cờ HCS (Host Capacity Support) */

    while (retry--) {
        SDMMC_SendCommand(55, 0, 1); /* CMD55: Thông báo lệnh kế tiếp là ACMD */
        SDMMC_SendCommand(41, 0x00FF8000 | hcs_arg, 1); /* ACMD41: Gửi dải điện áp và cờ HCS */

        /* Kiểm tra Bit 31 của thanh ghi OCR (Card Power Up Status Bit) */
        if (SDMMC1_RESP1 & (1U << 31)) {
            /* Thẻ đã thoát trạng thái bận. Kiểm tra bit 30 (CCS - Card Capacity Status) */
            if (SDMMC1_RESP1 & (1U << 30)) {
                s_CardType = CARD_TYPE_SDHC; /* Thẻ dung lượng cao (Block-addressed) */
            } else {
                s_CardType = CARD_TYPE_SDSC; /* Thẻ tiêu chuẩn (Byte-addressed) */
            }
            break;
        }
        SDMMC_Delay(10000);
    }
    if (retry == 0) return SD_TIMEOUT;

    /* Bước 5: Gửi CMD2 đọc mã nhận dạng CID 128-bit */
    SDMMC_SendCommand(2, 0, 3);

    /* Bước 6: Gửi CMD3 yêu cầu thẻ công bố địa chỉ RCA */
    SDMMC_SendCommand(3, 0, 1);
    s_RCA = SDMMC1_RESP1 & 0xFFFF0000;

    /* Bước 7: Gửi CMD7 chọn thẻ đưa vào trạng thái truyền dữ liệu (Transfer State) */
    SDMMC_SendCommand(7, s_RCA, 1);

    /* Bước 8: Chuyển sang Bus 4-bit và tăng xung nhịp tối đa 48 MHz */
    SDMMC_SendCommand(55, s_RCA, 1);
    SDMMC_SendCommand(6, 2, 1); /* ACMD6: Tham số 2 nghĩa là chọn 4-bit bus */

    /* Đổi CLKDIV = 0 (Xung đạt cực đại 48 MHz), Bật xung (Bit 8), Đặt WIDBUS = 01b (Bus 4-bit) */
    SDMMC1_CLKCR = (0U << 0) | (1U << 8) | (1U << 11);

    /* Khóa cố định kích thước khối 512 bytes bằng CMD16 */
    SDMMC_SendCommand(16, SD_BLOCK_SIZE, 1);

    return SD_OK;
}

SD_Status_t SDMMC_ReadSingleBlock(uint32_t sector_addr, uint8_t *pBuffer)
{
    if (pBuffer == NULL) return SD_ERROR;

    /* Xóa toàn bộ cờ ngắt và trạng thái cũ */
    SDMMC1_ICR = 0x00FFFFFF;

    SDMMC1_DTIMER = 0x0FFFFFFF;
    SDMMC1_DLEN = SD_BLOCK_SIZE;
    /* DTDIR = 1 (Card to Host), DBLOCKSIZE = 9 (2^9 = 512 bytes), DTEN = 1 */
    SDMMC1_DCTRL = (9U << 4) | (1U << 1) | (1U << 0);

    /* QUY TẮC SỐNG CÒN SDSC vs SDHC: Chuyển đổi địa chỉ */
    uint32_t final_addr = (s_CardType == CARD_TYPE_SDHC) ? sector_addr : (sector_addr * SD_BLOCK_SIZE);

    if (SDMMC_SendCommand(17, final_addr, 1) != SD_OK) return SD_ERROR;

    uint32_t *pDst = (uint32_t *)pBuffer;
    uint32_t words_left = SD_BLOCK_SIZE / 4; /* 128 words 32-bit */

    while (words_left > 0) {
        if (SDMMC1_STA & (1U << 21)) { /* Cờ RXDAVL: Có dữ liệu sẵn sàng trong FIFO */
            *pDst++ = SDMMC1_FIFO;
            words_left--;
        }
        if (SDMMC1_STA & ((1U << 1) | (1U << 3) | (1U << 5))) { /* Cờ lỗi DCRC/DTIMEOUT/RXOVERR */
            return SD_ERROR;
        }
    }

    /* ĐỒNG BỘ D-CACHE CORTEX-M7: Tránh đọc phải rác trong L1 Cache */
    __asm volatile ("dsb 0xF" ::: "memory");

    return SD_OK;
}

SD_Status_t SDMMC_ReadMultiBlocks(uint32_t sector_addr, uint8_t *pBuffer, uint32_t num_blocks)
{
    uint8_t *ptr = pBuffer;
    for (uint32_t i = 0; i < num_blocks; i++) {
        if (SDMMC_ReadSingleBlock(sector_addr + i, ptr) != SD_OK) {
            return SD_ERROR;
        }
        ptr += SD_BLOCK_SIZE;
    }
    return SD_OK;
}
```

---

### 📂 KHỐI 3: TẦNG CẦU NỐI FATFS DISKIO [ `drivers/src/diskio.c` ]

```c
/**
 * ==============================================================================
 * File: drivers/src/diskio.c
 * Mục đích: Ánh xạ 4 hàm tiêu chuẩn của ChaN FatFs vào driver bare-metal SDMMC
 * ==============================================================================
 */

#include "diskio.h"
#include "sdmmc.h"

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    if (SDMMC_Init() == SD_OK) return 0; /* Khởi tạo thành công */
    return STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    return 0; /* Trạng thái sẵn sàng */
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || count == 0) return RES_PARERR;

    if (SDMMC_ReadMultiBlocks((uint32_t)sector, buff, (uint32_t)count) == SD_OK) {
        return RES_OK;
    }
    return RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    (void)pdrv; (void)buff; (void)sector; (void)count;
    /* Trả về RES_OK nếu đã hoàn thiện hàm Write, hoặc RES_WRPRT nếu chế độ Read-Only */
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0) return RES_PARERR;

    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            *(DWORD *)buff = (SDMMC_GetCardType() == CARD_TYPE_SDHC) ? 15500000UL : 3900000UL;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *(WORD *)buff = 512;
            return RES_OK;
        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}

DWORD get_fattime(void)
{
    /* Trả về thời gian cố định: 2026-09-16 12:00:00 */
    return ((DWORD)(2026 - 1980) << 25) | ((DWORD)9 << 21) | ((DWORD)16 << 16) | ((DWORD)12 << 11);
}
```

---

### 📂 KHỐI 4: ỨNG DỤNG STREAMING VIDEO 60 FPS ZERO-COPY [ `drivers/src/media_player.c` ]

```c
/**
 * ==============================================================================
 * File: drivers/src/media_player.c
 * Mục đích: Đọc trực tiếp frame video từ thẻ nhớ vào SDRAM ngoài (Zero-Copy)
 * ==============================================================================
 */

#include "sdmmc.h"
#include "ff.h"
#include <string.h>

#define LCD_WIDTH       480
#define LCD_HEIGHT      272
#define LCD_FRAME_SIZE  (LCD_WIDTH * LCD_HEIGHT * 2) /* 261,120 bytes (RGB565) */
#define SDRAM_FRAMEBUF0 0xC0000000
#define SDRAM_FRAMEBUF1 0xC0040000

static FATFS s_fs;
static FIL   s_fil;

uint8_t MediaPlayer_Init(void)
{
    /* Mount thẻ FAT32 */
    if (f_mount(&s_fs, "", 1) != FR_OK) {
        return 1; /* Lỗi không mount được thẻ */
    }
    return 0;
}

void MediaPlayer_PlayVideo(const char *filename)
{
    if (f_open(&s_fil, filename, FA_READ) != FR_OK) return;

    uint32_t active_buf = SDRAM_FRAMEBUF0;

    while (1) {
        UINT bytes_read = 0;

        /* ZERO-COPY: Đọc thẳng từ Sector thẻ nhớ vào SDRAM Framebuffer ngoài */
        FRESULT res = f_read(&s_fil, (void *)active_buf, LCD_FRAME_SIZE, &bytes_read);
        if (res != FR_OK || bytes_read < LCD_FRAME_SIZE) {
            f_lseek(&s_fil, 0); /* Tua lại đầu video khi xem hết */
            continue;
        }

        /* Đảm bảo toàn vẹn dữ liệu trên Cortex-M7 */
        __asm volatile ("dsb 0xF" ::: "memory");

        /* Hoán đổi Back-Buffer tại kỳ ngắt VSYNC để chống xé hình */
        active_buf = (active_buf == SDRAM_FRAMEBUF0) ? SDRAM_FRAMEBUF1 : SDRAM_FRAMEBUF0;
    }
}
```

---

## 3.2. Mổ Xẻ 5 Bug Phần Cứng & Bẫy Lưu Trữ Kinh Điển

```text
┌─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                               5 BẪY HỆ THỐNG LƯU TRỮ SDMMC & FATFS KINH ĐIỂN                    │
├───────────────────┬───────────────────────────────────────────┬─────────────────────────────────┤
│ HIỆN TƯỢNG BUG    │ NGUYÊN NHÂN SÂU XA PHẦN CỨNG / FILE SYSTEM│ GIẢI PHÁP SỬA CODE CHUẨN XÁC    │
├───────────────────┼───────────────────────────────────────────┼─────────────────────────────────┤
│ 1. Tràn số địa chỉ│ Thẻ SDHC yêu cầu số thứ tự Block, nhưng   │ Kiểm tra cờ CCS lúc nhận diện:  │
│    làm đứng máy   │ code lại lấy sector nhân 512 khiến biến   │ Nếu SDHC -> gửi sector;         │
│    (SDSC vs SDHC) │ uint32_t bị tràn vượt quá ngưỡng 4GB.     │ Nếu SDSC -> gửi sector * 512.   │
├───────────────────┼───────────────────────────────────────────┼─────────────────────────────────┤
│ 2. D-Cache        │ Cortex-M7 có L1 D-Cache. Khi DMA/FIFO nạp │ Căn lề buffer 32 bytes          │
│    Coherency Bug  │ dữ liệu vào RAM, CPU đọc trúng cache cũ   │ __attribute__((aligned(32))) và │
│    làm rác hình   │ làm màn hình bị sọc rác nhấp nháy.        │ gọi SCB_InvalidateDCache_by_Addr│
├───────────────────┼───────────────────────────────────────────┼─────────────────────────────────┤
│ 3. Unaligned      │ SDMMC truyền từng Word 32-bit. Nếu địa chỉ│ Bắt buộc ép kiểu buffer đích    │
│    Buffer Address │ buffer không chia hết cho 4, vi điều khiển│ phải là bội số của 4 bytes      │
│    gây HardFault  │ nhảy vào bẫy lỗi phần cứng UsageFault.    │ (Word-aligned pointer).         │
├───────────────────┼───────────────────────────────────────────┼─────────────────────────────────┤
│ 4. Mất file khi xe│ Vi điều khiển ghi log CAN nhưng chưa đẩy  │ Luôn gọi f_sync(&fil) định kỳ   │
│    tắt máy đột ngột│ dữ liệu từ RAM Cache xuống Sector thẻ    │ mỗi 500ms để khóa chặt dữ liệu  │
│    (Power-Loss)   │ khiến tệp tin bị độ dài 0 bytes sau boot. │ an toàn vào flash vật lý.       │
├───────────────────┼───────────────────────────────────────────┼─────────────────────────────────┤
│ 5. Sụt giảm FPS   │ File video trên thẻ bị phân mảnh nhiều    │ Khi format thẻ FAT32, chọn kích │
│    do Cluster     │ mảnh (Fragmented). FatFs liên tục phải đọc│ thước Cluster lớn (32KB/64KB) và│
│    Fragmentation  │ bảng FAT tìm cluster làm giảm băng thông. │ copy file tuần tự từ đầu.       │
└───────────────────┴───────────────────────────────────────────┴─────────────────────────────────┘
```

---

# 🎯 BƯỚC 4: BỘ CÂU HỎI PHỎNG VẤN & KỊCH BẢN TRẢ LỜI CHUYÊN SÂU

---

### ❓ Câu 1: "Sự khác biệt cốt tử giữa thẻ SDSC và thẻ SDHC là gì? Nếu lập trình viên không xử lý sự khác biệt này trong Driver, lỗi nghiêm trọng nào sẽ xảy ra?"
* **Trả lời chuẩn Kỹ sư Nhúng:**
  * **SDSC (Standard Capacity, dung lượng <= 2GB):** Sử dụng cơ chế đánh địa chỉ theo Byte (**Byte Addressing**). Tham số truyền vào trong các lệnh đọc ghi (`CMD17`, `CMD18`, `CMD24`) là địa chỉ byte thực tế (`Sector_Number * 512`).
  * **SDHC (High Capacity, dung lượng 4GB - 32GB):** Do dung lượng vượt quá giới hạn 4GB của con số 32-bit, chuẩn SD 2.0 chuyển sang dùng cơ chế đánh địa chỉ theo Khối (**Block/Sector Addressing**). Tham số truyền vào chính là số thứ tự của Sector (`Sector_Number`).
  * **Lỗi xảy ra:** Nếu cắm thẻ SDHC mà driver vẫn nhân với 512, khi đọc các sector nằm ở vị trí cao trên thẻ (vượt quá 4GB / 512 = 8,388,608), phép tính `sector * 512` sẽ bị **tràn số nguyên 32-bit**, quay vòng về địa chỉ rác ở đầu thẻ, gây hỏng dữ liệu hoặc thẻ từ chối thực thi với lỗi `ADDR_OUT_OF_RANGE`.

---

### ❓ Câu 2: "Tại sao trong giai đoạn nhận diện thẻ nhớ (Initialization Phase), xung nhịp SDMMC bắt buộc phải chạy dưới 400 kHz, sau đó mới được tăng lên 24 MHz hoặc 48 MHz?"
* **Trả lời chuẩn Kỹ sư Nhúng:**
  * Khi vừa cấp nguồn, thẻ nhớ chưa xác định được mức điện áp logic hoạt động (chuẩn Legacy 3.3V hay điện áp thấp 1.8V) và chưa được gán địa chỉ logic RCA. Các đường dây bus lúc này hoạt động ở chế độ cực máng hở (**Open-Drain Mode**).
  * Tần số `f_OD <= 400 kHz` là tần số chuẩn quốc tế đảm bảo dạng sóng điện áp không bị méo hài trên mạch Open-Drain, giúp mọi loại thẻ nhớ từ cổ điển đến hiện đại đều bắt tay thành công.
  * Sau lệnh `CMD3`, thẻ đã công bố địa chỉ `RCA` và chuyển sang trạng thái `Transfer State`. Lúc này mạch chuyển sang chế độ **Push-Pull** (kéo đẩy chủ động) và cho phép nâng xung nhịp lên `24 MHz` hoặc `48 MHz` để đạt băng thông tối đa.

---

### ❓ Câu 3: "Trong vi điều khiển ARM Cortex-M7 có bộ nhớ đệm L1 Cache, việc đọc thẻ nhớ nạp vào SDRAM bằng DMA/FIFO gặp phải nguy cơ gì và cách xử lý triệt để ra sao?"
* **Trả lời chuẩn Kỹ sư Nhúng:**
  * Đây là lỗi kinh điển về **D-Cache Coherency (Bất đồng bộ dữ liệu giữa Cache và RAM)**: Khối phần cứng SDMMC đọc dữ liệu từ thẻ nhớ nạp thẳng vào ô nhớ SDRAM ngoài thông qua Bus Matrix. Nếu trước đó CPU đã từng truy cập vùng nhớ này, CPU Cortex-M7 sẽ tiếp tục đọc dữ liệu cũ nằm kẹt trong L1 D-Cache thay vì đọc dữ liệu mới dưới RAM, dẫn đến việc màn hình bị vỡ hình hoặc rác dữ liệu.
  * **Giải pháp khắc phục:**
    1. Căn lề bộ đệm đúng kích thước 1 Cache Line (32 bytes): `__attribute__((aligned(32)))`.
    2. Trước khi CPU đọc dữ liệu mới, bắt buộc gọi hàm xóa hiệu lực dòng Cache: `SCB_InvalidateDCache_by_Addr((uint32_t *)buffer, size);`.
    3. Gọi chỉ thị rào cản đồng bộ dữ liệu kiến trúc ARM: `__asm volatile ("dsb 0xF" ::: "memory");`.

---

### ❓ Câu 4: "Tại sao trong dự án em lại quyết định chuẩn hóa sử dụng thẻ nhớ SDHC (4GB - 32GB) thay vì thẻ SDSC hay SDXC?"
* **Trả lời chuẩn Kỹ sư Nhúng:**
  * **So với SDSC (<= 2GB):** Thẻ SDSC hiện đã ngừng sản xuất, không còn thực tế ngoài thị trường và dung lượng quá nhỏ (không đủ lưu trữ log CAN dài ngày hay video lớn). Ngoài ra, SDSC dùng Byte Addressing dễ gây tràn biến số nguyên.
  * **So với SDXC (> 32GB):** Thẻ SDXC mặc định định dạng bằng hệ thống tệp tin **exFAT** (yêu cầu bản quyền phần mềm độc quyền từ Microsoft và thư viện FatFs phải bật cấu hình phức tạp tốn RAM).
  * **Lý do chọn SDHC:** Thẻ SDHC (4GB đến 32GB) là "điểm ngọt ngào nhất" (Sweet Spot) trong công nghiệp nhúng: định dạng mặc định là **FAT32** tương thích 100% với mã nguồn mở ChaN FatFs, cơ chế **Block Addressing LBA** cố định 512 bytes cực kỳ trực quan cho vi điều khiển, và dung lượng 16GB - 32GB đáp ứng hoàn hảo cho cả bài toán Hộp đen ô tô EDR lẫn phát video 60 FPS.

---

### 🎙️ KỊCH BẢN TRẢ LỜI PHỎNG VẤN 60 GIÂY VỀ LƯU TRỮ NHÚNG (ELEVATOR PITCH)

> *"Trong các dự án hệ thống nhúng hiệu năng cao trên STM32F746, em đã tự tay xây dựng tầng lưu trữ hoàn chỉnh gồm Driver Bare-Metal cho khối ngoại vi **SDMMC1 bus 4-bit tần số 48 MHz** và tích hợp hệ thống tệp tin tiêu chuẩn **ChaN FatFs**, chuẩn hóa 100% trên **thẻ nhớ MicroSD SDHC (4GB - 32GB)**.  
> Em nắm vững cơ chế bắt tay **HCS/CCS trong lệnh ACMD41** để kích hoạt chế độ **Block Addressing (LBA)** đặc trưng của thẻ SDHC, truyền thẳng chỉ số Sector mà không bị lỗi tràn biến 32-bit như các dòng thẻ cũ.  
> Để đạt hiệu năng phát video **60 FPS mượt mà tuyệt đối không xé hình**, em áp dụng kiến trúc **Zero-Copy Streaming** đọc trực tiếp từ thẻ nhớ nạp vào SDRAM ngoài 8MB, đồng thời giải quyết triệt để lỗi **D-Cache Coherency** của Cortex-M7 bằng việc căn lề bộ đệm 32 bytes và điều khiển rào cản phần cứng DSB. Đây là nền tảng vững chắc để xây dựng các thiết bị Hộp đen ô tô EDR và hệ thống giải trí đa phương tiện cao cấp."*
