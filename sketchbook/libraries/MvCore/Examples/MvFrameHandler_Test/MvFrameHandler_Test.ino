// This includes are needed because otherwise the Arduino cannot find the headers
// for internal use of the libs
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#include "SPI.h"
#include "DataFlash.h"

#include "MvFrameHandler.h"
#include "MvCom.h"

class DummyMvCom : public MvCom
{
    public:
        int write_frame(char *frame, int size)
        {
            Serial.print("MvCom write_frame size:");
            Serial.print(size);
            Serial.print("\n");

            Serial.write(frame, size);
            Serial.print("\n");
            return size;
        }

        int read_frame(char *frame, int *size)
        {
            *size = Serial.readBytesUntil('\n', frame, 100);

            Serial.print("MvCom read_frame ");
            Serial.print(*size);
            Serial.print(":");

            Serial.write(frame, *size);

            Serial.print("\n");

            return 0;
        }

        int set_mode(enum mvCom_mode mode)
        {
            Serial.print("MvCom set_mode\n");
            return ANS_NACK_UNKNOWN_CMD;
        }

        enum mvCom_mode get_mode(void)
        {
            Serial.print("MvCom get_mode\n");
            return MVCOM_ASCII;
        }
};

// -------------------------------------------------

MvCom *g_com;
MvFrameHandler *g_fhandler;
struct frame frame_ack;
struct frame frame_nack;
struct frame frame_version;
struct frame frame_cfg_get;
struct frame frame_sensor_acc_raw;
struct frame frame_sensor_gyro_raw;
struct frame frame_sensor_euler;
struct frame frame_sensor_quat;
struct frame frame_sensor_grav;


void setup()
{
    Serial.begin(38400);

    g_com = new DummyMvCom;

    g_fhandler = new MvFrameHandler(&g_com, 1);

    // Prepare answers for testing
    frame_ack.answer.id = ANS_ID_ACK;
    frame_ack.com = g_com;

    frame_nack.answer.id = ANS_ID_NACK;
    frame_nack.answer.sub.nack_value = -4;
    frame_nack.com = g_com;

    frame_version.answer.id = ANS_ID_VERSION;
    frame_version.answer.sub.version[0] = 6;
    frame_version.answer.sub.version[1] = 45;
    frame_version.answer.sub.version[2] = 2;
    frame_version.com = g_com;

    frame_cfg_get.answer.id = ANS_ID_CONFIG_GET;
    frame_cfg_get.answer.sub.cfg.id = CFG_ID_ACC_SENS;
    frame_cfg_get.answer.sub.cfg.value = CFG_ACC_SENS_8G;
    frame_cfg_get.com = g_com;

    frame_sensor_acc_raw.answer.id = ANS_ID_LIVE;
    frame_sensor_acc_raw.answer.sub.sensor_data.type = SENS_ACC_RAW;
    frame_sensor_acc_raw.answer.sub.sensor_data.data.raw.x = 9;
    frame_sensor_acc_raw.answer.sub.sensor_data.data.raw.y = 8;
    frame_sensor_acc_raw.answer.sub.sensor_data.data.raw.z = 7;
    frame_sensor_acc_raw.com = g_com;

    frame_sensor_gyro_raw.answer.id = ANS_ID_REC_PLAY;
    frame_sensor_gyro_raw.answer.sub.sensor_data.type = SENS_GYRO_RAW;
    frame_sensor_gyro_raw.answer.sub.sensor_data.data.raw.x = 5;
    frame_sensor_gyro_raw.answer.sub.sensor_data.data.raw.y = 4;
    frame_sensor_gyro_raw.answer.sub.sensor_data.data.raw.z = 3;
    frame_sensor_gyro_raw.com = g_com;

    frame_sensor_quat.answer.id = ANS_ID_LIVE;
    frame_sensor_quat.answer.sub.sensor_data.type = SENS_QUAT;
    frame_sensor_quat.answer.sub.sensor_data.data.quat.w = 5.43;
    frame_sensor_quat.answer.sub.sensor_data.data.quat.x = 4.32;
    frame_sensor_quat.answer.sub.sensor_data.data.quat.y = -1105.98;
    frame_sensor_quat.answer.sub.sensor_data.data.quat.z = 98.0;
    frame_sensor_quat.com = g_com;

    frame_sensor_euler.answer.id = ANS_ID_REC_PLAY;
    frame_sensor_euler.answer.sub.sensor_data.type = SENS_EULER;
    frame_sensor_euler.answer.sub.sensor_data.data.euler.psi = 4.32;
    frame_sensor_euler.answer.sub.sensor_data.data.euler.theta = -1105.98;
    frame_sensor_euler.answer.sub.sensor_data.data.euler.phi = 98.0;
    frame_sensor_euler.com = g_com;

    frame_sensor_grav.answer.id = ANS_ID_REC_PLAY;
    frame_sensor_grav.answer.sub.sensor_data.type = SENS_GRAVITY;
    frame_sensor_grav.answer.sub.sensor_data.data.gravity.yaw = 4.32;
    frame_sensor_grav.answer.sub.sensor_data.data.gravity.pitch = -1105.98;
    frame_sensor_grav.answer.sub.sensor_data.data.gravity.roll = 98.0;
    frame_sensor_grav.com = g_com;
}

void loop()
{
    if (g_fhandler->read_frame(&frame_ack) == SUCCESS_FRAME_READ)
    {
        Serial.print("Mv frame cmd_id:");
        Serial.print((char)frame_ack.cmd.id);
        Serial.print(" cfg_id:");
        Serial.print((char)frame_ack.cmd.sub.cfg.id);
        Serial.print(" cfg_value:");
        Serial.print(frame_ack.cmd.sub.cfg.value);
        Serial.print("\n");

        // Send one from each frame to check the formats

        g_fhandler->write_frame(&frame_ack         );
        g_fhandler->write_frame(&frame_nack        );
        g_fhandler->write_frame(&frame_version     );
        g_fhandler->write_frame(&frame_cfg_get     );
        g_fhandler->write_frame(&frame_sensor_acc_raw);
        g_fhandler->write_frame(&frame_sensor_gyro_raw);
        g_fhandler->write_frame(&frame_sensor_euler);
        g_fhandler->write_frame(&frame_sensor_quat );
        g_fhandler->write_frame(&frame_sensor_grav );
    }
}
