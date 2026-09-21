/**
 * @file sdram.h
 * @brief Định nghĩa và giao diện điều khiển chip FMC SDRAM 8 MB (MT48LC4M32B2)
 */

#ifndef SDRAM_H
#define SDRAM_H

#include <stdint.h>

/* Địa chỉ bộ nhớ SDRAM Bank 1 */
#define SDRAM_BASE_ADDR         0xC0000000UL
#define SDRAM_SIZE_BYTES        (8UL * 1024UL * 1024UL) /* 8 Megabytes */

/* Phân bổ vùng nhớ Double Buffer cho màn hình LCD */
#define LCD_FRAME_SIZE          (480UL * 272UL * 2UL)   /* 261,120 bytes / frame (RGB565) */
#define LCD_FRAMEBUFFER_0       (SDRAM_BASE_ADDR)
#define LCD_FRAMEBUFFER_1       (SDRAM_BASE_ADDR + 0x00040000UL) /* Cách 256 KB */

void SDRAM_Init(void);
uint8_t SDRAM_Test(void);

#endif /* SDRAM_H */
