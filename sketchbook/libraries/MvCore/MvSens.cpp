#include "MvSens.h"

MPU6050 MvSens::accelgyro;
sensor_3_axes MvSens::acc;
sensor_3_axes MvSens::gyro;
sensor_3_axes MvSens::mag;

#ifdef MV_SENS_DMP_EN
struct dmp MvSens::dmp;
sensor_quaternion MvSens::quat;

void MvSens::dmpDataReady(void)
{
    MvSens::dmp.mpuInterrupt = true;
}

void MvSens::accelgyro_dmp_setup(void)
{
    MvSens::dmp.mpuInterrupt = false;
    // load and configure the DMP
    //Serial.println(F("Initializing DMP..."));
    MvSens::dmp.devStatus = MvSens::accelgyro.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    //mpu.setXGyroOffset(220);
    //mpu.setYGyroOffset(76);
    //mpu.setZGyroOffset(-85);
    //mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

    // make sure it worked
    if (MvSens::dmp.devStatus == 0) {
        // turn on the DMP, now that it's ready
        //Serial.println(F("Enabling DMP..."));
        MvSens::accelgyro.setDMPEnabled(true);

        // enable Arduino interrupt detection
        //Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
        attachInterrupt(0, MvSens::dmpDataReady, RISING);
        MvSens::dmp.mpuIntStatus = MvSens::accelgyro.getIntStatus();

        // get expected DMP packet size for later comparison
        MvSens::dmp.packetSize = MvSens::accelgyro.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print("DMP Init failed:");
        Serial.println(MvSens::dmp.devStatus);
        // Block
        while(1);
    }
}

void MvSens::accelgyro_dmp_data_get(void)
{
    // wait for MPU interrupt or extra packet(s) available
    while (!MvSens::dmp.mpuInterrupt);

    // reset interrupt flag and get INT_STATUS byte
    MvSens::dmp.mpuInterrupt = false;
    MvSens::dmp.mpuIntStatus = MvSens::accelgyro.getIntStatus();

    // get current FIFO count
    MvSens::dmp.fifoCount = MvSens::accelgyro.getFIFOCount();

    // check for overflow (this should never happen unless our code is too inefficient)
    // NOTE: this is happening all the time, even in the MPU6050 demo code
    if ((MvSens::dmp.mpuIntStatus & 0x10) || MvSens::dmp.fifoCount == 1024) {
        // reset so we can continue cleanly
        MvSens::accelgyro.resetFIFO();
        //Serial.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    } else if (MvSens::dmp.mpuIntStatus & 0x02) {
        // wait for correct available data length, should be a VERY short wait
        while (MvSens::dmp.fifoCount < MvSens::dmp.packetSize) MvSens::dmp.fifoCount = MvSens::accelgyro.getFIFOCount();

        // read a packet from FIFO
        MvSens::accelgyro.getFIFOBytes(MvSens::dmp.fifoBuffer, MvSens::dmp.packetSize);
        
        // track FIFO count here in case there is > 1 packet available
        // (this lets us immediately read more without waiting for an interrupt)
        MvSens::dmp.fifoCount -= MvSens::dmp.packetSize;

        // display quaternion values in easy matrix form: w x y z
        MvSens::accelgyro.dmpGetQuaternion(&MvSens::dmp.q, MvSens::dmp.fifoBuffer);
    }
}
#endif // #ifdef MV_SENS_DMP_EN

int MvSens::open(void)
{
    // Initialize I2C
    Wire.begin();

    // Initialize the sensor
    MvSens::accelgyro.initialize();

    // Setup the magnetometer
    // TODO: check if we really need these lines
    MvSens::accelgyro.setI2CBypassEnabled(true);
    delay(100);

    // TODO: test the connection
    // Test the connection
    //if(MvSens::accelgyro.testConnection())
    //    return ANS_NACK_UNKNOWN_ERR;

#ifdef MV_SENS_DMP_EN
    MvSens::accelgyro_dmp_setup();
#endif

    return 0;
}

int MvSens::close(void)
{
    // TODO
    return 0;
}

int MvSens::set_acc_sens(unsigned int value)
{
    switch(value)
    {
        case CFG_ACC_SENS_2G:
        case CFG_ACC_SENS_4G:
        case CFG_ACC_SENS_8G:
        case CFG_ACC_SENS_16G:
            MvSens::accelgyro.setFullScaleAccelRange(value);
            if(MvSens::accelgyro.getFullScaleAccelRange() != value)
                return ANS_NACK_UNKNOWN_CFG;
            return 0;

        default:
            return ANS_NACK_UNKNOWN_CFG;

    }
}

int MvSens::set_gyro_sens(unsigned int value)
{
    switch(value)
    {
        case CFG_GYRO_SENS_250DS:
        case CFG_GYRO_SENS_500DS:
        case CFG_GYRO_SENS_1000DS:
        case CFG_GYRO_SENS_2000DS:
            MvSens::accelgyro.setFullScaleGyroRange(value);
            if(MvSens::accelgyro.getFullScaleGyroRange() != value)
                return ANS_NACK_INTERNAL_ERR;
            return 0;
        default:
            return ANS_NACK_INTERNAL_ERR;
    }
}

int MvSens::read(void)
{
    // TODO: just read the right data if required

    // read raw accel/gyro measurements from device
    if (!MvSens::accelgyro.checkMag())
        MvSens::accelgyro.getMotion6(&MvSens::acc.x, &MvSens::acc.y, &MvSens::acc.z,
                                        &MvSens::gyro.x, &MvSens::gyro.y, &MvSens::gyro.z);
    else
        MvSens::accelgyro.getMotion9(&MvSens::acc.x, &MvSens::acc.y, &MvSens::acc.z,
                                        &MvSens::gyro.x, &MvSens::gyro.y, &MvSens::gyro.z,
                                        &MvSens::mag.x, &MvSens::mag.y, &MvSens::mag.z);

#ifdef MV_SENS_DMP_EN
    // read quaternions
    MvSens::accelgyro_dmp_data_get();

    // Do the math to fill the internal variables

    // Quaternion
    MvSens::quat.w = MvSens::dmp.q.w;
    MvSens::quat.x = MvSens::dmp.q.x;
    MvSens::quat.y = MvSens::dmp.q.y;
    MvSens::quat.z = MvSens::dmp.q.z;

#endif //#ifdev MV_SENS_DMP_EN

    return 0;
}

struct sensor_3_axes MvSens::get_raw_acc(void)
{
    return MvSens::acc;
}

struct sensor_3_axes MvSens::get_raw_gyro(void)
{
    return MvSens::gyro;
}

struct sensor_3_axes MvSens::get_raw_mag(void)
{
    return MvSens::mag;
}

#ifdef MV_SENS_DMP_EN
struct sensor_quaternion MvSens::get_quat(void)
{
    return MvSens::quat;
}

#endif //#ifdev MV_SENS_DMP_EN
