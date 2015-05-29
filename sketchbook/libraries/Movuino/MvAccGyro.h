#ifndef _MV_ACC_GYRO_H
#define _MV_ACC_GYRO_H

#include "MPU6050.h"
#include "frame_struct.h"

class MvAccGyro {

    public:
        int open(void);

        int close(void);

        int set_acc_sens(unsigned int value);

        int set_gyro_sens(unsigned int value);

        int read(void);

        struct sensor_3_axes get_raw_acc(void);

        struct sensor_3_axes get_raw_gyro(void);

        struct sensor_quaternion get_quat(void);

        struct sensor_euler get_euler(void);

        struct sensor_gravity get_gravity(void);

    private:
        MPU6050 accelgyro;
        sensor_3_axes acc;
        sensor_3_axes gyro;
        sensor_quaternion quat;
        sensor_euler euler;
        sensor_gravity gravity;
};

#endif /* _MV_ACC_GYRO_H */
