#include "MvAccGyro.h"

int MvAccGyro::open(void)
{
    // Initialize I2C
    Wire.begin();

    // Initialize the sensor
    this->accelgyro.initialize();

    // Test the connection
    //if(this->accelgyro.testConnection())
    //    return ANS_NACK_UNKNOWN_ERR;

    // TODO setup the dmp

    return 0;
}

int MvAccGyro::close(void)
{
    // TODO
    return 0;
}

int MvAccGyro::set_acc_sens(unsigned int value)
{
    // TODO
    return 0;
}

int MvAccGyro::set_gyro_sens(unsigned int value)
{
    // TODO
    return 0;
}

int MvAccGyro::read(void)
{
    // read raw accel/gyro measurements from device
    this->accelgyro.getMotion6(&this->acc.x, &this->acc.y, &this->acc.z, &this->gyro.x, &this->gyro.y, &this->gyro.z);

    // TODO: read quaternions/euler/gravity

    return 0;
}

struct sensor_3_axes MvAccGyro::get_raw_acc(void)
{
    return this->acc;
}

struct sensor_3_axes MvAccGyro::get_raw_gyro(void)
{
    return this->gyro;
}

struct sensor_quaternion MvAccGyro::get_quat(void)
{
    return this->quat;
}

struct sensor_euler MvAccGyro::get_euler(void)
{
    return this->euler;
}

struct sensor_gravity MvAccGyro::get_gravity(void)
{
    return this->gravity;
}
