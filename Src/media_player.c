/**
 * @file media_player.c
 * @brief Triển khai phát Video 60 FPS từ FAT32, hiển thị FPS và Menu chọn file trực quan
 */

#include "media_player.h"
#include "sdram.h"
#include "ltdc.h"
#include "dma2d.h"
#include "sys_clock.h"
#include "reg.h"
#include "font8x8.h"
#include <string.h>

static FATFS s_fs;
static FIL   s_fil;
static uint8_t s_current_buffer_idx = 0;

static void uint_to_str(uint32_t val, char *buf)
{
    char tmp[12];
    int i = 0;
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    while (val > 0)
    {
        tmp[i++] = (val % 10) + '0';
        val /= 10;
    }
    int j = 0;
    while (i > 0)
    {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

void LCD_DrawChar(uint32_t fb_addr, uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg)
{
    if (c < 32 || c > 126) c = '?';
    volatile uint16_t *fb = (volatile uint16_t *)fb_addr;
    const uint8_t *bitmap = Font8x8[c - 32];

    for (uint8_t row = 0; row < 8; row++)
    {
        uint16_t py = y + row;
        if (py >= LCD_HEIGHT) break;
        uint8_t line = bitmap[row];

        for (uint8_t col = 0; col < 8; col++)
        {
            uint16_t px = x + col;
            if (px >= LCD_WIDTH) break;

            if (line & (0x80 >> col))
            {
                fb[py * LCD_WIDTH + px] = fg;
            }
            else if (bg != 0x0001) /* 0x0001 = Trong suốt */
            {
                fb[py * LCD_WIDTH + px] = bg;
            }
        }
    }
}

void LCD_DrawString(uint32_t fb_addr, uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg)
{
    uint16_t cur_x = x;
    while (*str)
    {
        if (cur_x + 8 > LCD_WIDTH) break;
        LCD_DrawChar(fb_addr, cur_x, y, *str, fg, bg);
        cur_x += 8;
        str++;
    }
}

uint8_t MediaPlayer_Init_FAT(void)
{
    /* 1. Xóa sạch 2 Framebuffer trong SDRAM */
    LCD_Clear(LCD_FRAMEBUFFER_0, COLOR_DARKBLUE);
    LCD_Clear(LCD_FRAMEBUFFER_1, COLOR_DARKBLUE);
    LTDC_SetAddress(LCD_FRAMEBUFFER_0);
    s_current_buffer_idx = 0;

    /* 2. Cấu hình chân nút nhấn User Button (PI11) */
    RCC->AHB1ENR |= (1U << 8);
    (void)RCC->AHB1ENR; /* GPIOIEN */
    GPIOI->MODER &= ~(3U << (11 * 2)); /* Input mode */
    GPIOI->PUPDR &= ~(3U << (11 * 2));
    GPIOI->PUPDR |=  (2U << (11 * 2)); /* Pull-Down chong nhieu */

    /* 3. Mount hệ thống tệp tin FAT32 (Drive 0, Mount now = 1) */
    FRESULT res = f_mount(&s_fs, "", 1);
    if (res == FR_OK)
    {
        return 0; /* Thành công */
    }
    return (uint8_t)res; /* Lỗi Mount thẻ */
}

uint8_t MediaPlayer_ScanVideos(VideoFileInfo_t *file_list, uint8_t max_files)
{
    DIR dir;
    FILINFO fno;
    uint8_t count = 0;

    FRESULT res = f_opendir(&dir, "/");
    if (res != FR_OK) return 0;

    while (count < max_files)
    {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break; /* Hết file */

        if (fno.fattrib & (AM_DIR | AM_HID | AM_SYS)) continue;

        /* Kiểm tra đuôi file .BIN hoặc .RAW */
        char *dot = strrchr(fno.fname, '.');
        if (dot != NULL)
        {
            if (strcasecmp(dot, ".BIN") == 0 || strcasecmp(dot, ".RAW") == 0)
            {
                strncpy(file_list[count].filename, fno.fname, 15);
                file_list[count].filename[15] = '\0';
                file_list[count].size_bytes = (uint32_t)fno.fsize;
                file_list[count].total_frames = (uint32_t)fno.fsize / LCD_FRAME_SIZE;
                count++;
            }
        }
    }

    f_closedir(&dir);
    return count;
}

uint8_t MediaPlayer_SelectVideoUI(VideoFileInfo_t *files, uint8_t file_count)
{
    if (file_count == 0) return 0;

    uint8_t selected = 0;
    uint32_t last_action_time = Get_Tick_ms();
    const uint32_t auto_play_timeout = 4000; /* 4 giây tự động phát */

    while (1)
    {
        uint32_t fb = LCD_FRAMEBUFFER_0;

        /* 1. Nền Menu */
        DMA2D_FillRect(fb, 0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_DARKBLUE);

        /* 2. Tiêu đề Header */
        DMA2D_FillRect(fb, 10, 8, LCD_WIDTH - 20, 34, COLOR_NAVY);
        DMA2D_FillRect(fb, 10, 8, LCD_WIDTH - 20, 2, COLOR_CYAN);
        DMA2D_FillRect(fb, 10, 40, LCD_WIDTH - 20, 2, COLOR_CYAN);
        LCD_DrawString(fb, 20, 14, "STM32F746 MEDIA PLAYER - FILE MENU", COLOR_CYAN, COLOR_NAVY);
        LCD_DrawString(fb, 20, 26, "SDMMC1 Multi-Block Burst @ 48MHz (60 FPS)", COLOR_WHITE, COLOR_NAVY);

        /* 3. Danh sách tệp tin */
        for (uint8_t i = 0; i < file_count && i < 6; i++)
        {
            uint16_t row_y = 48 + i * 26;
            uint16_t bg = (i == selected) ? COLOR_NAVY : COLOR_DARKBLUE;
            uint16_t fg = (i == selected) ? COLOR_YELLOW : COLOR_WHITE;

            DMA2D_FillRect(fb, 16, row_y, LCD_WIDTH - 32, 22, bg);
            if (i == selected)
            {
                DMA2D_FillRect(fb, 16, row_y, 4, 22, COLOR_CYAN);
                DMA2D_FillRect(fb, 16, row_y, LCD_WIDTH - 32, 1, COLOR_CYAN);
                DMA2D_FillRect(fb, 16, row_y + 21, LCD_WIDTH - 32, 1, COLOR_CYAN);
            }

            char item_str[48];
            char idx_str[4];
            char sz_str[12];
            uint_to_str(i + 1, idx_str);
            uint_to_str(files[i].size_bytes / (1024 * 1024), sz_str);

            item_str[0] = (i == selected) ? '>' : ' ';
            item_str[1] = ' ';
            item_str[2] = '[';
            item_str[3] = idx_str[0];
            item_str[4] = ']';
            item_str[5] = ' ';
            item_str[6] = '\0';
            strcat(item_str, files[i].filename);
            strcat(item_str, "  (");
            strcat(item_str, sz_str);
            strcat(item_str, " MB)");

            LCD_DrawString(fb, 26, row_y + 6, item_str, fg, bg);
        }

        /* 4. Khung hướng dẫn bên dưới */
        uint32_t now = Get_Tick_ms();
        uint32_t elapsed = now - last_action_time;
        int32_t remaining = (auto_play_timeout > elapsed) ? (int32_t)((auto_play_timeout - elapsed) / 1000) + 1 : 0;

        DMA2D_FillRect(fb, 10, 218, LCD_WIDTH - 20, 46, COLOR_BLACK);
        DMA2D_FillRect(fb, 10, 218, LCD_WIDTH - 20, 2, COLOR_CYAN);

        char hint_str[64];
        char cd_str[8];
        uint_to_str(remaining, cd_str);
        strcpy(hint_str, "[BTN]: Click = Next File | Auto-Play in: ");
        strcat(hint_str, cd_str);
        strcat(hint_str, "s");
        LCD_DrawString(fb, 18, 226, hint_str, COLOR_YELLOW, COLOR_BLACK);
        LCD_DrawString(fb, 18, 242, "Hold Button >0.5s to PLAY SELECTED FILE now", COLOR_WHITE, COLOR_BLACK);

        LTDC_SetAddress(fb);

        /* Kiểm tra nút nhấn User Button (PI11) */
        if (GPIOI->IDR & (1U << 11))
        {
            uint32_t press_start = Get_Tick_ms();
            while (GPIOI->IDR & (1U << 11))
            {
                if (Get_Tick_ms() - press_start > 500)
                {
                    while (GPIOI->IDR & (1U << 11)); /* Chờ nhả nút */
                    Delay_ms(100);
                    return selected;
                }
            }
            /* Click ngắn: Chuyển sang file kế tiếp */
            selected = (selected + 1) % file_count;
            last_action_time = Get_Tick_ms();
            Delay_ms(150);
        }

        if (elapsed >= auto_play_timeout)
        {
            return selected;
        }

        Delay_ms(50);
    }
}

void MediaPlayer_PlayFile(const char *filename)
{
    /* 1. Dong file cu neu con mo */
    f_close(&s_fil);

    /* 2. Cho nguoi dung nha hoan toan nut bam tu Menu (tranh trigger nham khi vua chon file) */
    while (GPIOI->IDR & (1U << 11));
    Delay_ms(200);

    /* 3. Mo tap tin video tu the nho */
    FRESULT res = f_open(&s_fil, filename, FA_READ);
    if (res != FR_OK)
    {
        char err_msg[64];
        char err_code[8];
        uint_to_str((uint32_t)res, err_code);
        strcpy(err_msg, "F_OPEN FAILED: ");
        strcat(err_msg, filename);
        strcat(err_msg, " ERR=");
        strcat(err_msg, err_code);
        DMA2D_FillRect(LCD_FRAMEBUFFER_0, 0, 0, LCD_WIDTH, 40, COLOR_RED);
        LCD_DrawString(LCD_FRAMEBUFFER_0, 10, 14, err_msg, COLOR_WHITE, COLOR_RED);
        Delay_ms(3000);
        return;
    }

    uint32_t play_start_time = Get_Tick_ms();
    uint32_t frame_count = 0;
    uint32_t last_fps_time = Get_Tick_ms();
    uint32_t current_fps = 60;

    while (1)
    {
        uint32_t frame_start = Get_Tick_ms();

        /* Khoa nut bam trong 1.5 giay dau tien de tranh doi nut tu Menu */
        if (Get_Tick_ms() - play_start_time > 1500)
        {
            /* Kiem tra nut nhan User Button (PI11) loc nhieu 50ms */
            if (GPIOI->IDR & (1U << 11))
            {
                Delay_ms(50); /* Loc xung nhieu EMI */
                if (GPIOI->IDR & (1U << 11))
                {
                    while (GPIOI->IDR & (1U << 11)); /* Cho nha nut */
                    Delay_ms(100);
                    break; /* Nguoi dung thuc su chu dong bam nut -> Thoat ve Menu */
                }
            }
        }

        uint32_t back_buffer = (s_current_buffer_idx == 0) ? LCD_FRAMEBUFFER_1 : LCD_FRAMEBUFFER_0;
        UINT bytes_read = 0;

        /* Đọc 1 khung hình (261,120 bytes) qua CMD18 Multi-Block nạp vào SDRAM Back Buffer */
        res = f_read(&s_fil, (void *)back_buffer, LCD_FRAME_SIZE, &bytes_read);
        if (res != FR_OK || bytes_read < LCD_FRAME_SIZE)
        {
            /* Đã phát hết video: Tua lại từ đầu (Loop Video) */
            f_lseek(&s_fil, 0);
            continue;
        }

        /* Tính toán FPS thực tế */
        frame_count++;
        uint32_t now = Get_Tick_ms();
        if (now - last_fps_time >= 500)
        {
            current_fps = (frame_count * 1000UL) / (now - last_fps_time);
            frame_count = 0;
            last_fps_time = now;
        }

        /* 1. Vẽ Badge FPS ở góc trái trên cùng */
        DMA2D_FillRect(back_buffer, 6, 6, 140, 16, COLOR_BLACK);
        char str_buf[32];
        char fps_num[10];
        uint_to_str(current_fps, fps_num);
        strcpy(str_buf, "FPS: ");
        strcat(str_buf, fps_num);
        strcat(str_buf, " | ");
        strncat(str_buf, filename, 7);
        LCD_DrawString(back_buffer, 10, 10, str_buf, COLOR_GREEN, COLOR_BLACK);

        /* 2. Vẽ nút gợi ý quay về Menu ở góc phải trên cùng */

        /* Đồng bộ D-Cache */
        __asm volatile ("dsb 0xF" ::: "memory");

        /* Hoán đổi Framebuffer tại VSYNC (Triệt tiêu xé hình 100%) */
        LTDC_SwapBuffers_VBlank(back_buffer);
        s_current_buffer_idx = 1 - s_current_buffer_idx;

        /* Khống chế tốc độ tối đa chuẩn 60 FPS (16.6ms / frame) */
        uint32_t frame_time = Get_Tick_ms() - frame_start;
        if (frame_time < 16)
        {
            Delay_ms(16 - frame_time);
        }
    }

    f_close(&s_fil);
}

void MediaPlayer_RunGraphicsDemo(void)
{
    static int16_t x = 50, y = 50;
    static int16_t dx = 3, dy = 2;
    const uint16_t sprite_w = 64, sprite_h = 64;

    while (1)
    {
        /* Kiểm tra nút nhấn thoát */
        if (GPIOI->IDR & (1U << 11))
        {
            Delay_ms(200);
            break;
        }

        uint32_t back_buffer = (s_current_buffer_idx == 0) ? LCD_FRAMEBUFFER_1 : LCD_FRAMEBUFFER_0;

        /* 1. Vẽ dải màu Color Bar nền bằng DMA2D */
        uint16_t bar_w = LCD_WIDTH / 8;
        uint16_t colors[8] = {
            COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN, COLOR_GREEN,
            COLOR_MAGENTA, COLOR_RED, COLOR_BLUE, COLOR_BLACK
        };

        for (uint8_t i = 0; i < 8; i++)
        {
            DMA2D_FillRect(back_buffer, i * bar_w, 0, bar_w, LCD_HEIGHT, colors[i]);
        }

        /* 2. Cập nhật tọa độ Sprite */
        x += dx;
        y += dy;
        if (x <= 0 || (x + sprite_w) >= LCD_WIDTH)  dx = -dx;
        if (y <= 0 || (y + sprite_h) >= LCD_HEIGHT) dy = -dy;

        /* 3. Vẽ Sprite hộp khối di chuyển bằng DMA2D */
        DMA2D_FillRect(back_buffer, (uint16_t)x, (uint16_t)y, sprite_w, sprite_h, COLOR_MAGENTA);
        DMA2D_FillRect(back_buffer, (uint16_t)(x + 8), (uint16_t)(y + 8), sprite_w - 16, sprite_h - 16, COLOR_WHITE);

        /* 4. Hoán đổi buffer tại VBLANK (60 FPS mượt mà) */
        LTDC_SwapBuffers_VBlank(back_buffer);
        s_current_buffer_idx = 1 - s_current_buffer_idx;
        Delay_ms(16);
    }
}
