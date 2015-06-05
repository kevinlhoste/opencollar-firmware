#include "MvAccGyro.h"

MPU6050 MvAccGyro::accelgyro;
sensor_3_axes MvAccGyro::acc;
sensor_3_axes MvAccGyro::gyro;
sensor_quaternion MvAccGyro::quat;
sensor_euler MvAccGyro::euler;
sensor_gravity MvAccGyro::gravity;
struct dmp MvAccGyro::dmp;

#ifdef MV_ACC_GYRO_DMP_EN
void MvAccGyro::dmpDataReady(void)
{
    MvAccGyro::dmp.mpuInterrupt = true;
}

void MvAccGyro::accelgyro_dmp_setup(void)
{
    MvAccGyro::dmp.mpuInterrupt = false;
    // load and configure the DMP
    //Serial.println(F("Initializing DMP..."));
    MvAccGyro::dmp.devStatus = MvAccGyro::accelgyro.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    //mpu.setXGyroOffset(220);
    //mpu.setYGyroOffset(76);
    //mpu.setZGyroOffset(-85);
    //mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

    // make sure it worked
    if (MvAccGyro::dmp.devStatus == 0) {
        // turn on the DMP, now that it's ready
        //Serial.println(F("Enabling DMP..."));
        MvAccGyro::accelgyro.setDMPEnabled(true);

        // enable Arduino interrupt detection
        //Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
        attachInterrupt(0, MvAccGyro::dmpDataReady, RISING);
        MvAccGyro::dmp.mpuIntStatus = MvAccGyro::accelgyro.getIntStatus();

        // get expected DMP packet size for later comparison
        MvAccGyro::dmp.packetSize = MvAccGyro::accelgyro.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(MvAccGyro::dmp.devStatus);
        Serial.println(F(")"));
        // Block
        while(1);
    }
}

void MvAccGyro::accelgyro_dmp_data_get(void)
{
    // wait for MPU interrupt or extra packet(s) available
    while (!MvAccGyro::dmp.mpuInterrupt);

    // reset interrupt flag and get INT_STATUS byte
    MvAccGyro::dmp.mpuInterrupt = false;
    MvAccGyro::dmp.mpuIntStatus = MvAccGyro::accelgyro.getIntStatus();

    // get current FIFO count
    MvAccGyro::dmp.fifoCount = MvAccGyro::accelgyro.getFIFOCount();

    // check for overflow (this should never happen unless our code is too inefficient)
    // NOTE: this is happening all the time, even in the MPU6050 demo code
    if ((MvAccGyro::dmp.mpuIntStatus & 0x10) || MvAccGyro::dmp.fifoCount == 1024) {
        // reset so we can continue cleanly
        MvAccGyro::accelgyro.resetFIFO();
        //Serial.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    } else if (MvAccGyro::dmp.mpuIntStatus & 0x02) {
        // wait for correct available data length, should be a VERY short wait
        while (MvAccGyro::dmp.fifoCount < MvAccGyro::dmp.packetSize) MvAccGyro::dmp.fifoCount = MvAccGyro::accelgyro.getFIFOCount();

        // read a packet from FIFO
        MvAccGyro::accelgyro.getFIFOBytes(MvAccGyro::dmp.fifoBuffer, MvAccGyro::dmp.packetSize);
        
        // track FIFO count here in case there is > 1 packet available
        // (this lets us immediately read more without waiting for an interrupt)
        MvAccGyro::dmp.fifoCount -= MvAccGyro::dmp.packetSize;

        // display quaternion values in easy matrix form: w x y z
        MvAccGyro::accelgyro.dmpGetQuaternion(&MvAccGyro::dmp.q, MvAccGyro::dmp.fifoBuffer);
    }
}
#else
void MvAccGyro::dmpDataReady(void){}
void MvAccGyro::accelgyro_dmp_setup(void){}
void MvAccGyro::accelgyro_dmp_data_get(void){}
#endif // #ifdef MV_ACC_GYRO_DMP_EN

int MvAccGyro::open(void)
{
    // Initialize I2C
    Wire.begin();

    // Initialize the sensor
    MvAccGyro::accelgyro.initialize();

    // TODO: test the connection
    // Test the connection
    //if(MvAccGyro::accelgyro.testConnection())
    //    return ANS_NACK_UNKNOWN_ERR;

    // TODO The dmp shoul be initialized only if quaternion, euler or gravity are set
    MvAccGyro::accelgyro_dmp_setup();

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
    // TODO: just read the right data if required

    // read raw accel/gyro measurements from device
    MvAccGyro::accelgyro.getMotion6(&MvAccGyro::acc.x, &MvAccGyro::acc.y, &MvAccGyro::acc.z, &MvAccGyro::gyro.x, &MvAccGyro::gyro.y, &MvAccGyro::gyro.z);

    // read quaternions/euler/gravity
    MvAccGyro::accelgyro_dmp_data_get();

    // Do the math to fill the internal variables

    // Quaternion
    MvAccGyro::quat.w = MvAccGyro::dmp.q.w;
    MvAccGyro::quat.x = MvAccGyro::dmp.q.x;
    MvAccGyro::quat.y = MvAccGyro::dmp.q.y;
    MvAccGyro::quat.z = MvAccGyro::dmp.q.z;

    // Euler
    float ftemp[3];
#ifdef MV_ACC_GYRO_DMP_EULER_EN
    MvAccGyro::accelgyro.dmpGetEuler(ftemp, &MvAccGyro::dmp.q);
    MvAccGyro::euler.psi =      ftemp[0] * 180/M_PI;
    MvAccGyro::euler.theta =    ftemp[1] * 180/M_PI;
    MvAccGyro::euler.phi =      ftemp[2] * 180/M_PI;
#endif //#ifdef MV_ACC_GYRO_DMP_EULER_EN

#ifdef MV_ACC_GYRO_DMP_GRAV_EN
    // Gravity
    VectorFloat gravity;
    MvAccGyro::accelgyro.dmpGetGravity(&gravity, &MvAccGyro::dmp.q);
    MvAccGyro::accelgyro.dmpGetYawPitchRoll(ftemp, &MvAccGyro::dmp.q, &gravity);
    MvAccGyro::gravity.yaw      = ftemp[0] * 180/M_PI;
    MvAccGyro::gravity.pitch    = ftemp[1] * 180/M_PI;
    MvAccGyro::gravity.roll     = ftemp[2] * 180/M_PI;
#endif //#ifdef MV_ACC_GYRO_DMP_GRAV_EN

    return 0;
}

struct sensor_3_axes MvAccGyro::get_raw_acc(void)
{
    return MvAccGyro::acc;
}

struct sensor_3_axes MvAccGyro::get_raw_gyro(void)
{
    return MvAccGyro::gyro;
}

struct sensor_quaternion MvAccGyro::get_quat(void)
{
    return MvAccGyro::quat;
}

struct sensor_euler MvAccGyro::get_euler(void)
{
    return MvAccGyro::euler;
}

struct sensor_gravity MvAccGyro::get_gravity(void)
{
    return MvAccGyro::gravity;
}
