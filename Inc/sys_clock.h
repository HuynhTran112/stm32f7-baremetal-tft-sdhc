/**
 * @file sys_clock.h
 * @brief Cấu hình hệ thống Xung nhịp 216MHz, Cache và SysTick cho STM32F746
 */

#ifndef SYS_CLOCK_H
#define SYS_CLOCK_H

#include <stdint.h>

void SysClock_Init(void);
void CPU_Cache_Enable(void);
void SysTick_Init(void);
void Delay_ms(uint32_t ms);
uint32_t Get_Tick_ms(void);

#endif /* SYS_CLOCK_H */
