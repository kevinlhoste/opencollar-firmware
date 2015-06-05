#ifndef _MV_ACC_GYRO_H
#define _MV_ACC_GYRO_H

// This define is necessary because the MPU6050 have code in the .h files
// so we need it to force the declaration of the motionapps 20 functions
#define MPU6050_INCLUDE_DMP_MOTIONAPPS20
#include "MPU6050.h"
#include "frame_struct.h"
#include "definitions.h"

struct dmp {
    Quaternion q; // [w, x, y, z]         quaternion container
    uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error) DMP mode
    volatile bool mpuInterrupt;     // indicates whether MPU interrupt pin has gone high
    uint16_t fifoCount;     // count of all bytes currently in FIFO
    uint8_t fifoBuffer[64]; // FIFO storage buffer
    uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
    uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
};

class MvAccGyro {

    public:
        static int open(void);

        static int close(void);

        static int set_acc_sens(unsigned int value);

        static int set_gyro_sens(unsigned int value);

        static int read(void);

        static struct sensor_3_axes get_raw_acc(void);

        static struct sensor_3_axes get_raw_gyro(void);

        static struct sensor_quaternion get_quat(void);

        static struct sensor_euler get_euler(void);

        static struct sensor_gravity get_gravity(void);

    private:
        // This class shouldn't be instatiated
        MvAccGyro(void);
        static void dmpDataReady(void);
        static void accelgyro_dmp_setup(void);
        static void accelgyro_dmp_data_get(void);

        static MPU6050 accelgyro;
        static sensor_3_axes acc;
        static sensor_3_axes gyro;
        static struct dmp dmp;
        static sensor_quaternion quat;
        static sensor_euler euler;
        static sensor_gravity gravity;
};

#endif /* _MV_ACC_GYRO_H */
