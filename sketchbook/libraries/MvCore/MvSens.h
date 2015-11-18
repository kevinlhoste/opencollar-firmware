#ifndef _MV_SENS_H
#define _MV_SENS_H

#include "MPU6050.h"
#include "MS5611.h"
#include "frame_struct.h"
#include "definitions.h"

struct dmp;

class MvSens {

    public:
        static int open(int addr);

        static int close(void);

        static int set_acc_sens(unsigned int value);

        static unsigned int get_acc_sens(void);

        static int set_gyro_sens(unsigned int value);

        static unsigned int get_gyro_sens(void);

        static int read(void);

        static struct sensor_3_axes get_raw_acc(void);

        static struct sensor_3_axes get_raw_gyro(void);

        static struct sensor_3_axes get_raw_mag(void);

        static struct sensor_single get_raw_alt(void);

        static struct sensor_quaternion get_quat(void);

        static struct sensor_euler get_euler(void);

        static struct sensor_gravity get_gravity(void);

    private:
        // This class shouldn't be instatiated
        MvSens(void);

        static MPU6050 *mpu;
        static MS5611 ms5611;
        static sensor_3_axes acc;
        static sensor_3_axes gyro;
        static sensor_3_axes mag;
        static sensor_single alt;

        static void altimeter_setup(void);

#ifdef MV_SENS_DMP_EN
        static void dmpDataReady(void);
        static void accelgyro_dmp_setup(void);
        static void accelgyro_dmp_data_get(void);

        static struct dmp dmp;
        static sensor_quaternion quat;
#endif //#ifdev MV_SENS_DMP_EN
};

#endif /* _MV_SENS_H */
