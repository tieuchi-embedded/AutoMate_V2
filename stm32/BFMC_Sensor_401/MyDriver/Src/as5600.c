/*
 * as5600.c
 *
 *  Created on: Jan 28, 2026
 *      Author: Admin
 */

#include "mydriver.h"

#define AS5600_I2C_ADDR     0x36   // 7-bit
#define AS5600_STATUS       0x0B
#define AS5600_ANGLE_MSB    0x0E
#define AS5600_ANGLE_LSB    0x0F

uint8_t AS5600_ReadStatus(void)
{
    uint8_t v = 0;
    I2C1_ReadMulti(AS5600_I2C_ADDR, AS5600_STATUS, &v, 1);
    return v;
}

uint16_t AS5600_ReadAngleRaw(void)
{
    uint8_t buf[2];
    I2C1_ReadMulti(AS5600_I2C_ADDR, AS5600_ANGLE_MSB, buf, 2);

    uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
    raw &= 0x0FFF;   // lấy đúng 12 bit

    return raw;
}

float AS5600_ReadAngleDeg(void)
{
    uint16_t raw = AS5600_ReadAngleRaw();
    return (raw * 360.0f) / 4096.0f;
}

float AS5600_ReadAngularSpeedDeg(float dt_sec)
{
    static uint16_t prev_raw = 0;
    static uint8_t  first = 1;

    uint16_t curr_raw = AS5600_ReadAngleRaw();

    if (first) {
        prev_raw = curr_raw;
        first = 0;
        return 0.0f;
    }

    int16_t delta = (int16_t)(curr_raw - prev_raw);

    if (delta >  2048) delta -= 4096;
    if (delta < -2048) delta += 4096;

    prev_raw = curr_raw;
    float delta_deg = (delta * 360.0f) / 4096.0f;

    return delta_deg / dt_sec;
}
