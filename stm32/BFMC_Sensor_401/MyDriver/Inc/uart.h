/*
 * uart.h
 *
 *  Created on: Dec 19, 2025
 *      Author: Admin
 */

#ifndef INC_UART_H_
#define INC_UART_H_



void UART1_Transmit(uint8_t data);
void UART1_init(void);
void UART1_send_number(int num);
void UART1_print_log(char *m);
void UART1_send_float(float v);

void UART2_Transmit(uint8_t data);
void UART2_init(void);
void UART2_send_number(int num);
void UART2_print_log(char *m);
void UART2_send_float(float v);
void UART2_DMA_RX_Init(void);
void USART2_IRQHandler(void);
void UART_Parse_Byte(uint8_t c);
uint8_t UART_Parse_Speed( uint8_t *buf, uint16_t len, int *speed_out);
void UART_Parse_Command(char *cmd);


#endif /* INC_UART_H_ */
