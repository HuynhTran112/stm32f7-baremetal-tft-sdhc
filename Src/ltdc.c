/**
 * @file ltdc.c
 * @brief Cấu hình ngoại vi LTDC, GPIO và Backlight cho STM32F746G-DISCO
 *
 * Tra cứu phần cứng:
 * - Bo mạch: STM32F746G-DISCO (UM1907 Table 7/8/9 và Section 6.13)
 * - Màn hình: Rocktech RK043FN48H-CT672B 4.3" TFT 480x272 24-bit RGB
 * - Tín hiệu điều khiển màn hình:
 *   * PI12: LCD_DISP (Display ON/OFF, Active HIGH)
 *   * PK3:  LCD_BL_CTRL (Backlight Enable, Active HIGH)
 * - Tín hiệu LTDC Timing:
 *   * PI14: LTDC_CLK (AF14)
 *   * PI10: LTDC_HSYNC (AF14)
 *   * PI9:  LTDC_VSYNC (AF14)
 *   * PK7:  LTDC_DE (AF14)
 * - Tín hiệu LTDC Data:
 *   * Red (R0..R7): PI15, PJ0, PJ1, PJ2, PJ3, PJ4, PJ5, PJ6 (AF14)
 *   * Green (G0..G7): PJ7, PJ8, PJ9, PJ10, PJ11, PK0, PK1, PK2 (AF14)
 *   * Blue (B0..B7): PE4 (AF14), PJ13, PJ14, PJ15 (AF14), PG12 (AF9!), PK4, PK5, PK6 (AF14)
 */

#include "ltdc.h"
#include "sdram.h"
#include "reg.h"

static void LTDC_GPIO_Config(void)
{
    /* 1. Bật clock cho các Port GPIO tham gia hiển thị: E, G, I, J, K */
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOGEN |
                     RCC_AHB1ENR_GPIOIEN | RCC_AHB1ENR_GPIOJEN | RCC_AHB1ENR_GPIOKEN);
    (void)RCC->AHB1ENR;

    /* 2. Cấu hình Port E:
     * PE4 = LTDC_B0 -> Alternate Function AF14
     */
    GPIOE->MODER &= ~(3U << (4 * 2));
    GPIOE->MODER |=  (2U << (4 * 2));
    GPIOE->OSPEEDR |= (3U << (4 * 2));
    GPIOE->AFRL &= ~(0xFU << (4 * 4));
    GPIOE->AFRL |=  (14U << (4 * 4));

    /* 3. Cấu hình Port G:
     * PG12 = LTDC_B4 -> Alternate Function AF9 (Đặc biệt: AF9, không phải AF14)
     */
    GPIOG->MODER &= ~(3U << (12 * 2));
    GPIOG->MODER |=  (2U << (12 * 2));
    GPIOG->OSPEEDR |= (3U << (12 * 2));
    GPIOG->AFRH &= ~(0xFU << ((12 - 8) * 4));
    GPIOG->AFRH |=  (9U << ((12 - 8) * 4));

    /* 4. Cấu hình Port I:
     * PI9  = LTDC_VSYNC (AF14)
     * PI10 = LTDC_HSYNC (AF14)
     * PI12 = LCD_DISP  -> Output Push-Pull, High Speed (khởi tạo mức 0)
     * PI14 = LTDC_CLK   (AF14)
     * PI15 = LTDC_R0    (AF14)
     */
    GPIOI->MODER &= ~((3U << 18) | (3U << 20) | (3U << 24) | (3U << 28) | (3U << 30));
    GPIOI->MODER |=  ((2U << 18) | (2U << 20) | (1U << 24) | (2U << 28) | (2U << 30));
    GPIOI->OSPEEDR |= ((3U << 18) | (3U << 20) | (3U << 24) | (3U << 28) | (3U << 30));
    GPIOI->AFRH &= ~((0xFU << 4) | (0xFU << 8) | (0xFU << 16) | (0xFU << 24) | (0xFU << 28));
    GPIOI->AFRH |=  ((14U << 4) | (14U << 8) | (14U << 24) | (14U << 28));
    /* Giữ LCD_DISP mức 0 ban đầu */
    GPIOI->BSRR = (1U << (12 + 16));

    /* 5. Cấu hình Port J:
     * PJ0..PJ11, PJ13..PJ15 đều là AF14 (Red 1-7, Green 0-4, Blue 1-3)
     */
    GPIOJ->MODER = 0xAAAAAAAA;
    GPIOJ->OSPEEDR = 0xFFFFFFFF;
    GPIOJ->AFRL = 0xEEEEEEEE;
    GPIOJ->AFRH = 0xEEEEEEEE;

    /* 6. Cấu hình Port K:
     * PK0..PK2 = LTDC_G5..G7 (AF14)
     * PK3      = LCD_BL_CTRL -> Output Push-Pull, High Speed (khởi tạo mức 0)
     * PK4..PK6 = LTDC_B5..B7 (AF14)
     * PK7      = LTDC_DE     (AF14)
     */
    GPIOK->MODER &= ~0x0000FFFF;
    GPIOK->MODER |= 0xAA6A;
    GPIOK->OSPEEDR |= 0x0000FFFF;
    GPIOK->AFRL &= ~0xFFFFFFFF;
    GPIOK->AFRL |=  0xEEEE0EEE;
    /* Giữ LCD_BL_CTRL mức 0 ban đầu (tắt đèn nền trong lúc cấu hình) */
    GPIOK->BSRR = (1U << (3 + 16));
}

void LTDC_Init(void)
{
    /* 1. Cấu hình GPIO chân kết nối LCD */
    LTDC_GPIO_Config();

    /* 2. Cấp xung nhịp cho khối LTDC trên APB2 */
    RCC->APB2ENR |= RCC_APB2ENR_LTDCEN;
    (void)RCC->APB2ENR;

    /* 3. Cấu hình các tham số định thời màn hình (RM0385 Sec 18.4 & Rocktech RK043FN48H)
     * HSYNC = 41, HBP = 13, ActiveW = 480, HFP = 32
     * VSYNC = 10, VBP = 2,  ActiveH = 272, VFP = 2
     *
     * SSCR: HSW = 41 - 1 = 40, VSH = 10 - 1 = 9
     * BPCR: AHBP = 41 + 13 - 1 = 53, AVBP = 10 + 2 - 1 = 11
     * AWCR: AAW = 53 + 480 = 533, AAH = 11 + 272 = 283
     * TWCR: TOTALW = 533 + 32 = 565, TOTALH = 283 + 2 = 285
     */
    LTDC->SSCR = (40U << 16) | 9U;
    LTDC->BPCR = (53U << 16) | 11U;
    LTDC->AWCR = (533U << 16) | 283U;
    LTDC->TWCR = (565U << 16) | 285U;

    /* 4. Cấu hình màu nền (Background Color): Màu đen */
    LTDC->BCCR = 0x00000000UL;

    /* 5. Cấu hình Layer 1 */
    /* 5.1 Vị trí cửa sổ hiển thị (Window Horizontal / Vertical Position) */
    /* Start = AHBP + 1 = 54, Stop = AAW = 533 */
    LTDC_Layer1->WHPCR = (533U << 16) | 54U;
    /* Start = AVBP + 1 = 12, Stop = AAH = 283 */
    LTDC_Layer1->WVPCR = (283U << 16) | 12U;

    /* 5.2 Định dạng pixel: RGB565 (PFCR = 010b = 2U) */
    LTDC_Layer1->PFCR = 2U;

    /* 5.3 Nạp địa chỉ Framebuffer khởi tạo (SDRAM Bank 1) */
    LTDC_Layer1->CFBAR = LCD_FRAMEBUFFER_0;

    /* 5.4 Độ dài dòng và bước nhảy Pitch (CFBLR)
     * Bits 28:16: CFBP = Pitch (bytes) = 480 * 2 = 960
     * Bits 12:0:  CFBLL = Line Length (bytes) + 3 = 480 * 2 + 3 = 963
     */
    LTDC_Layer1->CFBLR = ((LCD_WIDTH * 2U) << 16) | (LCD_WIDTH * 2U + 3U);

    /* 5.5 Số dòng quét (Line Number): 272 dòng */
    LTDC_Layer1->CFBLNR = LCD_HEIGHT;

    /* 5.6 Độ mờ Constant Alpha = 255 (hoàn toàn đục, hiển thị 100%) */
    LTDC_Layer1->CACR = 255U;

    /* 5.7 Màu mặc định ngoài khung: 0 */
    LTDC_Layer1->DCCR = 0U;

    /* 5.8 Hệ số hòa trộn Alpha (Blending Factors):
     * BF1 = 100b (Constant Alpha = 255)
     * BF2 = 101b (1 - Constant Alpha = 0)
     */
    LTDC_Layer1->BFCR = (4U << 8) | 5U;

    /* 5.9 Kích hoạt Layer 1 */
    LTDC_Layer1->CR |= LTDC_LxCR_LEN;

    /* 6. Nạp bóng cấu hình thanh ghi ngay lập tức (Immediate Reload) */
    LTDC->SRCR = LTDC_SRCR_IMR;

    /* 7. Trình tự bật nguồn màn hình chuẩn Rocktech (Power-On Sequence):
     * Bước 7.1: Kéo LCD_DISP = 1 để cấp nguồn màn hình panel LCD
     */
    GPIOI->BSRR = (1U << 12);
    for (volatile int i = 0; i < 50000; i++);

    /* Bước 7.2: Kích hoạt bộ điều khiển LTDC phát tín hiệu quét */
    LTDC->GCR |= LTDC_GCR_LTDCEN;
    LTDC->SRCR = LTDC_SRCR_IMR;
    for (volatile int i = 0; i < 50000; i++);

    /* Bước 7.3: Bật đèn nền Backlight LCD_BL_CTRL = 1 sau khi tín hiệu quét đã ổn định */
    GPIOK->BSRR = (1U << 3);
}

void LTDC_SetAddress(uint32_t framebuffer_address)
{
    LTDC_Layer1->CFBAR = framebuffer_address;
    LTDC->SRCR = LTDC_SRCR_IMR;
}

void LTDC_SwapBuffers_VBlank(uint32_t new_framebuffer_address)
{
    LTDC_Layer1->CFBAR = new_framebuffer_address;
    /* Vertical Blanking Reload: Chờ tia quét hết khung hình mới nạp để triệt tiêu xé hình */
    LTDC->SRCR = LTDC_SRCR_VBR;
}

void LCD_Clear(uint32_t fb_address, uint16_t color_rgb565)
{
    volatile uint16_t *ptr = (volatile uint16_t *)fb_address;
    for (uint32_t i = 0; i < (LCD_WIDTH * LCD_HEIGHT); i++)
    {
        ptr[i] = color_rgb565;
    }
}
