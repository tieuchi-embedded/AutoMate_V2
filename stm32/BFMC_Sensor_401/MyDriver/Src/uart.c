/*
 * uart.c
 *
 *  Created on: Dec 19, 2025
 *      Author: Admin
 */
#include "mydriver.h"
#define RCC_BASE_ADDR      0x40023800UL
#define GPIOA_BASE_ADDR    0x40020000UL
#define USART1_BASE_ADDR   0x40011000UL
#define USART2_BASE_ADDR   0x40004400UL
#define DMA1_BASE_ADDR        0x40026000UL


#define NVIC_ISER0   ((volatile uint32_t*)0xE000E100)
#define NVIC_ISER1   ((volatile uint32_t*)0xE000E104)
#define USART2_IRQN  38

#define UART2_RX_BUF_SIZE 256
volatile uint8_t uart2_rx_dma_buf[UART2_RX_BUF_SIZE];

void UART1_init(void)
{
    uint32_t* RCC_AHB1ENR = (uint32_t*)(RCC_BASE_ADDR + 0x30);
    uint32_t* RCC_APB2ENR = (uint32_t*)(RCC_BASE_ADDR + 0x44);

    *RCC_AHB1ENR |= (1 << 0);    // GPIOA clock
    *RCC_APB2ENR |= (1 << 4);    // USART1 clock

    uint32_t* GPIOA_MODER = (uint32_t*)(GPIOA_BASE_ADDR + 0x00);
    uint32_t* GPIOA_AFRH  = (uint32_t*)(GPIOA_BASE_ADDR + 0x24);

    *GPIOA_MODER &= ~(0b11 << (9 * 2));				//PA9 - TX - AF7
    *GPIOA_MODER |=  (0b10 << (9 * 2));
    *GPIOA_AFRH  &= ~(0xF << ((9 - 8) * 4));
    *GPIOA_AFRH  |=  (0x7 << ((9 - 8) * 4));

    *GPIOA_MODER &= ~(0b11 << (10 * 2));			//PA10 - RX - AF7
    *GPIOA_MODER |=  (0b10 << (10 * 2));
    *GPIOA_AFRH  &= ~(0xF << ((10 - 8) * 4));
    *GPIOA_AFRH  |=  (0x7 << ((10 - 8) * 4));

    uint32_t* USART1_BRR = (uint32_t*)(USART1_BASE_ADDR + 0x08);
    uint32_t* USART1_CR1 = (uint32_t*)(USART1_BASE_ADDR + 0x0C);

    /* Baudrate = 115200, PCLK2 = 16 MHz
       USARTDIV = 16MHz / (16 * 115200) = 8.680 */
    *USART1_BRR = (8 << 4) | 11;

    *USART1_CR1 &= ~(1 << 13);
    *USART1_CR1 = 0;
    *USART1_CR1 &= ~(1 << 15);   // Oversampling 16
    *USART1_CR1 &= ~(1 << 12);   // 8-bit data
    *USART1_CR1 &= ~(1 << 10);   // No parity
    *USART1_CR1 |=  (1 << 2);    // RE
    *USART1_CR1 |=  (1 << 3);    // TE
    *USART1_CR1 |=  (1 << 13);   // UE
}
void UART2_init(void)
{
    uint32_t* RCC_AHB1ENR = (uint32_t*)(RCC_BASE_ADDR + 0x30);
    uint32_t* RCC_APB1ENR = (uint32_t*)(RCC_BASE_ADDR + 0x40);

    *RCC_AHB1ENR |= (1 << 0);     // GPIOA
    *RCC_APB1ENR |= (1 << 17);    // USART2

    /* PA2 TX, PA3 RX */
    uint32_t* GPIOA_MODER = (uint32_t*)(GPIOA_BASE_ADDR + 0x00);
    uint32_t* GPIOA_AFRL  = (uint32_t*)(GPIOA_BASE_ADDR + 0x20);

    *GPIOA_MODER &= ~((3 << (2*2)) | (3 << (3*2)));
    *GPIOA_MODER |=  ((2 << (2*2)) | (2 << (3*2)));

    *GPIOA_AFRL  &= ~((0xF << (2*4)) | (0xF << (3*4)));
    *GPIOA_AFRL  |=  ((7 << (2*4)) | (7 << (3*4)));

    uint32_t* USART2_BRR = (uint32_t*)(USART2_BASE_ADDR + 0x08);
    uint32_t* USART2_CR1 = (uint32_t*)(USART2_BASE_ADDR + 0x0C);

    *USART2_BRR = (8 << 4) | 11;   // 115200 @16MHz

    *USART2_CR1 = 0;
    *USART2_CR1 |= (1 << 2);   // RE
    *USART2_CR1 |= (1 << 3);   // TE
    *USART2_CR1 |= (1 << 5);   // RXNEIE
    *USART2_CR1 |= (1 << 13);  // UE

    NVIC_ISER1[0] = (1 << (USART2_IRQN - 32));
}


#define DMA1_BASE_ADDR 0x40026000UL

void UART2_DMA_RX_Init(void)
{
    uint32_t* DMA1_S5CR   = (uint32_t*)(DMA1_BASE_ADDR + 0x10 + 0x18 * 5);
    uint32_t* DMA1_S5NDTR = (uint32_t*)(DMA1_BASE_ADDR + 0x14 + 0x18 * 5);
    uint32_t* DMA1_S5PAR  = (uint32_t*)(DMA1_BASE_ADDR + 0x18 + 0x18 * 5);
    uint32_t* DMA1_S5M0AR = (uint32_t*)(DMA1_BASE_ADDR + 0x1C + 0x18 * 5);

    *DMA1_S5CR &= ~(1 << 0);   // EN = 0
    while (*DMA1_S5CR & 1);    // Wait disable

    *DMA1_S5PAR  = USART2_BASE_ADDR + 0x04;  // USART2->DR
    *DMA1_S5M0AR = (uint32_t)uart2_rx_dma_buf;
    *DMA1_S5NDTR = UART2_RX_BUF_SIZE;

    *DMA1_S5CR = 0;
    *DMA1_S5CR |= (4 << 25);   // USART2 - CH4
    *DMA1_S5CR |= (0 << 6);    // (PERIPHERAL -> MEMORY)
    *DMA1_S5CR |= (1 << 10);   // MINC = 1
    *DMA1_S5CR |= (0 << 8);    // PSIZE = 8-bit
    *DMA1_S5CR |= (0 << 11);   // MSIZE = 8-bit
    *DMA1_S5CR |= (1 << 8);    // CIRCULAR MODE
    *DMA1_S5CR |= (0 << 16);   // PL = lOW

    *DMA1_S5CR |= (1 << 0);    // EN = 1
}

void USART2_IRQHandler(void)
{
    if (USART2->SR & (1 << 5))   // RXNE
    {
        uint8_t c = USART2->DR; // CLEAR RXNE
        UART_Parse_Byte(c);
    }
}

void UART1_Transmit(uint8_t data)
{
    uint32_t* USART_SR = (uint32_t*)(USART1_BASE_ADDR + 0x00);
    uint32_t* USART_DR = (uint32_t*)(USART1_BASE_ADDR + 0x04);

    while(((*USART_SR >> 7) & 1) == 0);   // TXE
    *USART_DR = data;
    while(((*USART_SR >> 6) & 1) == 0);   // TC
}
void UART2_Transmit(uint8_t data)
{
    uint32_t* USART_SR = (uint32_t*)(USART2_BASE_ADDR + 0x00);
    uint32_t* USART_DR = (uint32_t*)(USART2_BASE_ADDR + 0x04);

    while(((*USART_SR >> 7) & 1) == 0);   // TXE
    *USART_DR = data;
    while(((*USART_SR >> 6) & 1) == 0);   // TC
}

uint8_t UART1_Receive(void)
{
    uint32_t* USART_SR = (uint32_t*)(USART1_BASE_ADDR + 0x00);
    uint32_t* USART_DR = (uint32_t*)(USART1_BASE_ADDR + 0x04);

    while(((*USART_SR >> 5) & 1) == 0);

    return (uint8_t)(*USART_DR & 0xFF);
}
uint8_t UART2_Receive(void)
{
    uint32_t* USART_SR = (uint32_t*)(USART2_BASE_ADDR + 0x00);
    uint32_t* USART_DR = (uint32_t*)(USART2_BASE_ADDR + 0x04);

    while(((*USART_SR >> 5) & 1) == 0);

    return (uint8_t)(*USART_DR & 0xFF);
}
void UART1_send_number(int num)
{
    char buf[12];
    int i = 0;
    if (num == 0) {
        UART1_Transmit('0');
        return;
    }
    if (num < 0) {
        UART1_Transmit('-');
        num = -num;
    }
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    while (i--)
        UART1_Transmit(buf[i]);
}
void UART2_send_number(int num)
{
    char buf[12];
    int i = 0;
    if (num == 0) {
        UART2_Transmit('0');
        return;
    }
    if (num < 0) {
        UART2_Transmit('-');
        num = -num;
    }
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    while (i--)
        UART2_Transmit(buf[i]);
}


void UART1_print_log(char *msg)
{
    for (int i = 0; msg[i]; i++) {
        UART1_Transmit((uint8_t)msg[i]);
    }
}

void UART2_print_log(char *msg)
{
    for (int i = 0; msg[i]; i++) {
        UART2_Transmit((uint8_t)msg[i]);
    }
}

void UART1_send_float(float v)
{
    if (v < 0) {
        UART1_Transmit('-');
        v = -v;
    }

    int ip = (int)v;
    int fp = (int)((v - ip) * 100);

    UART1_send_number(ip);
    UART1_Transmit('.');
    if (fp < 10) UART1_Transmit('0');
    UART1_send_number(fp);
}
void UART2_send_float(float v)
{
    if (v < 0) {
        UART2_Transmit('-');
        v = -v;
    }

    int ip = (int)v;
    int fp = (int)((v - ip) * 100);

    UART2_send_number(ip);
    UART2_Transmit('.');
    if (fp < 10) UART2_Transmit('0');
    UART2_send_number(fp);
}

uint8_t UART_Parse_Speed( uint8_t *buf, uint16_t len, int *speed_out)
{
    if (len < 5) return 0;

    if (len >= UART2_RX_BUF_SIZE)
        len = UART2_RX_BUF_SIZE - 1;

    ((uint8_t*)buf)[len] = '\0';

    if (buf[0] != '#') return 0;

    char *colon = strchr((char*)buf, ':');
    if (!colon) return 0;

    char *end = strstr((char*)buf, ";;");
    if (!end) return 0;

    int speed = atoi(colon + 1);

    *speed_out = speed;
    return 1;
}

void UART_Parse_Command(char *cmd)
{
    //#speed:60;;
    if (strncmp(cmd, "#speed:", 7) == 0) {
        int speed = atoi(cmd + 7);
        Servo_Speed_Set(speed);
        return;
    }

    if (strncmp(cmd, "#steer:", 7) == 0) {
        int steer = atoi(cmd + 7);
        Servo_Steering_Set(steer);
        return;
    }
}

static char cmd_buf[64];
static uint8_t cmd_idx = 0;

void UART_Parse_Byte(uint8_t c)
{
    if (c == '#') {
        cmd_idx = 0;
        cmd_buf[cmd_idx++] = c;
        return;
    }

    if (cmd_idx == 0)
        return;

    if (cmd_idx < sizeof(cmd_buf) - 1) {
        cmd_buf[cmd_idx++] = c;

        if (cmd_idx >= 2 && cmd_buf[cmd_idx - 1] == ';' && cmd_buf[cmd_idx - 2] == ';') {
            cmd_buf[cmd_idx] = '\0';
            UART_Parse_Command(cmd_buf);
            cmd_idx = 0;
        }
    }
    else {
        cmd_idx = 0;
    }
}

