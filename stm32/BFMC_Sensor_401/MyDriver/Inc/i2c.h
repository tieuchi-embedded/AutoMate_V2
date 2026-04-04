/*
 * i2c.h
 *
 *  Created on: Dec 19, 2025
 *      Author: Admin
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_

#include "stdint.h"

void I2C1_Master_Init(void);
void I2C1_WriteReg(uint8_t dev, uint8_t reg, uint8_t data);
void I2C1_ReadMulti(uint8_t dev, uint8_t reg, uint8_t *buf, uint8_t len);

#endif /* INC_I2C_H_ */
