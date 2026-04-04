/*
 * as5600.h
 *
 *  Created on: Jan 28, 2026
 *      Author: Admin
 */

#ifndef INC_AS5600_H_
#define INC_AS5600_H_

#define BNO055_CHIP_ID         0x00
#define BNO055_CHIP_ID_VAL     0xA0

uint8_t AS5600_ReadStatus(void);
uint16_t AS5600_ReadAngleRaw(void);
float AS5600_ReadAngleDeg(void);
float AS5600_ReadAngularSpeedDeg(float dt_sec);



#endif /* INC_AS5600_H_ */
