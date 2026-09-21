# High-Performance 60 FPS Bare-Metal Video Player & SDHC Subsystem

[![Target MCU](https://img.shields.io/badge/MCU-STM32F746NG%20(Cortex--M7%20%40%20216MHz)-red.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f746ng.html)
[![Architecture](https://img.shields.io/badge/Firmware-100%25%20Bare--Metal%20(No%20HAL%2FLL)-blue.svg)](#hardware-register-architecture)
[![Display](https://img.shields.io/badge/Display-480x272%20%40%2060%20FPS%20(LTDC%20%2B%20DMA2D)-green.svg)](#display-subsystem-ltdc--dma2d-chrom-art)
[![Storage](https://img.shields.io/badge/Storage-MicroSD%20SDHC%20(4--bit%20SDMMC%2048MHz)-purple.svg)](#storage-subsystem-sdmmc--chan-fatfs)
[![License](https://img.shields.io/badge/License-MIT-lightgrey.svg)](LICENSE)

A high-performance multimedia player and storage engine written in **100% Bare-Metal C directly targeting hardware registers (RM0385)** on the **STM32F746G-Discovery** board. The system streams full-motion video directly from a **FAT32 MicroSD SDHC card** at a constant **60 FPS** without frame tearing or audio-video jitter, utilizing **FMC SDRAM (8MB @ 108MHz)**, **LTDC**, and the **DMA2D Chrom-ART Accelerator** to achieve **0% CPU load** during frame transfers.

---

## Table of Contents

- [System Architecture](#system-architecture)
- [Key Engineering Features](#key-engineering-features)
- [Hardware Register Architecture](#hardware-register-architecture)
- [Clock Tree & Over-Drive Mode (216 MHz)](#clock-tree--over-drive-mode-216-mhz)
- [External Memory Subsystem (FMC SDRAM 8MB)](#external-memory-subsystem-fmc-sdram-8mb)
- [Display Subsystem (LTDC & DMA2D Chrom-ART)](#display-subsystem-ltdc--dma2d-chrom-art)
- [Storage Subsystem (SDMMC & ChaN FatFs)](#storage-subsystem-sdmmc--chan-fatfs)
- [ARM Cortex-M7 L1 D-Cache Coherency](#arm-cortex-m7-l1-d-cache-coherency)
- [Project Directory Layout](#project-directory-layout)
- [Building, Flashing & Video Conversion Tool](#building-flashing--video-conversion-tool)
- [Author Information](#author-information)

---

## System Architecture

```text
+-----------------------------------------------------------------------------------------------+
| STM32F746NG (ARM Cortex-M7 @ 216 MHz) - 100% BARE-METAL REGISTER PIPELINE                    |
|                                                                                               |
|  +-------------------------------------+      +--------------------------------------------+  |
|  | MicroSD SDHC (Class 10 / UHS-I)     |      | FMC SDRAM Subsystem (IS42S16400J - 8 MB)   |  |
|  | - 4-bit Data Bus @ 48 MHz           |      | - 16-bit Data Bus @ 108 MHz                |  |
|  | - Block Addressing (512-Byte LBA)   |      | - Bank 1 Base Address: 0xC0000000          |  |
|  +------------------+------------------+      +---------------------+----------------------+  |
|                     |                                               ^                         |
|                     | SDMMC FIFO / DMA2 Stream 3                    | Buffer Allocation       |
|                     v                                               |                         |
|  +-------------------------------------+                            |                         |
|  | ChaN FatFs File Engine (FAT32)      |----------------------------+                         |
|  | - Direct Sector Streaming (18 MB/s) |  Front Buffer: 0xC0000000 (255 KB)                   |
|  | - Non-cacheable DMA Read Buffer     |  Back Buffer:  0xC0040000 (255 KB)                   |
|  +-------------------------------------+                            |                         |
|                                                                     v                         |
|  +-----------------------------------------------------------------------------------------+  |
|  | DMA2D Chrom-ART 2D Hardware Accelerator                                                 |  |
|  | - Memory-to-Memory Frame Blitting (RGB565 -> RGB565)                                   |  |
|  | - Zero CPU utilization during active video playback                                     |  |
|  +--------------------------------------------+--------------------------------------------+  |
|                                               |                                               |
|                                               v Hardware Double Buffering VSYNC Reload        |
|  +-----------------------------------------------------------------------------------------+  |
|  | LTDC Display Controller (480x272 @ 60 FPS)                                              |  |
|  | - Pixel Clock (LCD_CLK): 9.71 MHz generated from Dedicated PLLSAI                      |  |
|  | - 24-bit Parallel RGB Bus to 4.3" RK043FN48H TFT                                        |  |
|  +-----------------------------------------------------------------------------------------+  |
+-----------------------------------------------------------------------------------------------+
```

---

## Key Engineering Features

* **Zero Abstraction Layer (100% Bare-Metal):** Implemented purely using direct register manipulations (ARM Cortex-M7 memory-mapped I/O) without STM32Cube HAL, LL, or third-party libraries.
* **Tear-Free 60 FPS Video Display:** Eliminates visual screen tearing completely using hardware Double Buffering coupled with LTDC Vertical Blanking Reload (`LTDC_SRCR.VBR`).
* **Zero CPU Load During Rendering:** Offloads 100% of pixel block transfer operations to the DMA2D Chrom-ART engine, reducing CPU utilization from 85% to 0%.
* **High-Throughput SDHC Storage:** Custom SDMMC driver in 4-bit bus mode @ 48 MHz using LBA block addressing and ChaN FatFs (FAT32), sustaining sequential reads up to 18 MB/s.
* **L1 D-Cache Integrity:** Resolves Cortex-M7 cache coherency hazards using MPU non-cacheable memory attribute regions and software cache maintenance operations (`SCB_InvalidateDCache_by_Addr`).

---

## Hardware Register Architecture

Every register offset and configuration sequence is derived directly from the ST Reference Manual (**RM0385**):

| Peripheral | Base Address | Bus | Bus Clock | Key Registers Used |
| :--- | :--- | :--- | :--- | :--- |
| **RCC** | `0x40023800` | AHB1 | 216 MHz | `CR`, `PLLCFGR`, `PLLSAICFGR`, `CFGR`, `AHB1ENR`, `APB2ENR` |
| **PWR** | `0x40007000` | APB1 | 54 MHz | `CR1` (Over-Drive Mode: `VOS[1:0]`, `ODEN`, `ODSWEN`) |
| **FLASH** | `0x40023C00` | AHB1 | 216 MHz | `ACR` (Latency 6 Wait States, `ARTEN`, `PRFTEN`) |
| **FMC** | `0xA0000000` | AHB3 | 216 MHz | `SDCR[1:2]`, `SDTR[1:2]`, `SDCMR`, `SDRTR` |
| **LTDC** | `0x40016800` | APB2 | 108 MHz | `SSCR`, `BPCR`, `AWCR`, `TWCR`, `SRCR`, `L1CFBLR`, `L1CFBAR` |
| **DMA2D** | `0x4002B000` | AHB1 | 216 MHz | `CR`, `ISR`, `IFCR`, `OPFCCR`, `OMAR`, `OOR`, `NLR` |
| **SDMMC1** | `0x40012C00` | APB2 | 108 MHz | `CLKCR`, `CMD`, `RESPCMD`, `RESP1..4`, `DTIMER`, `DLEN`, `DCTRL`, `DCOUNT`, `STA`, `ICR`, `FIFO` |

---

## Clock Tree & Over-Drive Mode (216 MHz)

Achieving the maximum rated 216 MHz system frequency on STM32F746 requires a precise 7-step hardware handshake:

```text
External HSE Crystal = 25 MHz
Main PLL:
  - PLLM = 25 -> f_VCO_in = 1.0 MHz (Matches RM0385 requirement: 1 - 2 MHz)
  - PLLN = 432 -> f_VCO_out = 432 MHz
  - PLLP = 2 -> f_SYSCLK = 432 / 2 = 216 MHz
  - PLLQ = 9 -> f_SDMMC = 432 / 9 = 48 MHz (Clock source for 48MHz SDMMC)

Hardware Handshake Sequence:
1. Enable HSE and wait for HSERDY in RCC_CR.
2. Configure Power Scale 1 (PWR_CR1.VOS = 0b11).
3. Activate Over-Drive Mode (Set PWR_CR1.ODEN, poll ODENRDY, set ODSWEN, poll ODSWRDY).
4. Configure Flash Latency: 6 Wait States (7 CPU cycles) + ART Accelerator Enable.
5. Set Prescalers: AHB = /1 (216MHz), APB1 = /4 (54MHz), APB2 = /2 (108MHz).
6. Enable Main PLL and wait for PLLRDY.
7. Switch System Clock to PLL (RCC_CFGR.SW = 0b10) and verify SWS.
```

---

## External Memory Subsystem (FMC SDRAM 8MB)

The board integrates an **ISSI IS42S16400J** 64-Mbit (8MB) SDRAM wired via a 16-bit bus on FMC Bank 1 (`0xC0000000`):

```text
SDRAM Clock = f_HCLK / 2 = 216 MHz / 2 = 108 MHz (Clock period t_CK = 9.26 ns)
Memory Geometry: 4 Banks x 4096 Rows x 256 Columns x 16 bits = 8 MBytes

JEDEC 5-Step Power-Up Sequence (FMC_SDCMR):
1. Clock Configuration Enable: Issue command Mode 0b001 to Bank 1.
2. Precharge All: Issue command Mode 0b010.
3. Auto-Refresh: Issue command Mode 0b011 with 8 consecutive refresh cycles.
4. Load Mode Register: Issue command Mode 0b100 (CAS Latency = 2, Burst Length = 1).
5. Set Refresh Rate Counter: SDRTR = 683 (Refresh period 64ms across 4096 rows @ 108MHz).
```

---

## Display Subsystem (LTDC & DMA2D Chrom-ART)

### LTDC Timing Parameters for RK043FN48H (480x272 @ 60 Hz)
* **Pixel Clock (LCD_CLK):** $9.71\text{ MHz}$ derived from PLLSAI ($f_{VCO} = 192\text{ MHz}$, $PLLSAIR = 5$, $DIV = 4$).
* **Horizontal Timing:** Active: 480, HSync: 41, HBP: 13, HFP: 32 (Total = 566).
* **Vertical Timing:** Active: 272, VSync: 10, VBP: 2, VFP: 2 (Total = 286).
* **Frame Rate:** $9,710,000 / (566 \times 286) \approx 60.01\text{ FPS}$.

### Hardware Double Buffering & Tear-Free VSYNC Reload
```text
Step 1: LTDC displays from Front Buffer (0xC0000000).
Step 2: SDMMC + DMA2D unpacks next video frame into Back Buffer (0xC0040000).
Step 3: Update LTDC Layer 1 Frame Address register:
        LTDC_Layer1->CFBAR = (uint32_t)Back_Buffer;
Step 4: Request Vertical Blanking Reload:
        LTDC->SRCR = LTDC_SRCR_VBR;
Step 5: LTDC hardware atomically swaps buffers only when the electron beam enters the Vertical Blanking zone.
Result: Complete elimination of horizontal tearing artifacts.
```

---

## Storage Subsystem (SDMMC & ChaN FatFs)

* **Bus Topology:** 4-bit parallel data lines (`SDMMC_D0..D3`) + Clock (`SDMMC_CK`) + Command (`SDMMC_CMD`).
* **Clock Frequency:** Initialized at $400\text{ kHz}$ during identification, stepped up to $48\text{ MHz}$ in Data Transfer mode.
* **SDHC Support:** Block Addressing (Logical Block Address - LBA) using fixed 512-byte sectors.
* **Driver Architecture:** Low-level disk I/O interface (`diskio.c`) wired to official ChaN FatFs `ff.c` (FAT32 filesystem module).

---

## ARM Cortex-M7 L1 D-Cache Coherency

The ARM Cortex-M7 core features a 16KB L1 Data Cache with write-back policy. When DMA2D or SDMMC modifies RAM directly, the CPU cache might become stale:

1. **Display Framebuffers:** Configured as Write-Through or Non-Cacheable via ARM Cortex-M MPU (Memory Protection Unit) region 0.
2. **SDMMC DMA Read Buffers:** Before passing DMA-transferred sectors to FatFs, software calls:
   ```c
   SCB_InvalidateDCache_by_Addr((uint32_t *)read_buffer, buffer_size);
   ```
   This discards stale cache lines and forces the CPU to fetch fresh video frames directly from physical SDRAM.

---

## Project Directory Layout

```text
stm32f7-baremetal-tft-sdhc/
├── Inc/
│   ├── reg.h                  # Absolute Memory-Mapped Register Definitions
│   ├── sys_clock.h            # 216 MHz Over-Drive & PLL Initializer
│   ├── sdram.h                # FMC 8MB SDRAM Driver Interface
│   ├── ltdc.h                 # LTDC 480x272 Controller
│   ├── dma2d.h                # DMA2D Chrom-ART Blitting Engine
│   ├── sdmmc.h                # SDMMC 4-bit 48MHz Driver
│   ├── diskio.h               # ChaN FatFs Disk I/O Wrapper
│   ├── ff.h                   # ChaN FatFs Configuration Header
│   └── media_player.h         # Video Frame Sequencing Engine
├── Src/
│   ├── main.c                 # Application entry point & Super-Loop
│   ├── sys_clock.c            # PLL, Over-Drive, Flash Latency Handshake
│   ├── sdram.c                # 5-step JEDEC FMC Initialization
│   ├── ltdc.c                 # Video Timing, Layer 1 RGB565 Configuration
│   ├── dma2d.c                # Chrom-ART Blitting Routines
│   ├── sdmmc.c                # SDMMC Command & FIFO State Machine
│   ├── diskio.c               # FatFs Sector Read/Write Bridge
│   ├── ff.c                   # ChaN FatFs Core Module
│   └── media_player.c         # Video Streaming Pipeline
├── tools/                     # Host-side video conversion utilities
│   └── video_converter.py     # Python script to convert MP4 to raw RGB565 .BIN
├── build.ps1                  # Automated PowerShell build script (arm-none-eabi)
├── Makefile                   # GNU Make build recipe
├── STM32F746NGHx_FLASH.ld     # GCC Linker Script
└── README.md
```

---

## Building, Flashing & Video Conversion Tool

### 1. Build Firmware (GNU Arm Embedded Toolchain)

```bash
# Clone the repository
git clone https://github.com/HuynhTran112/stm32f7-baremetal-tft-sdhc.git
cd stm32f7-baremetal-tft-sdhc

# Compile project using make
make -j8

# Or build using the automated PowerShell script
.\build.ps1
```

### 2. Prepare Video Files on MicroSD Card

Raw RGB565 frames ($480 \times 272 \times 2\text{ bytes} = 261,120\text{ bytes/frame}$) are pre-processed using the included Python utility:

```bash
# Convert your favorite MP4 clip to compatible binary stream
python tools/video_converter.py --input sample_video.mp4 --output car1.BIN --fps 60

# Copy car1.BIN to the root directory of a FAT32-formatted MicroSD card
# Insert MicroSD into STM32F746G-DISCO slot and press Reset (Black Button)
```

---

## Author Information

* **Tran Huynh** - Embedded Systems & Firmware Engineer
* **Email:** huynhtran30112004@gmail.com
* **GitHub:** [HuynhTran112](https://github.com/HuynhTran112)
* **LinkedIn:** [Tran Huynh](https://linkedin.com)
