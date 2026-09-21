/**
 * @file sys_clock.c
 * @brief Triển khai hệ thống Clock 216MHz, Over-Drive và SysTick Delay
 */

#include "sys_clock.h"
#include "reg.h"

static volatile uint32_t s_ticks = 0;

void SysTick_Handler(void)
{
    s_ticks++;
}

uint32_t Get_Tick_ms(void)
{
    return s_ticks;
}

void Delay_ms(uint32_t ms)
{
    uint32_t start = s_ticks;
    while ((s_ticks - start) < ms);
}

void SysTick_Init(void)
{
    /* Reload cho 1ms với HCLK = 216MHz: 216000 - 1 */
    SysTick->LOAD = (216000000UL / 1000UL) - 1UL;
    SysTick->VAL = 0UL;
    /* CLKSOURCE = Processor clock (bit 2), TICKINT = Enable (bit 1), ENABLE = 1 (bit 0) */
    SysTick->CTRL = (1U << 2) | (1U << 1) | (1U << 0);
}

void CPU_Cache_Enable(void)
{
    /* 1. Bat Instruction Cache (I-Cache) giup CPU chay 216 MHz toi da */
    SCB->ICIALLU = 0UL;
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");
    SCB->CCR |= SCB_CCR_IC;
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");

    /* 2. Tat D-Cache de tranh xung dot bo nho ngoai SDRAM & DMA2D */
    SCB->CCR &= ~SCB_CCR_DC;
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");
}

void SysClock_Init(void)
{
    /* 1. Bật Power interface clock */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;

    /* 2. Cấu hình Voltage Output Scaling = Scale 1 (Cần thiết cho 216MHz) */
    PWR->CR1 |= PWR_CR1_VOS_SCALE1;

    /* 3. Bật thạch anh ngoài HSE (25 MHz trên STM32F746G-DISCO) */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* 4. Cấu hình bộ chia PLL:
     * f_VCO_in  = HSE / PLLM = 25 / 25 = 1 MHz
     * f_VCO_out = f_VCO_in * PLLN = 1 * 432 = 432 MHz
     * f_SYSCLK  = f_VCO_out / PLLP = 432 / 2 = 216 MHz
     * f_PLLQ    = 432 / 9 = 48 MHz (cho USB / SDMMC)
     */
    RCC->PLLCFGR = (25U << 0)               |  /* PLLM = 25 */
                   (432U << 6)              |  /* PLLN = 432 */
                   (0U << 16)               |  /* PLLP = 2 (00b) */
                   (RCC_PLLCFGR_PLLSRC_HSE) |  /* Source = HSE */
                   (9U << 24);                 /* PLLQ = 9 */

    /* 5. Bật PLL chính và chờ ổn định */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* 6. Kích hoạt chế độ Over-Drive (Bắt buộc để chạy 216MHz theo RM0385 Sec 5.1.4) */
    PWR->CR1 |= PWR_CR1_ODEN;
    while (!(PWR->CSR1 & PWR_CSR1_ODRDY));

    PWR->CR1 |= PWR_CR1_ODSWEN;
    while (!(PWR->CSR1 & PWR_CSR1_ODSWRDY));

    /* 7. Cấu hình Flash Latency = 7 Wait States (cho 216MHz ở điện áp 3.3V) + Bật Prefetch & ART */
        /* 7. Cau hinh Flash Latency = 7 Wait States (cho 216MHz o dien ap 3.3V) + Bat Prefetch & ART */
    FLASH->ACR = FLASH_ACR_LATENCY_7WS | FLASH_ACR_PRFTEN | FLASH_ACR_ARTEN;
    while ((FLASH->ACR & 0x0FU) != FLASH_ACR_LATENCY_7WS);
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");

    /* 8. Cấu hình bộ chia Bus:
     * HCLK  = SYSCLK / 1 = 216 MHz (AHB Prescaler = 1)
     * PCLK1 = HCLK / 4   = 54 MHz  (APB1 Prescaler = 4, max 54MHz)
     * PCLK2 = HCLK / 2   = 108 MHz (APB2 Prescaler = 2, max 108MHz)
     */
    RCC->CFGR &= ~((0xFU << 4) | (7U << 10) | (7U << 13));
    RCC->CFGR |= (0U << 4) | (5U << 10) | (4U << 13);

    /* 9. Chuyển SYSCLK sang nguồn PLL */
    RCC->CFGR &= ~3U;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & (3U << 2)) != RCC_CFGR_SWS_PLL);

    /* 10. Cấu hình PLLSAI cấp pixel clock ~9.6 MHz cho LTDC:
     * f_VCO_SAI = (25 / 25) * 192 = 192 MHz
     * f_PLLSAI_R = 192 / 5 = 38.4 MHz
     * Bộ chia DCKCFGR1 PLLSAIDIVR = div 4 -> 38.4 / 4 = 9.6 MHz (chuẩn 480x272 @ 60Hz)
     */
    RCC->PLLSAICFGR = (192U << 6) | (5U << 28);
    RCC->DCKCFGR1 &= ~(3U << 16);
    RCC->DCKCFGR1 |= (1U << 16);

    RCC->CR |= RCC_CR_PLLSAION;
    while (!(RCC->CR & RCC_CR_PLLSAIRDY));

    /* 11. Khởi tạo bộ đếm thời gian SysTick */
    SysTick_Init();

    /* 12. Kích hoạt L1 I-Cache và D-Cache */
    // CPU_Cache_Enable();
}
