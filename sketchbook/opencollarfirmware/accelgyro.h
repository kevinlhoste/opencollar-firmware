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
char enabled_sensors;
char acc_range;
char gyro_range;
Quaternion q; // [w, x, y, z]         quaternion container
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error) DMP mode
volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
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

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

void dmpDataReady() {
    accelgyro.mpuInterrupt = true;
}

void 
accelgyro_default_conf(void)
{
    ACCELGYRO_SET_ACCELRANGE(ACC_4G);
    ACCELGYRO_SET_GYRORANGE(GYRO_500);
    accelgyro.sampling_rate = 10;
    accelgyro.enabled_sensors = 2;
}

void
accelgyro_setup(void)
{
    //Join I2C
    Wire.begin();
    accelgyro.mpu.initialize();
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

void
accelgyro_quaternion_setup(void)
{
    // load and configure the DMP
    //Serial.println(F("Initializing DMP..."));
    accelgyro.devStatus = accelgyro.mpu.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    //mpu.setXGyroOffset(220);
    //mpu.setYGyroOffset(76);
    //mpu.setZGyroOffset(-85);
    //mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

    // make sure it worked
    if (accelgyro.devStatus == 0) {
        // turn on the DMP, now that it's ready
        //Serial.println(F("Enabling DMP..."));
        accelgyro.mpu.setDMPEnabled(true);

        // enable Arduino interrupt detection
        //Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
        attachInterrupt(0, dmpDataReady, RISING);
        accelgyro.mpuIntStatus = accelgyro.mpu.getIntStatus();

        // get expected DMP packet size for later comparison
        accelgyro.packetSize = accelgyro.mpu.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(accelgyro.devStatus);
        Serial.println(F(")"));
        // Block
        while(1);
    }
}

void
accelgyro_quaternion_get(void)
{
    // wait for MPU interrupt or extra packet(s) available
    while (!accelgyro.mpuInterrupt);

    // reset interrupt flag and get INT_STATUS byte
    accelgyro.mpuInterrupt = false;
    accelgyro.mpuIntStatus = accelgyro.mpu.getIntStatus();

    // get current FIFO count
    accelgyro.fifoCount = accelgyro.mpu.getFIFOCount();

    // check for overflow (this should never happen unless our code is too inefficient)
    // NOTE: this is happening all the time, even in the MPU6050 demo code
    if ((accelgyro.mpuIntStatus & 0x10) || accelgyro.fifoCount == 1024) {
        // reset so we can continue cleanly
        accelgyro.mpu.resetFIFO();
        //Serial.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    } else if (accelgyro.mpuIntStatus & 0x02) {
        // wait for correct available data length, should be a VERY short wait
        while (accelgyro.fifoCount < accelgyro.packetSize) accelgyro.fifoCount = accelgyro.mpu.getFIFOCount();

        // read a packet from FIFO
        accelgyro.mpu.getFIFOBytes(accelgyro.fifoBuffer, accelgyro.packetSize);
        
        // track FIFO count here in case there is > 1 packet available
        // (this lets us immediately read more without waiting for an interrupt)
        accelgyro.fifoCount -= accelgyro.packetSize;

        // display quaternion values in easy matrix form: w x y z
        accelgyro.mpu.dmpGetQuaternion(&accelgyro.q, accelgyro.fifoBuffer);

        // TODO: remove this print
        Serial.print("quat\t");
        Serial.print(accelgyro.q.w);
        Serial.print("\t");
        Serial.print(accelgyro.q.x);
        Serial.print("\t");
        Serial.print(accelgyro.q.y);
        Serial.print("\t");
        Serial.println(accelgyro.q.z);
    }
}

#endif
