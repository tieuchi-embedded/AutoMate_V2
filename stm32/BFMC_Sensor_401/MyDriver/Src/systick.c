/*
 * systick.c
 *
 *  Created on: Feb 14, 2026
 *      Author: Admin
 */

#include "mydriver.h"

#define SYSTICK_BASE  0xE000E010UL

#define SYST_CSR   (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYST_RVR   (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYST_CVR   (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

volatile uint32_t ms_tick = 0;

void SysTick_Init(uint32_t cpu_freq)
{
    uint32_t reload = (cpu_freq / 1000) - 1;

    SYST_RVR = reload;      // Set reload value
    SYST_CVR = 0;           // Clear current value

    SYST_CSR = (1 << 2) |   // CLKSOURCE = CPU
               (1 << 0);    // ENABLE
}

void Delay_t(uint32_t ms)
{
	while(ms--)
	{
		while(((SYST_CSR >>16) &1) ==0);
	}
}

