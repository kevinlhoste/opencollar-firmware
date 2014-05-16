#ifndef ACCELGYRO_H
#define ACCELGYRO_H

#include "serial_functions.h"

/* acc_range values */
#define ACC_2G 0
#define ACC_4G 1
#define ACC_8G 2
#define ACC_16G 3

/* gyro_range values in degrees/sec */
#define GYRO_250 0
#define GYRO_500 1
#define GYRO_1000 2
#define GYRO_2000 3

struct Accelgyro {
MPU6050 mpu;
int16_t ax, ay, az; //acceleroscope axes
int16_t gx, gy, gz; //gyroscope axes
int16_t sampling_rate; //unity = Hz
char acc_range;
char gyro_range;
} accelgyro;

#define ACCELGYRO_SET_ACCELRANGE(value)                                 \
{                                                                       \
    accelgyro.mpu.setFullScaleAccelRange(value);                        \
    if(accelgyro.mpu.getFullScaleAccelRange() != (value))               \
        { serial_println_str("M failed to configure"); }                    \
    else { accelgyro.acc_range = (value); }                             \
}

#define ACCELGYRO_SET_GYRORANGE(value)                                  \
{                                                                       \
    accelgyro.mpu.setFullScaleGyroRange(value);                         \
    if(accelgyro.mpu.getFullScaleGyroRange() != value)        \
        { serial_println_str("M failed to configure"); }                    \
    else { accelgyro.gyro_range = value; }                              \
}

void 
accelgyro_default_conf(void)
{
    ACCELGYRO_SET_ACCELRANGE(ACC_4G);
    ACCELGYRO_SET_GYRORANGE(GYRO_500);
    accelgyro.sampling_rate = 10;
}

void
accelgyro_setup(void)
{
    //Join I2C
    Wire.begin();
    accelgyro.mpu.initialize();
    accelgyro_default_conf();
}

void
accelgyro_get(void)
{
    accelgyro.mpu.getMotion6(&accelgyro.ax,
                             &accelgyro.ay,
                             &accelgyro.az,
                             &accelgyro.gx,
                             &accelgyro.gy,
                             &accelgyro.gz);
    
}

#endif
