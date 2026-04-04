/*
 * watchdog.c
 *
 *  Created on: Feb 5, 2026
 *      Author: Admin
 */

#include "mydriver.h"

#define IWDG_BASE_ADDR 0x40003000

void Watchdog_Init()
{
    volatile uint32_t* IWDG_KR  = (uint32_t*)(IWDG_BASE_ADDR + 0x00);
    volatile uint32_t* IWDG_PR  = (uint32_t*)(IWDG_BASE_ADDR + 0x04);
    volatile uint32_t* IWDG_RLR = (uint32_t*)(IWDG_BASE_ADDR + 0x08);
    volatile uint32_t* IWDG_SR  = (uint32_t*)(IWDG_BASE_ADDR + 0x0C);


    *IWDG_KR = 0x5555; 				//Unlock
    while (*IWDG_SR & (1 << 0));	//Wait prescaler ready
    *IWDG_PR = 0x06;    			// Prescaler /256
    while (*IWDG_SR & (1 << 1));	//Wait reload ready
    *IWDG_RLR = 600;				//Set reload ~16s
    *IWDG_KR = 0xAAAA;				//Reload counter
    *IWDG_KR = 0xCCCC;				//Enable watchdog
}

void Feed_WD()
{
	volatile uint32_t* IWDG_KR = (uint32_t*)(IWDG_BASE_ADDR + 0x00);
	*IWDG_KR = 0xAAAA;				//Reload watchdog counter
}
