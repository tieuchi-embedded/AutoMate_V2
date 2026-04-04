/*
 * bno055.c
 *
 *  Created on: Dec 19, 2025
 *      Author: Admin
 */

#include "mydriver.h"

#define BNO055_I2C_ADDR        0x29   // 7-bit

#define BNO055_CHIP_ID         0x00
#define BNO055_CHIP_ID_VAL     0xA0

#define BNO055_PAGE_ID         0x07
#define BNO055_EUL_X_LSB       0x1A
#define BNO055_EUL_X_MSB       0x1B
#define BNO055_EUL_Y_LSB       0x1C
#define BNO055_EUL_Y_MSB       0x1D
#define BNO055_EUL_Z_LSB       0x1E
#define BNO055_EUL_Z_MSB       0x1F

#define BNO055_CALIB_STAT      0x35
#define BNO055_UNIT_SEL        0x3B
#define BNO055_OPR_MODE        0x3D
#define BNO055_PWR_MODE        0x3E

/* OPR MODE */
#define OPR_MODE_CONFIG        0x00
#define OPR_MODE_NDOF          0x0C

/* POWER MODE */
/* POWER MODE – BNO055 */
#define BNO055_PWR_NORMAL     0x00
#define BNO055_PWR_LOWPOWER   0x01
#define BNO055_PWR_SUSPEND    0x02


void BNO055_Init(void)
{
    I2C1_WriteReg(BNO055_I2C_ADDR, BNO055_OPR_MODE, OPR_MODE_CONFIG);

    I2C1_WriteReg(BNO055_I2C_ADDR, BNO055_PAGE_ID, 0x00);
    I2C1_WriteReg(BNO055_I2C_ADDR, BNO055_PWR_MODE, BNO055_PWR_NORMAL);

    I2C1_WriteReg(BNO055_I2C_ADDR, BNO055_OPR_MODE, OPR_MODE_NDOF);
}

uint8_t BNO055_ReadReg(uint8_t reg)
{
    uint8_t val = 0;
    I2C1_ReadMulti(BNO055_I2C_ADDR, reg, &val, 1);
    return val;
}

void BNO055_ReadEuler_raw(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t b[6];
    I2C1_ReadMulti(BNO055_I2C_ADDR, BNO055_EUL_X_LSB, b, 6);

    *x = (int16_t)((b[1]<<8)|b[0]);
    *y = (int16_t)((b[3]<<8)|b[2]);
    *z = (int16_t)((b[5]<<8)|b[4]);
}

uint8_t BNO055_ReadCalibStatus(void)
{
    return BNO055_ReadReg(BNO055_CALIB_STAT);
}

void IMU_HealthCheck(int16_t ex, int16_t ey, int16_t ez)
{
	uint8_t imu_zero_cnt= 0;
	uint8_t imu_fault = 0;
    if (ex == 0 && ey == 0 && ez == 0) {
        if (imu_zero_cnt < 255)
            imu_zero_cnt++;
        if (imu_zero_cnt >= 5)
            imu_fault = 1;
    }
    else {
        imu_zero_cnt = 0;
        imu_fault = 0;
    }
    if (!imu_fault) {
        Feed_WD();
    }
}

