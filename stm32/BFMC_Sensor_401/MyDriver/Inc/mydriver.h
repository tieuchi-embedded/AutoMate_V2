/*
 * mydriver.h
 *
 *  Created on: Dec 19, 2025
 *      Author: Admin
 */
#include "main.h"
#include <string.h>
#include <stdlib.h>

#ifndef INC_MYDRIVER_H_
#define INC_MYDRIVER_H_

#define UART2_RX_BUF_SIZE 256
extern volatile uint8_t uart2_rx_dma_buf[UART2_RX_BUF_SIZE];
//extern volatile uint16_t uart2_rx_len;
//extern volatile uint8_t  uart2_rx_ready;
//extern volatile uint16_t rx_head;


#include "main.h"
#include "i2c.h"
#include "uart.h"
#include "bno055.h"
#include "as5600.h"
#include "timer.h"
#include "servo.h"
#include "rcc.h"
#include "watchdog.h"
#include "systick.h"
#endif /* INC_MYDRIVER_H_ */
