/*
 * i2c.c
 *
 *  Created on: Dec 19, 2025
 *      Author: Admin
 */
#include "mydriver.h"

#define RCC_BASE_ADDR        0x40023800
#define GPIOA_BASE_ADDR      0x40020000
#define GPIOB_BASE_ADDR      0x40020400
#define GPIOD_BASE_ADDR      0x40020C00
#define I2C1_BASE_ADDR       0x40005400

void I2C1_Master_Init()
{
    volatile uint32_t* RCC_AHB1ENR = (uint32_t*)(RCC_BASE_ADDR + 0x30);
    volatile uint32_t* RCC_APB1ENR = (uint32_t*)(RCC_BASE_ADDR + 0x40);

    volatile uint32_t* MODER   = (uint32_t*)(GPIOB_BASE_ADDR + 0x00);
    volatile uint32_t* OTYPER  = (uint32_t*)(GPIOB_BASE_ADDR + 0x04);
    volatile uint32_t* OSPEEDR = (uint32_t*)(GPIOB_BASE_ADDR + 0x08);
    volatile uint32_t* PUPDR   = (uint32_t*)(GPIOB_BASE_ADDR + 0x0C);
    volatile uint32_t* AFRH    = (uint32_t*)(GPIOB_BASE_ADDR + 0x24);

    volatile uint32_t* CR1   = (uint32_t*)(I2C1_BASE_ADDR + 0x00);
    volatile uint32_t* CR2   = (uint32_t*)(I2C1_BASE_ADDR + 0x04);
    volatile uint32_t* CCR   = (uint32_t*)(I2C1_BASE_ADDR + 0x1C);
    volatile uint32_t* TRISE = (uint32_t*)(I2C1_BASE_ADDR + 0x20);

    *RCC_AHB1ENR |= (1 << 1);		// ENABLE GPIOB CLOCK
    *RCC_APB1ENR |= (1 << 21); 		// ENABLE I2C1 CLOCK

    *MODER &= ~((0b11<<(8*2)) | (0b11<<(9*2)));
    *MODER |=  ((0b10<<(8*2)) | (0b10<<(9*2)));  // AF
    *OTYPER |= (1<<8) | (1<<9);                  // OD
    *OSPEEDR |= (0b11<<(8*2)) | (0b11<<(9*2));   // High
    *PUPDR &= ~((0b11<<(8*2)) | (0b11<<(9*2)));
    *PUPDR |=  ((0b01<<(8*2)) | (0b01<<(9*2)));  // Pull-up

    *AFRH &= ~(0xF<<(0*4));
    *AFRH |= (0x4<<(0*4));
    *AFRH &= ~(0xF<<(1*4));
    *AFRH |= (0x4<<(1*4));

    *CR1 &= ~(1<<0);   // PE=0
    *CR2 = 16;         // PCLK1=16MHz
    *CCR = 80;         // 100kHz
    *TRISE = 17;       // 1000ns @16MHz
    *CR1 |= (1<<10);   // ACK
    *CR1 |= (1<<0);    // PE=1
}

void I2C1_WriteReg(uint8_t dev, uint8_t reg, uint8_t data)
{
    volatile uint32_t *CR1 = (uint32_t*)(I2C1_BASE_ADDR + 0x00);
    volatile uint32_t *SR1 = (uint32_t*)(I2C1_BASE_ADDR + 0x14);
    volatile uint32_t *SR2 = (uint32_t*)(I2C1_BASE_ADDR + 0x18);
    volatile uint32_t *DR  = (uint32_t*)(I2C1_BASE_ADDR + 0x10);

    *CR1 |= (1<<8);                    // START
    while(!(*SR1 & 1));                // SB

    *DR = dev << 1;                    // Write
    for (int i = 0; i < 1000; i++) {
        if (((*SR1 >> 1) &1) ==1) {
            (void)*SR2;
            break;
        }
        if (i == 999)
            goto i2c_error;
    }

    while(!(*SR1 & (1<<7)));	// TxE
    *DR = reg;					// REGISTER
    while(!(*SR1 & (1<<7)));
    *DR = data;					// DATA
    while(!(*SR1 & (1<<2)));    // BTF
    *CR1 |= (1<<9);             // STOP
    return;

    //I2C ERROR
    i2c_error:
    *SR1 &= ~(1<<10);			// CLEAR AF
    *CR1 |= (1<<9);				// STOP
    return;
}
void I2C1_ReadMulti(uint8_t dev, uint8_t reg, uint8_t *buf, uint8_t len)
{
    volatile uint32_t *CR1 = (uint32_t*)(I2C1_BASE_ADDR + 0x00);
    volatile uint32_t *SR1 = (uint32_t*)(I2C1_BASE_ADDR + 0x14);
    volatile uint32_t *SR2 = (uint32_t*)(I2C1_BASE_ADDR + 0x18);
    volatile uint32_t *DR  = (uint32_t*)(I2C1_BASE_ADDR + 0x10);

    *CR1 |= (1<<8);
    while(!(*SR1 & 1));

    *DR = dev << 1;
    for (int i = 0; i < 1000; i++) {
        if (((*SR1 >> 1) &1) ==1) {
            (void)*SR2;
            break;
        }
        if (i == 999)
            goto i2c_error;
    }

    while(!(*SR1 & (1<<7)));
    *DR = reg;

    while(!(*SR1 & (1<<2)));

    *CR1 |= (1<<8);
    while(!(*SR1 & 1));

    *DR = (dev << 1) | 1;
    for (int i = 0; i < 1000; i++) {
        if (((*SR1 >> 1) &1) ==1) {
            (void)*SR2;
            break;
        }
        if (i == 999)
            goto i2c_error;
    }


    for(uint8_t i=0;i<len;i++) {
        if(i == len-1) {
            *CR1 &= ~(1<<10);  // ACK = 0
            *CR1 |= (1<<9);    // STOP
        }
        while(!(*SR1 & (1<<6))); // RxNE
        buf[i] = *DR;
    }
    *CR1 |= (1<<10); // ACK lại
    return;

    i2c_error:
    *SR1 &= ~(1<<10);  // clear AF
    *CR1 |= (1<<9);    // STOP
    return;
}
