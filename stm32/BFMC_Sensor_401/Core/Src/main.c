#include "main.h"
#include "mydriver.h"



uint8_t imu_available = 0;
extern volatile uint32_t ms_tick;
extern uint32_t SystemCoreClock;



int main(void)
{
#if 1
  UART2_init();
  I2C1_Master_Init();
  PWM_Init();
  Timer3_Init();
  UART2_print_log("Chip clock:");	UART2_send_number(SystemCoreClock);	  UART2_print_log("\r\n");
  SysTick_Init(SystemCoreClock);
  UART2_print_log("\r\n");
  UART2_print_log("BNO055 scan...\r\n");
  uint8_t id = BNO055_ReadReg(BNO055_CHIP_ID);
  UART2_print_log("CHIP_ID=");
  UART2_send_number(id);
  UART2_print_log("\r\n");
  if (id != BNO055_CHIP_ID_VAL) {
      UART2_print_log("BNO055 not found, degraded mode\r\n");
      imu_available = 0;
  } else {
      imu_available = 1;
  }
  UART2_print_log("BNO055 init...\r\n");
  BNO055_Init();

  uint8_t as_stat = AS5600_ReadStatus();
  UART2_print_log("AS5600 STATUS=");
  UART2_send_number(as_stat);
  UART2_print_log("\r\n");
  Watchdog_Init();

#endif


  while(1)
  {
#if 1
	  if (imu_available) {
	    // đọc IMU
		int16_t ex, ey, ez;     // raw (1/16 deg)
		float   yaw, roll, pitch;

		BNO055_ReadEuler_raw(&ex, &ey, &ez);

		yaw   =  ex / 16.0f;    // Heading/Yaw
		roll  =  ey / 16.0f;    // Roll
		pitch =  ez / 16.0f;    // Pitch

		UART2_print_log("@imu:");
		UART2_send_float(yaw);
		UART2_print_log(",");
		UART2_send_float(pitch);
		UART2_print_log(",");
		UART2_send_float(roll);
        UART2_print_log(";;");
		UART2_print_log("\r\n");

		IMU_HealthCheck(ex,ey,ez);
	  }
#endif
#if 0
        float dt = 0.01f; // 10 ms

        float w = AS5600_ReadAngularSpeedDeg(dt);

        UART2_print_log("@rpm:");
        UART2_send_float(w);
        UART2_print_log(";;");
        UART2_print_log("\r\n");
        Feed_WD();

    	Delay_t(10);
#endif
#if 0
    	float deg = AS5600_ReadAngleDeg();
        UART2_send_float(deg);
        UART2_print_log("\r\n");
        Feed_WD();
//        uint8_t s = AS5600_ReadStatus();
//        UART2_send_number(s);
//        UART2_print_log("\r\n");
    	Delay_t(10);

#endif
  }
}
