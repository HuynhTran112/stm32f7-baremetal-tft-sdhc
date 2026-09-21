/**
 * @file sdmmc.c
 * @brief Điều khiển khối ngoại vi SDMMC1 bus 4-bit Bare-Metal tốc độ cao (CMD18 Multi-block streaming)
 */

#include "sdmmc.h"
#include "sys_clock.h"
#include "reg.h"

static uint32_t s_RCA = 0;
static uint8_t  s_CardType = SD_TYPE_UNKNOWN;

static void SDMMC_GPIO_Config(void)
{
    /* Cấp xung cho GPIOC và GPIOD */
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN);
    (void)RCC->AHB1ENR;

    /* PC8 (D0), PC9 (D1), PC10 (D2), PC11 (D3), PC12 (CK) -> AF12 */
    GPIOC->MODER &= ~((3U << 16) | (3U << 18) | (3U << 20) | (3U << 22) | (3U << 24));
    GPIOC->MODER |=  ((2U << 16) | (2U << 18) | (2U << 20) | (2U << 22) | (2U << 24));
    GPIOC->OSPEEDR |= ((3U << 16) | (3U << 18) | (3U << 20) | (3U << 22) | (3U << 24));
    GPIOC->PUPDR   &= ~((3U << 16) | (3U << 18) | (3U << 20) | (3U << 22) | (3U << 24));
    GPIOC->PUPDR   |= ((1U << 16) | (1U << 18) | (1U << 20) | (1U << 22)); /* Pull-up các đường data */
    GPIOC->AFRH &= ~((0xFU << 0) | (0xFU << 4) | (0xFU << 8) | (0xFU << 12) | (0xFU << 16));
    GPIOC->AFRH |=  ((12U << 0) | (12U << 4) | (12U << 8) | (12U << 12) | (12U << 16));

    /* PD2 (CMD) -> AF12 */
    GPIOD->MODER &= ~(3U << 4);
    GPIOD->MODER |=  (2U << 4);
    GPIOD->OSPEEDR |= (3U << 4);
    GPIOD->PUPDR   &= ~(3U << 4);
    GPIOD->PUPDR   |= (1U << 4); /* Pull-up CMD */
    GPIOD->AFRL &= ~(0xFU << 8);
    GPIOD->AFRL |=  (12U << 8);
}

static uint8_t SDMMC_SendCommand(uint32_t cmd_index, uint32_t arg, uint32_t wait_resp)
{
    /* Xóa cờ lệnh cũ */
    SDMMC1->ICR = (1U << 7) | (1U << 6) | (1U << 2) | (1U << 0);

    SDMMC1->ARG = arg;
    SDMMC1->CMD = (cmd_index & 0x3F) | (wait_resp << 6) | (1U << 10); /* CPSMEN = 1 */

    if (wait_resp == 0)
    {
        uint32_t timeout = 500000;
        while (!(SDMMC1->STA & (1U << 7)) && --timeout);
        return (timeout > 0) ? SD_OK : SD_TIMEOUT;
    }
    else
    {
        uint32_t timeout = 5000000; /* ~70 ms ở 216 MHz */
        while (!(SDMMC1->STA & ((1U << 6) | (1U << 0) | (1U << 2))) && --timeout);
        if (timeout == 0) return SD_TIMEOUT;

        /* ACMD41 trả về R3 (không có CRC) nên cờ CCRCFAIL (bit 0) là bình thường */
        if (cmd_index == 41)
        {
            return SD_OK;
        }

        if (SDMMC1->STA & (1U << 6)) return SD_OK;
        return SD_ERROR;
    }
}

uint8_t SDMMC_Init(void)
{
    s_CardType = SD_TYPE_UNKNOWN;

    /* 1. Cấu hình chân GPIO */
    SDMMC_GPIO_Config();

    /* 2. Cấp xung cho SDMMC1 (APB2) */
    RCC->APB2ENR |= RCC_APB2ENR_SDMMC1EN;
    (void)RCC->APB2ENR;

    /* 3. Bật nguồn module SDMMC: PWRCTRL = 11b */
    SDMMC1->POWER = 3U;
    Delay_ms(20);

    /* 4. Khởi động ở tần số chậm < 400 kHz (Open-Drain):
     * f_SDMMCCLK = 48 MHz -> CLKDIV = (48000 / 400) - 2 = 118
     */
    SDMMC1->CLKCR = (118U << 0) | (1U << 8);
    Delay_ms(50);

    /* 5. Gửi CMD0: Reset thẻ về IDLE state */
    SDMMC_SendCommand(0, 0, 0);
    Delay_ms(20);

    /* 6. Gửi CMD8: Kiểm tra điện áp 2.7V - 3.6V (VHS=0001b, Check Pattern=0xAA) */
    if (SDMMC_SendCommand(8, 0x000001AA, 1) != SD_OK)
    {
        return SD_ERROR;
    }

    /* 7. Gửi ACMD41 lặp cho đến khi thẻ thoát Busy (bit 31 = 1) */
    uint32_t retry = 200;
    while (retry--)
    {
        SDMMC_SendCommand(55, 0, 1);
        SDMMC_SendCommand(41, (1U << 30) | (1U << 20) | 0x00FF8000, 1);
        if (SDMMC1->RESP1 & (1U << 31)) break;
        Delay_ms(10);
    }
    if (retry == 0) return SD_TIMEOUT;

    /* Kiểm tra cờ CCS (Card Capacity Status, bit 30) trong phản hồi OCR */
    if (SDMMC1->RESP1 & (1U << 30))
    {
        s_CardType = SD_TYPE_SDHC; /* SDHC / SDXC: Block Addressing (LBA 512 bytes) */
    }
    else
    {
        s_CardType = SD_TYPE_SDSC; /* SDSC: Byte Addressing */
    }

    /* 8. Gửi CMD2 (ALL_SEND_CID) đọc định danh thẻ */
    SDMMC_SendCommand(2, 0, 3);
    Delay_ms(10);

    /* 9. Gửi CMD3 (SEND_RELATIVE_ADDR) lấy địa chỉ logic RCA */
    SDMMC_SendCommand(3, 0, 1);
    s_RCA = SDMMC1->RESP1 & 0xFFFF0000;
    Delay_ms(10);

    /* 10. Gửi CMD7 chọn thẻ (chuyển từ Standby sang Transfer State) */
    SDMMC_SendCommand(7, s_RCA, 1);
    Delay_ms(20); /* Chờ thẻ chuyển sang trạng thái tran */

    /* 11. Chuyển sang Bus 4-bit (ACMD6 với tham số 2) */
    SDMMC_SendCommand(55, s_RCA, 1);
    Delay_ms(5);
    uint8_t acmd6_res = SDMMC_SendCommand(6, 2, 1);
    Delay_ms(10);

    /* 12. Nâng xung nhịp lên 24 MHz (CLKDIV = 0: 48 / (0 + 2) = 24 MHz) */
    if (acmd6_res == SD_OK)
    {
        /* 4-bit mode: WIDBUS = 01b (bit 11 = 1), CLKDIV = 0 */
        /* 48 MHz Bypass Mode + HW Flow Control */
        SDMMC1->CLKCR = (1U << 10) | (1U << 8) | (1U << 11) | (1U << 14);
    }
    else
    {
        /* 1-bit mode fallback an toàn: WIDBUS = 00b */
        SDMMC1->CLKCR = (0U << 0) | (1U << 8);
    }
    Delay_ms(20);

    /* 13. Khóa kích thước khối 512 bytes */
    SDMMC_SendCommand(16, SD_BLOCK_SIZE, 1);
    Delay_ms(10);

    return SD_OK;
}

uint8_t SDMMC_GetCardType(void)
{
    return s_CardType;
}

const char *SDMMC_GetCardTypeName(void)
{
    switch (s_CardType)
    {
        case SD_TYPE_SDHC: return "SDHC (High Capacity 4GB-32GB, Block Addressing LBA)";
        case SD_TYPE_SDSC: return "SDSC (Standard Capacity <=2GB, Byte Addressing)";
        default:           return "UNKNOWN";
    }
}

uint8_t SDMMC_ReadSingleBlock(uint32_t block_addr, uint8_t *pBuffer)
{
    /* 1. Xóa sạch cờ dữ liệu cũ */
    SDMMC1->ICR = 0x00FFFFFF;

    /* 2. Cấu hình khối nhận dữ liệu */
    SDMMC1->DTIMER = 0x0FFFFFFF;
    SDMMC1->DLEN = SD_BLOCK_SIZE;
    /* DBLOCKSIZE = 9 (512B), DTDIR = 1 (Card to Controller), DTEN = 1 */
    SDMMC1->DCTRL = (9U << 4) | (1U << 1) | (1U << 0);

    /* 3. Gửi lệnh CMD17 */
    uint32_t cmd_arg = (s_CardType == SD_TYPE_SDHC) ? block_addr : (block_addr * SD_BLOCK_SIZE);

    SDMMC1->ICR = (1U << 7) | (1U << 6) | (1U << 2) | (1U << 0);
    SDMMC1->ARG = cmd_arg;
    SDMMC1->CMD = 17U | (1U << 6) | (1U << 10);

    /* Chờ phản hồi CMD17 */
    uint32_t cmd_timeout = 5000000;
    while (!(SDMMC1->STA & ((1U << 6) | (1U << 0) | (1U << 2))) && --cmd_timeout);
    if (cmd_timeout == 0 || !(SDMMC1->STA & (1U << 6)))
    {
        SDMMC1->DCTRL = 0;
        return SD_ERROR;
    }

    /* 4. Nhận 128 words (512 bytes) từ FIFO */
    uint32_t *pDst = (uint32_t *)pBuffer;
    uint32_t words_left = SD_BLOCK_SIZE / 4;
    uint32_t data_timeout = 10000000;

    while (words_left > 0 && --data_timeout)
    {
        if (SDMMC1->STA & (1U << 21)) /* RXDAVL */
        {
            *pDst++ = SDMMC1->FIFO;
            words_left--;
        }
        if (SDMMC1->STA & ((1U << 1) | (1U << 3) | (1U << 5)))
        {
            SDMMC1->DCTRL = 0;
            return SD_ERROR;
        }
    }

    uint32_t wait_end = 100000;
    while (!(SDMMC1->STA & (1U << 8)) && --wait_end);
    SDMMC1->ICR = 0x00FFFFFF;

    return (words_left == 0) ? SD_OK : SD_TIMEOUT;
}

/**
 * @brief Đọc liên tục nhiều sector bằng CMD18 (READ_MULTIPLE_BLOCK) tốc độ cao
 * Giúp đạt băng thông tối đa 12 - 24 MB/s cho video 60 FPS
 */
uint8_t SDMMC_ReadMultiBlocks(uint32_t block_addr, uint8_t *pBuffer, uint32_t num_blocks)
{
    if (num_blocks == 0) return SD_OK;
    if (num_blocks == 1) return SDMMC_ReadSingleBlock(block_addr, pBuffer);

    /* 1. Xóa cờ trạng thái cũ */
    SDMMC1->ICR = 0x00FFFFFF;

    /* 2. Cấu hình khối nhận dữ liệu multi-block */
    uint32_t total_bytes = num_blocks * SD_BLOCK_SIZE;
    uint32_t total_words = total_bytes / 4;

    SDMMC1->DTIMER = 0x0FFFFFFF;
    SDMMC1->DLEN = total_bytes;
    /* DBLOCKSIZE = 9 (512B), DTDIR = 1 (Card to Controller), DTEN = 1 */
    SDMMC1->DCTRL = (9U << 4) | (1U << 1) | (1U << 0);

    /* 3. Gửi lệnh CMD18 (READ_MULTIPLE_BLOCK) */
    uint32_t cmd_arg = (s_CardType == SD_TYPE_SDHC) ? block_addr : (block_addr * SD_BLOCK_SIZE);

    SDMMC1->ICR = (1U << 7) | (1U << 6) | (1U << 2) | (1U << 0);
    SDMMC1->ARG = cmd_arg;
    SDMMC1->CMD = 18U | (1U << 6) | (1U << 10);

    /* Chờ phản hồi CMD18 */
    uint32_t cmd_timeout = 5000000;
    while (!(SDMMC1->STA & ((1U << 6) | (1U << 0) | (1U << 2))) && --cmd_timeout);
    if (cmd_timeout == 0 || !(SDMMC1->STA & (1U << 6)))
    {
        SDMMC1->DCTRL = 0;
        return SD_ERROR;
    }

    /* 4. Nhận luồng dữ liệu từ FIFO bằng vòng lặp unroll tốc độ cao */
    uint32_t *pDst = (uint32_t *)pBuffer;
    uint32_t words_left = total_words;
    uint32_t data_timeout = 30000000;

    while (words_left > 0 && --data_timeout)
    {
        /* Đọc cụm 8 words nếu FIFO có sẵn (RXFIFOHF - Half Full) */
        if (words_left >= 8 && (SDMMC1->STA & (1U << 15)))
        {
            pDst[0] = SDMMC1->FIFO;
            pDst[1] = SDMMC1->FIFO;
            pDst[2] = SDMMC1->FIFO;
            pDst[3] = SDMMC1->FIFO;
            pDst[4] = SDMMC1->FIFO;
            pDst[5] = SDMMC1->FIFO;
            pDst[6] = SDMMC1->FIFO;
            pDst[7] = SDMMC1->FIFO;
            pDst += 8;
            words_left -= 8;
        }
        else if (SDMMC1->STA & (1U << 21)) /* RXDAVL */
        {
            *pDst++ = SDMMC1->FIFO;
            words_left--;
        }

        if (SDMMC1->STA & ((1U << 1) | (1U << 3) | (1U << 5))) /* DCRCFAIL, DTIMEOUT, RXOVERR */
        {
            /* Gửi CMD12 dừng truyền trước khi return */
            SDMMC1->ARG = 0;
            SDMMC1->CMD = 12U | (1U << 6) | (1U << 10);
            SDMMC1->DCTRL = 0;
            return SD_ERROR;
        }
    }

    /* 5. Gửi lệnh CMD12 (STOP_TRANSMISSION) để báo thẻ ngừng bơm dữ liệu */
    SDMMC1->ICR = (1U << 7) | (1U << 6) | (1U << 2) | (1U << 0);
    SDMMC1->ARG = 0;
    SDMMC1->CMD = 12U | (1U << 6) | (1U << 10);
    cmd_timeout = 500000;
    while (!(SDMMC1->STA & ((1U << 6) | (1U << 0) | (1U << 2))) && --cmd_timeout);

    /* Chờ cờ DATAEND */
    uint32_t wait_end = 100000;
    while (!(SDMMC1->STA & (1U << 8)) && --wait_end);
    SDMMC1->ICR = 0x00FFFFFF;

    return (words_left == 0) ? SD_OK : SD_TIMEOUT;
}
