/*
 * servo.c
 *
 *  Created on: Jan 28, 2026
 *      Author: Admin
 */

#include "mydriver.h"

#define RCC_BASE_ADDR      0x40023800
#define GPIOA_BASE_ADDR    0x40020000
#define TIM2_BASE_ADDR     0x40000000


void PWM_Init(void)
{
    volatile uint32_t* RCC_AHB1ENR = (uint32_t*)(RCC_BASE_ADDR + 0x30);
    volatile uint32_t* RCC_APB1ENR = (uint32_t*)(RCC_BASE_ADDR + 0x40);

    *RCC_AHB1ENR |= (1 << 0);     // GPIOAEN
    *RCC_APB1ENR |= (1 << 0);     // TIM2EN

    volatile uint32_t* GPIOA_MODER = (uint32_t*)(GPIOA_BASE_ADDR + 0x00);
    volatile uint32_t* GPIOA_AFRL  = (uint32_t*)(GPIOA_BASE_ADDR + 0x20);

    *GPIOA_MODER &= ~(0b11 << (0 * 2));  // clear mode PA0
    *GPIOA_MODER |=  (0b10 << (0 * 2));  // set AF mode
    *GPIOA_AFRL  &= ~(0xF << (0 * 4));
    *GPIOA_AFRL  |=  (0x1 << (0 * 4));   // AF1 (TIM2_CH1)

    *GPIOA_MODER &= ~(0b11 << (1 * 2));
    *GPIOA_MODER |=  (0b10 << (1 * 2));
    *GPIOA_AFRL  &= ~(0xF << (1 * 4));
    *GPIOA_AFRL  |=  (0x1 << (1 * 4));   // AF1

    volatile uint32_t* TIM2_PSC   = (uint32_t*)(TIM2_BASE_ADDR + 0x28);
    volatile uint32_t* TIM2_ARR   = (uint32_t*)(TIM2_BASE_ADDR + 0x2C);
    volatile uint32_t* TIM2_CCR1  = (uint32_t*)(TIM2_BASE_ADDR + 0x34);
    volatile uint32_t* TIM2_CCR2  = (uint32_t*)(TIM2_BASE_ADDR + 0x38);
    volatile uint32_t* TIM2_CCMR1 = (uint32_t*)(TIM2_BASE_ADDR + 0x18);
    volatile uint32_t* TIM2_CCER  = (uint32_t*)(TIM2_BASE_ADDR + 0x20);
    volatile uint32_t* TIM2_CR1   = (uint32_t*)(TIM2_BASE_ADDR + 0x00);
    volatile uint32_t* TIM2_EGR   = (uint32_t*)(TIM2_BASE_ADDR + 0x14);

    *TIM2_PSC = 16 - 1;     // chia 16MHz/16 = 1MHz tick
    *TIM2_ARR = 20000-1;       // chu kỳ 1000 tick => PWM 1kHz
    *TIM2_CCR1 = 1500;        // Servo speed stop (1.5 ms)
    *TIM2_CCR2 = 1500;        // Servo steering center


    // ==== 4. Chọn PWM mode 1, bật preload ====
    *TIM2_CCMR1 &= ~((0xFF << 0) | (0xFF << 8));
    // CH1
    *TIM2_CCMR1 |= (6 << 4) | (1 << 3);   // OC1M, OC1PE
    // CH2
    *TIM2_CCMR1 |= (6 << 12) | (1 << 11); // OC2M, OC2PE

    *TIM2_CCER |= (1 << 0);   // CC1E
    *TIM2_CCER |= (1 << 4);   // CC2E

    *TIM2_EGR = 1;            // UG=1

    *TIM2_CR1 |= (1 << 0);    // CEN=1

}

void Servo_Speed_Set(uint16_t us)
{
    volatile uint32_t* TIM2_CCR1 = (uint32_t*)(TIM2_BASE_ADDR + 0x34);
    *TIM2_CCR1 = us;
    UART2_print_log("@speed:");
    UART2_send_float(us);
}

void Servo_Steering_Set(uint16_t us)
{
    volatile uint32_t* TIM2_CCR2 = (uint32_t*)(TIM2_BASE_ADDR + 0x38);
    *TIM2_CCR2 = us;
    UART2_print_log("@steer:");
    UART2_send_float(us);
}
