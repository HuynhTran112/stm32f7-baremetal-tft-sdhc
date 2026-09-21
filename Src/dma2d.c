/**
 * @file dma2d.c
 * @brief Điều khiển bộ tăng tốc 2D Chrom-ART (DMA2D) Bare-Metal
 */

#include "dma2d.h"
#include "ltdc.h"
#include "reg.h"

void DMA2D_Init(void)
{
    /* Cấp xung nhịp cho DMA2D (AHB1) */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2DEN;
    (void)RCC->AHB1ENR;
}

void DMA2D_FillRect(uint32_t dst_addr, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color_rgb565)
{
    if (w == 0 || h == 0) return;

    /* Tính toán địa chỉ byte đầu tiên trong Framebuffer */
    uint32_t start_addr = dst_addr + 2U * ((uint32_t)y * LCD_WIDTH + (uint32_t)x);
    uint32_t line_offset = LCD_WIDTH - w;

    /* Chờ lượt DMA2D trước hoàn tất */
    while (DMA2D->CR & DMA2D_CR_START);

    /* Chế độ: Register-to-Memory (MODE = 11b = 3U << 16) */
    DMA2D->CR = (3U << 16);

    /* Định dạng đích: RGB565 (Mã số: 2U) */
    DMA2D->OPFCCR = 2U;

    /* Giá trị màu RGB565 cần tô */
    DMA2D->OCOLR = (uint32_t)color_rgb565;

    /* Địa chỉ bộ nhớ đích và Offset dòng */
    DMA2D->OMAR = start_addr;
    DMA2D->OOR  = line_offset;

    /* Số lượng pixel (Chiều rộng x Chiều cao) */
    DMA2D->NLR = ((uint32_t)w << 16) | (uint32_t)h;

    /* Bắt đầu chuyển dữ liệu */
    DMA2D->CR |= DMA2D_CR_START;

    /* Chờ cờ hoàn tất truyền Transfer Complete */
    while (!(DMA2D->ISR & DMA2D_ISR_TCIF));

    /* Xóa cờ bằng cách ghi 1 vào thanh ghi W1C IFCR */
    DMA2D->IFCR = DMA2D_IFCR_CTCIF;
}

void DMA2D_CopyRect(uint32_t src_addr, uint32_t dst_addr,
                    uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    uint16_t src_pitch, uint16_t dst_pitch)
{
    if (w == 0 || h == 0) return;

    uint32_t start_dst = dst_addr + 2U * ((uint32_t)y * dst_pitch + (uint32_t)x);

    while (DMA2D->CR & DMA2D_CR_START);

    /* Chế độ: Memory-to-Memory (MODE = 00b) */
    DMA2D->CR = 0U;

    /* Định dạng nguồn và đích: RGB565 (Mã số: 2U) */
    DMA2D->FGPFCCR = 2U;
    DMA2D->OPFCCR  = 2U;

    DMA2D->FGMAR = src_addr;
    DMA2D->OMAR  = start_dst;

    DMA2D->FGOR  = src_pitch - w;
    DMA2D->OOR   = dst_pitch - w;

    DMA2D->NLR   = ((uint32_t)w << 16) | (uint32_t)h;

    DMA2D->CR |= DMA2D_CR_START;
    while (!(DMA2D->ISR & DMA2D_ISR_TCIF));
    DMA2D->IFCR = DMA2D_IFCR_CTCIF;
}
