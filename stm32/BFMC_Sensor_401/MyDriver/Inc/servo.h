/*
 * servo.h
 *
 *  Created on: Jan 28, 2026
 *      Author: Admin
 */

#ifndef INC_SERVO_H_
#define INC_SERVO_H_

void PWM_Init(void);
void Servo_Speed_Set(uint16_t us);
void Servo_Steering_Set(uint16_t us);

#endif /* INC_SERVO_H_ */
