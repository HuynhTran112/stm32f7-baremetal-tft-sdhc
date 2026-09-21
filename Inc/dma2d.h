/**
 * @file dma2d.h
 * @brief Giao diện bộ tăng tốc đồ họa phần cứng Chrom-ART (DMA2D)
 */

#ifndef DMA2D_H
#define DMA2D_H

#include <stdint.h>

void DMA2D_Init(void);
void DMA2D_FillRect(uint32_t dst_addr, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color_rgb565);
void DMA2D_CopyRect(uint32_t src_addr, uint32_t dst_addr,
                    uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    uint16_t src_pitch, uint16_t dst_pitch);

#endif /* DMA2D_H */
