#include "MPU6050_6Axis_MotionApps20.h"
#include "MvSens.h"

MPU6050 *MvSens::mpu = NULL;
MS5611 MvSens::ms5611;
sensor_3_axes MvSens::acc;
sensor_3_axes MvSens::gyro;
sensor_3_axes MvSens::mag;
sensor_single MvSens::alt;

void MvSens::altimeter_setup(void)
{
    // TODO: check what to do in case of error
    ms5611.begin(MS5611_ULTRA_HIGH_RES);
}

#ifdef MV_SENS_DMP_EN
struct dmp {
    Quaternion q; // [w, x, y, z]         quaternion container
    uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error) DMP mode
    volatile bool mpuInterrupt;     // indicates whether MPU interrupt pin has gone high
    uint16_t fifoCount;     // count of all bytes currently in FIFO
    uint8_t fifoBuffer[64]; // FIFO storage buffer
    uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
    uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
};

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
    MvSens::dmp.devStatus = MvSens::mpu->dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    //mpu.setXGyroOffset(220);
    //mpu.setYGyroOffset(76);
    //mpu.setZGyroOffset(-85);
    //mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

    // make sure it worked
    if (MvSens::dmp.devStatus == 0) {
        // turn on the DMP, now that it's ready
        //Serial.println(F("Enabling DMP..."));
        MvSens::mpu->setDMPEnabled(true);

        // enable Arduino interrupt detection
        //Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
        attachInterrupt(0, MvSens::dmpDataReady, RISING);
        MvSens::dmp.mpuIntStatus = MvSens::mpu->getIntStatus();

        // get expected DMP packet size for later comparison
        MvSens::dmp.packetSize = MvSens::mpu->dmpGetFIFOPacketSize();
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
    MvSens::dmp.mpuIntStatus = MvSens::mpu->getIntStatus();

    // get current FIFO count
    MvSens::dmp.fifoCount = MvSens::mpu->getFIFOCount();

    // check for overflow (this should never happen unless our code is too inefficient)
    // NOTE: this is happening all the time, even in the MPU6050 demo code
    if ((MvSens::dmp.mpuIntStatus & 0x10) || MvSens::dmp.fifoCount == 1024) {
        // reset so we can continue cleanly
        MvSens::mpu->resetFIFO();
        //Serial.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    } else if (MvSens::dmp.mpuIntStatus & 0x02) {
        // wait for correct available data length, should be a VERY short wait
        while (MvSens::dmp.fifoCount < MvSens::dmp.packetSize) MvSens::dmp.fifoCount = MvSens::mpu->getFIFOCount();

        // read a packet from FIFO
        MvSens::mpu->getFIFOBytes(MvSens::dmp.fifoBuffer, MvSens::dmp.packetSize);
        
        // track FIFO count here in case there is > 1 packet available
        // (this lets us immediately read more without waiting for an interrupt)
        MvSens::dmp.fifoCount -= MvSens::dmp.packetSize;

        // display quaternion values in easy matrix form: w x y z
        MvSens::mpu->dmpGetQuaternion(&MvSens::dmp.q, MvSens::dmp.fifoBuffer);
    }
}
#endif // #ifdef MV_SENS_DMP_EN

int MvSens::open(int addr)
{
    MvSens::mpu = new MPU6050(addr);
    // Initialize I2C
    Wire.begin();

    // Initialize the sensor
    MvSens::mpu->initialize();

    // Setup the magnetometer
    // TODO: check if we really need these lines
    MvSens::mpu->setI2CBypassEnabled(true);
    delay(100);

    // TODO: test the connection
    // Test the connection
    //if(MvSens::mpu->testConnection())
    //    return ANS_NACK_UNKNOWN_ERR;

#ifdef MV_SENS_DMP_EN
    MvSens::accelgyro_dmp_setup();
#endif

    MvSens::altimeter_setup();

    return 0;
}

int MvSens::close(void)
{
    delete MvSens::mpu;
    MvSens::mpu = NULL;
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
            MvSens::mpu->setFullScaleAccelRange(value);
            if(MvSens::mpu->getFullScaleAccelRange() != value)
                return ANS_NACK_INTERNAL_ERR;
            return 0;

        default:
            return ANS_NACK_UNKNOWN_CFG;

    }
}

unsigned int MvSens::get_acc_sens(void)
{
    return MvSens::mpu->getFullScaleAccelRange();
}

int MvSens::set_gyro_sens(unsigned int value)
{
    switch(value)
    {
        case CFG_GYRO_SENS_250DS:
        case CFG_GYRO_SENS_500DS:
        case CFG_GYRO_SENS_1000DS:
        case CFG_GYRO_SENS_2000DS:
            MvSens::mpu->setFullScaleGyroRange(value);
            if(MvSens::mpu->getFullScaleGyroRange() != value)
                return ANS_NACK_INTERNAL_ERR;
            return 0;
        default:
            return ANS_NACK_UNKNOWN_CFG;
    }
}

unsigned int MvSens::get_gyro_sens(void)
{
    return MvSens::mpu->getFullScaleGyroRange();
}

int MvSens::read(void)
{
    // TODO: just read the right data if required

    // read raw measurements from device
    MvSens::mpu->getMotion9(&MvSens::acc.x, &MvSens::acc.y, &MvSens::acc.z,
                            &MvSens::gyro.x, &MvSens::gyro.y, &MvSens::gyro.z,
                            &MvSens::mag.x, &MvSens::mag.y, &MvSens::mag.z);
    MvSens::acc.ts = millis();
    MvSens::gyro.ts = MvSens::acc.ts;
    MvSens::mag.ts = MvSens::acc.ts;
    // FIXME: this was done to avoid I2C blocage

    // Read from the altimeter
    MvSens::alt.p = MvSens::ms5611.readPressure();
    MvSens::alt.ts = millis();
    // FIXME: this was done to avoid I2C blocage

#ifdef MV_SENS_DMP_EN
    // read quaternions
    MvSens::accelgyro_dmp_data_get();

    // Do the math to fill the internal variables

    // Quaternion
    MvSens::quat.w = MvSens::dmp.q.w;
    MvSens::quat.x = MvSens::dmp.q.x;
    MvSens::quat.y = MvSens::dmp.q.y;
    MvSens::quat.z = MvSens::dmp.q.z;
    // FIXME: this was done to avoid I2C blocage

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

struct sensor_single MvSens::get_raw_alt(void)
{
    return MvSens::alt;
}

#ifdef MV_SENS_DMP_EN
struct sensor_quaternion MvSens::get_quat(void)
{
    return MvSens::quat;
}

#endif //#ifdev MV_SENS_DMP_EN
