/**
 * @file reg.h
 * @brief Định nghĩa thanh ghi Bare-Metal STM32F746NGH6 (ARM Cortex-M7)
 * @note Không sử dụng thư viện HAL/LL của STMicroelectronics.
 */

#ifndef REG_H
#define REG_H

#include <stdint.h>

/* --- Địa chỉ cơ sở ngoại vi (Base Addresses) --- */
#define PERIPH_BASE         0x40000000UL
#define APB1PERIPH_BASE     PERIPH_BASE
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define AHB3PERIPH_BASE     0xA0000000UL

/* GPIO Base */
#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE          (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE          (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE          (AHB1PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE          (AHB1PERIPH_BASE + 0x1000UL)
#define GPIOF_BASE          (AHB1PERIPH_BASE + 0x1400UL)
#define GPIOG_BASE          (AHB1PERIPH_BASE + 0x1800UL)
#define GPIOH_BASE          (AHB1PERIPH_BASE + 0x1C00UL)
#define GPIOI_BASE          (AHB1PERIPH_BASE + 0x2000UL)
#define GPIOJ_BASE          (AHB1PERIPH_BASE + 0x2400UL)
#define GPIOK_BASE          (AHB1PERIPH_BASE + 0x2800UL)

/* Core Peripherals */
#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800UL)
#define FLASH_R_BASE        (AHB1PERIPH_BASE + 0x3C00UL)
#define DMA2D_BASE          (AHB1PERIPH_BASE + 0xB000UL)
#define PWR_BASE            (APB1PERIPH_BASE + 0x7000UL)
#define SDMMC1_BASE         (APB2PERIPH_BASE + 0x2C00UL)
#define LTDC_BASE           (APB2PERIPH_BASE + 0x6800UL)
#define FMC_R_BASE          AHB3PERIPH_BASE
#define I2C3_BASE           (APB1PERIPH_BASE + 0x5C00UL)
#define SAI2_BASE           (APB2PERIPH_BASE + 0x5C00UL)
#define DMA2_BASE           (AHB1PERIPH_BASE + 0x6400UL)
#define DMA2_Stream4_BASE   (DMA2_BASE + 0x70UL)

/* System Control Space (ARM Cortex-M7) */
#define SCS_BASE            0xE000E000UL
#define SYSTICK_BASE        (SCS_BASE + 0x0010UL)
#define NVIC_BASE           (SCS_BASE + 0x0100UL)
#define SCB_BASE            (SCS_BASE + 0x0D00UL)

/* --- Struct thanh ghi --- */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFRL;
    volatile uint32_t AFRH;
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
    uint32_t RESERVED3[2];
    volatile uint32_t AHB1LPENR;
    volatile uint32_t AHB2LPENR;
    volatile uint32_t AHB3LPENR;
    uint32_t RESERVED4;
    volatile uint32_t APB1LPENR;
    volatile uint32_t APB2LPENR;
    uint32_t RESERVED5[2];
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
    uint32_t RESERVED6[2];
    volatile uint32_t SSCGR;
    volatile uint32_t PLLI2SCFGR;
    volatile uint32_t PLLSAICFGR;
    volatile uint32_t DCKCFGR1;
    volatile uint32_t DCKCFGR2;
} RCC_TypeDef;

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CSR1;
    volatile uint32_t CR2;
    volatile uint32_t CSR2;
} PWR_TypeDef;

typedef struct {
    volatile uint32_t ACR;
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t OPTCR;
    volatile uint32_t OPTCR1;
} FLASH_TypeDef;

typedef struct {
    volatile uint32_t SDCR[2];
    volatile uint32_t SDTR[2];
    volatile uint32_t SDCMR;
    volatile uint32_t SDRTR;
    volatile uint32_t SDSR;
} FMC_SDRAM_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t WHPCR;
    volatile uint32_t WVPCR;
    volatile uint32_t CKCR;
    volatile uint32_t PFCR;
    volatile uint32_t CACR;
    volatile uint32_t DCCR;
    volatile uint32_t BFCR;
    uint32_t RESERVED[2];
    volatile uint32_t CFBAR;
    volatile uint32_t CFBLR;
    volatile uint32_t CFBLNR;
    uint32_t RESERVED1[3];
    volatile uint32_t CLUTWR;
} LTDC_Layer_TypeDef;

typedef struct {
    uint32_t RESERVED0[2];
    volatile uint32_t SSCR;
    volatile uint32_t BPCR;
    volatile uint32_t AWCR;
    volatile uint32_t TWCR;
    volatile uint32_t GCR;
    uint32_t RESERVED1[2];
    volatile uint32_t SRCR;
    uint32_t RESERVED2;
    volatile uint32_t BCCR;
    uint32_t RESERVED3;
    volatile uint32_t IER;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t LIPCR;
    volatile uint32_t CPSR;
    volatile uint32_t CDSR;
} LTDC_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t ISR;
    volatile uint32_t IFCR;
    volatile uint32_t FGMAR;
    volatile uint32_t FGOR;
    volatile uint32_t BGMAR;
    volatile uint32_t BGOR;
    volatile uint32_t FGPFCCR;
    volatile uint32_t FGCOLR;
    volatile uint32_t BGPFCCR;
    volatile uint32_t BGCOLR;
    volatile uint32_t FGCLUT;
    volatile uint32_t BGCLUT;
    volatile uint32_t OPFCCR;
    volatile uint32_t OCOLR;
    volatile uint32_t OMAR;
    volatile uint32_t OOR;
    volatile uint32_t NLR;
    volatile uint32_t LWR;
    volatile uint32_t AMTCR;
} DMA2D_TypeDef;

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t TIMINGR;
    volatile uint32_t TIMEOUTR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t PECR;
    volatile uint32_t RXDR;
    volatile uint32_t TXDR;
} I2C_TypeDef;

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t FRCR;
    volatile uint32_t SLOTR;
    volatile uint32_t IMR;
    volatile uint32_t SR;
    volatile uint32_t CLRFR;
    volatile uint32_t DR;
} SAI_Block_TypeDef;

typedef struct {
    volatile uint32_t GCR;
    SAI_Block_TypeDef Block_A;
    SAI_Block_TypeDef Block_B;
} SAI_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t NDTR;
    volatile uint32_t PAR;
    volatile uint32_t M0AR;
    volatile uint32_t M1AR;
    volatile uint32_t FCR;
} DMA_Stream_TypeDef;

typedef struct {
    volatile uint32_t LISR;
    volatile uint32_t HISR;
    volatile uint32_t LIFCR;
    volatile uint32_t HIFCR;
} DMA_TypeDef;

typedef struct {
    volatile uint32_t POWER;
    volatile uint32_t CLKCR;
    volatile uint32_t ARG;
    volatile uint32_t CMD;
    volatile uint32_t RESPCMD;
    volatile uint32_t RESP1;
    volatile uint32_t RESP2;
    volatile uint32_t RESP3;
    volatile uint32_t RESP4;
    volatile uint32_t DTIMER;
    volatile uint32_t DLEN;
    volatile uint32_t DCTRL;
    volatile uint32_t DCOUNT;
    volatile uint32_t STA;
    volatile uint32_t ICR;
    volatile uint32_t MASK;
    uint32_t RESERVED0[2];
    volatile uint32_t FIFOCNT;
    uint32_t RESERVED1[13];
    volatile uint32_t FIFO;
} SDMMC_TypeDef;

typedef struct {
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint32_t SHPR[3];
    volatile uint32_t SHCSR;
    volatile uint32_t CFSR;
    volatile uint32_t HFSR;
    volatile uint32_t DFSR;
    volatile uint32_t MMFAR;
    volatile uint32_t BFAR;
    volatile uint32_t AFSR;
    uint32_t RESERVED0[18];
    volatile uint32_t CSSELR;
    volatile uint32_t CCSIDR;
    uint32_t RESERVED1;
    volatile uint32_t ICIALLU;
    uint32_t RESERVED2;
    volatile uint32_t ICIMVAU;
    volatile uint32_t DCIMVAC;
    volatile uint32_t DCISW;
    volatile uint32_t DCCMVAU;
    volatile uint32_t DCCMVAC;
    volatile uint32_t DCCSW;
    volatile uint32_t DCCIMVAC;
    volatile uint32_t DCCISW;
} SCB_TypeDef;

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_TypeDef;

/* Peripheral Pointers */
#define GPIOA               ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE               ((GPIO_TypeDef *) GPIOE_BASE)
#define GPIOF               ((GPIO_TypeDef *) GPIOF_BASE)
#define GPIOG               ((GPIO_TypeDef *) GPIOG_BASE)
#define GPIOH               ((GPIO_TypeDef *) GPIOH_BASE)
#define GPIOI               ((GPIO_TypeDef *) GPIOI_BASE)
#define GPIOJ               ((GPIO_TypeDef *) GPIOJ_BASE)
#define GPIOK               ((GPIO_TypeDef *) GPIOK_BASE)

#define RCC                 ((RCC_TypeDef *) RCC_BASE)
#define PWR                 ((PWR_TypeDef *) PWR_BASE)
#define FLASH               ((FLASH_TypeDef *) FLASH_R_BASE)

#define FMC_SDRAM           ((FMC_SDRAM_TypeDef *) (FMC_R_BASE + 0x0140UL))
#define LTDC                ((LTDC_TypeDef *) LTDC_BASE)
#define LTDC_Layer1         ((LTDC_Layer_TypeDef *) (LTDC_BASE + 0x84UL))
#define DMA2D               ((DMA2D_TypeDef *) DMA2D_BASE)
#define SDMMC1              ((SDMMC_TypeDef *) SDMMC1_BASE)
#define I2C3                ((I2C_TypeDef *) I2C3_BASE)
#define SAI2                ((SAI_TypeDef *) SAI2_BASE)
#define SAI2_Block_A        (&(SAI2->Block_A))
#define SAI2_Block_B        (&(SAI2->Block_B))
#define DMA2                ((DMA_TypeDef *) DMA2_BASE)
#define DMA2_Stream4        ((DMA_Stream_TypeDef *) DMA2_Stream4_BASE)

#define SCB                 ((SCB_TypeDef *) SCB_BASE)
#define SysTick             ((SysTick_TypeDef *) SYSTICK_BASE)

/* RCC Bit Masks */
#define RCC_CR_HSEON                (1U << 16)
#define RCC_CR_HSERDY               (1U << 17)
#define RCC_CR_PLLON                (1U << 24)
#define RCC_CR_PLLRDY               (1U << 25)
#define RCC_CR_PLLSAION             (1U << 28)
#define RCC_CR_PLLSAIRDY            (1U << 29)
#define RCC_CR_PLLI2SON             (1U << 26)
#define RCC_CR_PLLI2SRDY            (1U << 27)
#define RCC_APB1ENR_I2C3EN          (1U << 23)
#define RCC_APB2ENR_SAI2EN          (1U << 23)
#define RCC_AHB1ENR_DMA2EN          (1U << 22)

#define RCC_PLLCFGR_PLLSRC_HSE      (1U << 22)

#define RCC_CFGR_SW_PLL             (2U << 0)
#define RCC_CFGR_SWS_PLL            (2U << 2)

#define RCC_AHB1ENR_GPIOAEN         (1U << 0)
#define RCC_AHB1ENR_GPIOBEN         (1U << 1)
#define RCC_AHB1ENR_GPIOCEN         (1U << 2)
#define RCC_AHB1ENR_GPIODEN         (1U << 3)
#define RCC_AHB1ENR_GPIOEEN         (1U << 4)
#define RCC_AHB1ENR_GPIOFEN         (1U << 5)
#define RCC_AHB1ENR_GPIOGEN         (1U << 6)
#define RCC_AHB1ENR_GPIOHEN         (1U << 7)
#define RCC_AHB1ENR_GPIOIEN         (1U << 8)
#define RCC_AHB1ENR_GPIOJEN         (1U << 9)
#define RCC_AHB1ENR_GPIOKEN         (1U << 10)
#define RCC_AHB1ENR_DMA2DEN         (1U << 23)

#define RCC_AHB3ENR_FMCEN           (1U << 0)

#define RCC_APB1ENR_PWREN           (1U << 28)

#define RCC_APB2ENR_SDMMC1EN        (1U << 11)
#define RCC_APB2ENR_LTDCEN          (1U << 26)

/* PWR Bit Masks */
#define PWR_CR1_VOS_SCALE1          (3U << 14)
#define PWR_CR1_ODEN                (1U << 16)
#define PWR_CR1_ODSWEN              (1U << 17)
#define PWR_CSR1_ODRDY              (1U << 16)
#define PWR_CSR1_ODSWRDY            (1U << 17)

/* FLASH Bit Masks */
#define FLASH_ACR_LATENCY_7WS       (7U << 0)
#define FLASH_ACR_PRFTEN            (1U << 8)
#define FLASH_ACR_ARTEN             (1U << 9)

/* FMC SDRAM Bit Masks */
#define FMC_SDSR_BUSY               (1U << 5)

/* LTDC Bit Masks */
#define LTDC_GCR_LTDCEN             (1U << 0)
#define LTDC_LxCR_LEN               (1U << 0)
#define LTDC_SRCR_IMR               (1U << 0)
#define LTDC_SRCR_VBR               (1U << 1)

/* DMA2D Bit Masks */
#define DMA2D_CR_START              (1U << 0)
#define DMA2D_ISR_TCIF              (1U << 1)
#define DMA2D_IFCR_CTCIF            (1U << 1)

/* SCB Cache Masks */
#define SCB_CCR_IC                  (1U << 17)
#define SCB_CCR_DC                  (1U << 16)

#endif /* REG_H */
