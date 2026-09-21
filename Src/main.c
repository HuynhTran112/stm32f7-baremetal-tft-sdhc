/**
 * @file main.c
 * @brief Điểm vào chính của hệ thống Bare-Metal Media Streamer quản lý tệp tin FAT32 trên STM32F746G-DISCO
 */

#include "reg.h"
#include "sys_clock.h"
#include "sdram.h"
#include "ltdc.h"
#include "dma2d.h"
#include "sdmmc.h"
#include "media_player.h"

int main(void)
{
    /* 0. Bật sáng Green LED1 (PI1) ngay lập tức báo hiệu chip đang chạy */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOIEN;
    (void)RCC->AHB1ENR;
    GPIOI->MODER &= ~((3U << 22) | (3U << 2));
    GPIOI->MODER |=  (1U << 2);  /* PI1 = Output */
    GPIOI->PUPDR &= ~(3U << 22);
    GPIOI->PUPDR |=  (2U << 22); /* PI11 = Pull-Down (chong nhieu) */
    GPIOI->BSRR   =  (1U << 1);  /* Sáng Green LED */

    /* 1. Khởi tạo xung nhịp 216 MHz */
    SysClock_Init();

    /* 2. Cấu hình chân User Button (PI11) và User LED1 (PI1) */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOIEN;
    (void)RCC->AHB1ENR;
    GPIOI->MODER &= ~((3U << 22) | (3U << 2));
    GPIOI->MODER |=  (1U << 2);  /* PI1 = Output (Green LED) */
    GPIOI->PUPDR &= ~(3U << 22);
    GPIOI->PUPDR |=  (2U << 22); /* PI11 = Pull-Down (chong nhieu) */
    GPIOI->BSRR   =  (1U << 1);  /* Bật sáng Green LED */

    /* 3. Khởi tạo bộ nhớ ngoài FMC SDRAM 8 MB */
    SDRAM_Init();
    if (!SDRAM_Test())
    {
        /* Nếu SDRAM test fail: nháy LED nhanh */
        while (1) { GPIOI->ODR ^= (1U << 1); Delay_ms(100); }
    }

    /* 4. Khởi tạo bộ tăng tốc đồ họa DMA2D */
    DMA2D_Init();

    /* 5. Khởi tạo bộ điều khiển hiển thị LTDC 480x272 @ 60 FPS, bật LCD_DISP và LCD_BL_CTRL */
    LTDC_Init();

    /* Vòng lặp chính quản lý phát Video và Menu */
    while (1)
    {
        /* 6. Hiển thị màn hình khởi động (Splash Screen) */
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_DARKBLUE);
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 40, 50, 400, 160, COLOR_NAVY);
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 40, 50, 400, 3, COLOR_CYAN);
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 40, 207, 400, 3, COLOR_CYAN);
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 40, 50, 3, 160, COLOR_CYAN);
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 437, 50, 3, 160, COLOR_CYAN);

        /* Dải chỉ thị màu sắc */
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 60, 70, 70, 20, COLOR_RED);
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 140, 70, 70, 20, COLOR_GREEN);
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 220, 70, 70, 20, COLOR_BLUE);
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 300, 70, 120, 20, COLOR_YELLOW);

        /* Thanh tiến trình khởi động: Màu đen nền */
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 60, 140, 360, 26, COLOR_BLACK);
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 62, 142, 100, 22, COLOR_CYAN);
        Delay_ms(200);

        /* 7. Khởi tạo và Mount hệ thống tệp tin FAT32 từ thẻ MicroSD */
        uint8_t fat_status = MediaPlayer_Init_FAT();
        if (fat_status == 0)
        {
            /* Mount thẻ nhớ thành công: Đẩy thanh tiến trình xanh */
            DMA2D_FillRect(LCD_FRAMEBUFFER_0, 62, 142, 356, 22, COLOR_GREEN);
            Delay_ms(250);

            /* 8. Quét danh sách các file video (.BIN / .RAW) trong thư mục gốc */
            VideoFileInfo_t video_list[MAX_VIDEO_FILES];
            uint8_t video_count = MediaPlayer_ScanVideos(video_list, MAX_VIDEO_FILES);

            if (video_count > 0)
            {
                while (1)
                {
                    /* Hiển thị Menu chọn file video trên LCD */
                    uint8_t chosen_idx = MediaPlayer_SelectVideoUI(video_list, video_count);

                    /* Phát video được chọn */
                    MediaPlayer_PlayFile(video_list[chosen_idx].filename);

                    /* Khi nhấn User Button để dừng video, vòng lặp while(1) này sẽ
                     * tự động mở lại Menu chọn file ngay lập tức! */
                }
            }
            else
            {
                /* Thẻ nhớ nhận diện OK nhưng không có file video .BIN / .RAW:
                 * Báo vàng và chuyển sang Demo
                 */
                DMA2D_FillRect(LCD_FRAMEBUFFER_0, 62, 142, 356, 22, COLOR_YELLOW);
                Delay_ms(1000);
                MediaPlayer_RunGraphicsDemo();
            }
        }
        else
        {
            /* Chưa cắm thẻ nhớ hoặc lỗi thẻ: Báo đỏ và chạy demo */
            DMA2D_FillRect(LCD_FRAMEBUFFER_0, 62, 142, 356, 22, COLOR_RED);
            Delay_ms(600);
            MediaPlayer_RunGraphicsDemo();
        }

        Delay_ms(300);
    }

    return 0;
}
