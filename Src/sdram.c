/**
 * @file sdram.c
 * @brief Cấu hình ngoại vi FMC và chu trình khởi tạo JEDEC SDRAM cho STM32F746G-DISCO
 *
 * Tra cứu phần cứng bo mạch Discovery:
 * - Chip SDRAM: Micron MT48LC4M32B2B5-6A (128-Mbit, truy xuất 16-bit data = 8 MB)
 * - Địa chỉ bắt đầu: 0xC0000000 (FMC SDRAM Bank 1)
 * - Ánh xạ chân GPIO từ sơ đồ nguyên lý bo mạch (UM1907 Section 6.11 & Table 7/8/9):
 *   * SDCKE0: PC3 (AF12) - LƯU Ý: PC3, KHÔNG PHẢI PH2!
 *   * SDNE0:  PH3 (AF12)
 *   * SDNWE:  PH5 (AF12)
 *   * SDCLK:  PG8 (AF12)
 *   * SDNRAS: PF11 (AF12)
 *   * SDNCAS: PG15 (AF12)
 *   * Bank Address: BA0(PG4), BA1(PG5) (AF12)
 *   * Address A0..A11:
 *     PF0..PF5 (A0..A5), PF12..PF15 (A6..A9), PG0..PG1 (A10..A11) (AF12)
 *   * Data Mask: NBL0(PE0), NBL1(PE1) (AF12)
 *   * Data D0..D15:
 *     PD14(D0), PD15(D1), PD0(D2), PD1(D3) (AF12)
 *     PE7(D4), PE8(D5), PE9(D6), PE10(D7), PE11(D8) (AF12)
 *     PE12(D9), PE13(D10), PE14(D11), PE15(D12) (AF12)
 *     PD8(D13), PD9(D14), PD10(D15) (AF12) - LƯU Ý: PD8..PD10, KHÔNG PHẢI PH8..PH10!
 */

#include "sdram.h"
#include "reg.h"

static void SDRAM_GPIO_Config(void)
{
    /* 1. Bật xung nhịp cho toàn bộ các Port GPIO của FMC: C, D, E, F, G, H */
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                     RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                     RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN);
    (void)RCC->AHB1ENR;

    /* 2. Port C: PC3 = FMC_SDCKE0 (AF12) */
    GPIOC->MODER &= ~(3U << 6);
    GPIOC->MODER |=  (2U << 6);
    GPIOC->OSPEEDR |= (3U << 6);
    GPIOC->AFRL &= ~(0xFU << 12);
    GPIOC->AFRL |=  (12U << 12);

    /* 3. Port D:
     * PD0 (D2), PD1 (D3)
     * PD8 (D13), PD9 (D14), PD10 (D15)
     * PD14 (D0), PD15 (D1)
     */
    GPIOD->MODER &= ~((3U << 0) | (3U << 2) | (3U << 16) | (3U << 18) | (3U << 20) | (3U << 28) | (3U << 30));
    GPIOD->MODER |=  ((2U << 0) | (2U << 2) | (2U << 16) | (2U << 18) | (2U << 20) | (2U << 28) | (2U << 30));
    GPIOD->OSPEEDR |= ((3U << 0) | (3U << 2) | (3U << 16) | (3U << 18) | (3U << 20) | (3U << 28) | (3U << 30));
    GPIOD->AFRL &= ~((0xFU << 0) | (0xFU << 4));
    GPIOD->AFRL |=  ((12U << 0) | (12U << 4));
    GPIOD->AFRH &= ~((0xFU << 0) | (0xFU << 4) | (0xFU << 8) | (0xFU << 24) | (0xFU << 28));
    GPIOD->AFRH |=  ((12U << 0) | (12U << 4) | (12U << 8) | (12U << 24) | (12U << 28));

    /* 4. Port E:
     * PE0 (NBL0), PE1 (NBL1)
     * PE7 (D4), PE8 (D5), PE9 (D6), PE10 (D7), PE11 (D8)
     * PE12 (D9), PE13 (D10), PE14 (D11), PE15 (D12)
     */
    GPIOE->MODER &= ~((3U << 0) | (3U << 2) | (3U << 14) | (3U << 16) | (3U << 18) | (3U << 20) | (3U << 22) | (3U << 24) | (3U << 26) | (3U << 28) | (3U << 30));
    GPIOE->MODER |=  ((2U << 0) | (2U << 2) | (2U << 14) | (2U << 16) | (2U << 18) | (2U << 20) | (2U << 22) | (2U << 24) | (2U << 26) | (2U << 28) | (2U << 30));
    GPIOE->OSPEEDR |= ((3U << 0) | (3U << 2) | (3U << 14) | (3U << 16) | (3U << 18) | (3U << 20) | (3U << 22) | (3U << 24) | (3U << 26) | (3U << 28) | (3U << 30));
    GPIOE->AFRL &= ~((0xFU << 0) | (0xFU << 4) | (0xFU << 28));
    GPIOE->AFRL |=  ((12U << 0) | (12U << 4) | (12U << 28));
    GPIOE->AFRH = 0xCCCCCCCC; /* Toàn bộ PE8..PE15 là AF12 */

    /* 5. Port F:
     * PF0..PF5 (A0..A5)
     * PF11 (SDNRAS)
     * PF12..PF15 (A6..A9)
     */
    GPIOF->MODER &= ~((3U << 0) | (3U << 2) | (3U << 4) | (3U << 6) | (3U << 8) | (3U << 10) | (3U << 22) | (3U << 24) | (3U << 26) | (3U << 28) | (3U << 30));
    GPIOF->MODER |=  ((2U << 0) | (2U << 2) | (2U << 4) | (2U << 6) | (2U << 8) | (2U << 10) | (2U << 22) | (2U << 24) | (2U << 26) | (2U << 28) | (2U << 30));
    GPIOF->OSPEEDR |= ((3U << 0) | (3U << 2) | (3U << 4) | (3U << 6) | (3U << 8) | (3U << 10) | (3U << 22) | (3U << 24) | (3U << 26) | (3U << 28) | (3U << 30));
    GPIOF->AFRL &= ~((0xFU << 0) | (0xFU << 4) | (0xFU << 8) | (0xFU << 12) | (0xFU << 16) | (0xFU << 20));
    GPIOF->AFRL |=  ((12U << 0) | (12U << 4) | (12U << 8) | (12U << 12) | (12U << 16) | (12U << 20));
    GPIOF->AFRH &= ~((0xFU << 12) | (0xFU << 16) | (0xFU << 20) | (0xFU << 24) | (0xFU << 28));
    GPIOF->AFRH |=  ((12U << 12) | (12U << 16) | (12U << 20) | (12U << 24) | (12U << 28));

    /* 6. Port G:
     * PG0 (A10), PG1 (A11)
     * PG4 (BA0), PG5 (BA1)
     * PG8 (SDCLK)
     * PG15 (SDNCAS)
     */
    GPIOG->MODER &= ~((3U << 0) | (3U << 2) | (3U << 8) | (3U << 10) | (3U << 16) | (3U << 30));
    GPIOG->MODER |=  ((2U << 0) | (2U << 2) | (2U << 8) | (2U << 10) | (2U << 16) | (2U << 30));
    GPIOG->OSPEEDR |= ((3U << 0) | (3U << 2) | (3U << 8) | (3U << 10) | (3U << 16) | (3U << 30));
    GPIOG->AFRL &= ~((0xFU << 0) | (0xFU << 4) | (0xFU << 16) | (0xFU << 20));
    GPIOG->AFRL |=  ((12U << 0) | (12U << 4) | (12U << 16) | (12U << 20));
    GPIOG->AFRH &= ~((0xFU << 0) | (0xFU << 28));
    GPIOG->AFRH |=  ((12U << 0) | (12U << 28));

    /* 7. Port H:
     * PH3 (SDNE0), PH5 (SDNWE)
     */
    GPIOH->MODER &= ~((3U << 6) | (3U << 10));
    GPIOH->MODER |=  ((2U << 6) | (2U << 10));
    GPIOH->OSPEEDR |= ((3U << 6) | (3U << 10));
    GPIOH->AFRL &= ~((0xFU << 12) | (0xFU << 20));
    GPIOH->AFRL |=  ((12U << 12) | (12U << 20));
}

void SDRAM_Init(void)
{
    /* 1. Cấu hình chân GPIO Alternate Function AF12 cho FMC */
    SDRAM_GPIO_Config();

    /* 2. Cấp xung nhịp cho bộ điều khiển FMC (AHB3) */
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
    (void)RCC->AHB3ENR;
    volatile uint32_t dummy = RCC->AHB3ENR;
    (void)dummy;

    /* 3. Cấu hình thanh ghi điều khiển SDCR[0] và định thời SDTR[0] (SDRAM Bank 1):
     * NC = 8 cols (00b), NR = 12 rows (01b), MWID = 16 bits (01b), NB = 4 banks (1b)
     * CAS Latency = 2 cycles (10b), Write Protection = 0 (Cho phép đọc ghi)
     * SDCLK = HCLK / 2 = 108 MHz (10b), RBURST = 1 (Burst read)
     */
    FMC_SDRAM->SDCR[0] = (0U << 0)  |  /* NC = 8-bit Column */
                         (1U << 2)  |  /* NR = 12-bit Row */
                         (1U << 4)  |  /* MWID = 16-bit Bus */
                         (1U << 6)  |  /* NB = 4 internal banks */
                         (2U << 7)  |  /* CAS Latency = 2 */
                         (2U << 10) |  /* SDCLK = 2 x HCLK (108 MHz) */
                         (1U << 12);   /* Burst Read Enable */

    /* Định thời SDTR (Đơn vị: Chu kỳ xung nhịp 108MHz - 9.26ns):
     * TMRD = 2 cycles (1U)
     * TXSR = 7 cycles (6U)
     * TRAS = 4 cycles (3U)
     * TRC  = 7 cycles (6U)
     * TWR  = 2 cycles (1U)
     * TRP  = 2 cycles (1U)
     * TRCD = 2 cycles (1U)
     */
    FMC_SDRAM->SDTR[0] = (1U << 0)  |
                         (6U << 4)  |
                         (3U << 8)  |
                         (6U << 12) |
                         (1U << 16) |
                         (1U << 20) |
                         (1U << 24);

    /* 5 Bước khởi tạo JEDEC SDRAM chuẩn công nghiệp: */

    /* BƯỚC 1: Lệnh Clock Configuration Enable (Mode = 001b, Bank 1 = bit 4) */
    while (FMC_SDRAM->SDSR & FMC_SDSR_BUSY);
    FMC_SDRAM->SDCMR = (1U << 0) | (1U << 4);
    for (volatile int i = 0; i < 25000; i++); /* Delay > 100us cho điện áp ổn định */

    /* BƯỚC 2: Lệnh PALL (Precharge All Banks) (Mode = 010b, Bank 1 = bit 4) */
    while (FMC_SDRAM->SDSR & FMC_SDSR_BUSY);
    FMC_SDRAM->SDCMR = (2U << 0) | (1U << 4);

    /* BƯỚC 3: Lệnh Auto-Refresh 8 chu kỳ (Mode = 011b, NRFS = 7 (8 chu kỳ), Bank 1 = bit 4) */
    while (FMC_SDRAM->SDSR & FMC_SDSR_BUSY);
    FMC_SDRAM->SDCMR = (3U << 0) | (1U << 4) | (7U << 5);

    /* BƯỚC 4: Lệnh Load Mode Register (Mode = 100b, Bank 1 = bit 4)
     * MRD = 0x0220: Burst Length = 1, Burst Type = Sequential, CAS Latency = 2, Write Burst = Single
     */
    while (FMC_SDRAM->SDSR & FMC_SDSR_BUSY);
    FMC_SDRAM->SDCMR = (4U << 0) | (1U << 4) | (0x0220U << 9);

    /* BƯỚC 5: Cài đặt bộ đếm làm tươi Refresh Counter (COUNT = 1667)
     * Công thức: (64ms / 4096 rows) * 108MHz - 20 = 1666 ~ 1667
     */
    while (FMC_SDRAM->SDSR & FMC_SDSR_BUSY);
    FMC_SDRAM->SDRTR = (1667U << 1);
}

uint8_t SDRAM_Test(void)
{
    volatile uint32_t *pMem = (volatile uint32_t *)SDRAM_BASE_ADDR;
    
    /* Ghi và đọc mẫu kiểm tra tại 4 vị trí phân tán trong 8 MB */
    pMem[0] = 0xA5A55A5A;
    pMem[1024] = 0x12345678;
    pMem[65536] = 0xCAFEBABE;
    pMem[1048576] = 0xDEADBEEF;

    if (pMem[0] != 0xA5A55A5A) return 0;
    if (pMem[1024] != 0x12345678) return 0;
    if (pMem[65536] != 0xCAFEBABE) return 0;
    if (pMem[1048576] != 0xDEADBEEF) return 0;

    return 1; /* SDRAM hoạt động hoàn hảo */
}
