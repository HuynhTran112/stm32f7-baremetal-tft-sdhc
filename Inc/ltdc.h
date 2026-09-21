/**
 * @file ltdc.h
 * @brief Định nghĩa và giao diện điều khiển màn hình LCD LTDC 480x272
 */

#ifndef LTDC_H
#define LTDC_H

#include <stdint.h>

#define LCD_WIDTH           480U
#define LCD_HEIGHT          272U

/* Bảng màu RGB565 cơ bản */
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_DARKBLUE      0x0810
#define COLOR_GRAY          0x8410
#define COLOR_NAVY          0x000F

void LTDC_Init(void);
void LTDC_SetAddress(uint32_t framebuffer_address);
void LTDC_SwapBuffers_VBlank(uint32_t new_framebuffer_address);
void LCD_Clear(uint32_t fb_address, uint16_t color_rgb565);

#endif /* LTDC_H */
