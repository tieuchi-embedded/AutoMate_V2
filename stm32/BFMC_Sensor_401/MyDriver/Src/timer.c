/*
 * timer.c
 *
 *  Created on: Jan 28, 2026
 *      Author: Admin
 */

#include "mydriver.h"

#define RCC_BASE_ADDR      0x40023800
#define GPIOC_BASE_ADDR    0x40020800
#define TIM2_BASE_ADDR     0x40000000
#define GPIOA_BASE_ADDR    0x40020000
#define TIM3_BASE_ADDR     0x40000400

void Timer2_Init()
{
    uint32_t* RCC_APB1ENR = (uint32_t*)(RCC_BASE_ADDR + 0x40);
	uint16_t* TIM2_PSC = (uint16_t*)(TIM2_BASE_ADDR + 0x28);
	uint32_t* TIM2_ARR = (uint32_t*)(TIM2_BASE_ADDR + 0x2C);
	uint16_t* TIM2_CR1 = (uint16_t*)(TIM2_BASE_ADDR + 0x00);

	*RCC_APB1ENR |= (1 << 0);   // ENABLE TIM2 CLOCK
	*TIM2_PSC = 16;				// PRESCALER
	*TIM2_ARR = 1;				// AUTO RELOAD
	*TIM2_CR1 |= (1 << 0);		// ENABLE TIM2
}

void Timer3_Init(void)
{
    uint32_t* RCC_APB1ENR = (uint32_t*)(RCC_BASE_ADDR + 0x40);
    uint32_t* TIM3_PSC = (uint32_t*)(TIM3_BASE_ADDR + 0x28);
    uint32_t* TIM3_ARR = (uint32_t*)(TIM3_BASE_ADDR + 0x2C);
    uint32_t* TIM3_CR1 = (uint32_t*)(TIM3_BASE_ADDR + 0x00);
    uint32_t* TIM3_EGR = (uint32_t*)(TIM3_BASE_ADDR + 0x14);
    uint32_t* TIM3_SR  = (uint32_t*)(TIM3_BASE_ADDR + 0x10);

    *RCC_APB1ENR |= (1 << 1);   // TIM3EN
    *TIM3_CR1 = 0;
    *TIM3_PSC = 84 - 1;         // 1 MHz
    *TIM3_ARR = 1000 - 1;       // 1 ms
    *TIM3_EGR = 1;              // LOAD PSC, ARR
    *TIM3_SR &= ~1;             // CLEAR UIF
    *TIM3_CR1 |= 1;             // CEN
}

