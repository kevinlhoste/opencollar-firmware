#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"

#include "MvFrameHandler.h"
#include "MvCom.h"
#include "MvAccGyro.h"


class DummyMvCom : public MvCom
{
    public:
        DummyMvCom()
        {
            // set the time this dummy com will block on readBytesUntil function call
            Serial.setTimeout(1);
        }

        int write_frame(char *frame, int size)
        {
            //Serial.print("MvCom write_frame size:");
            //Serial.print(size);
            //Serial.print("\n");

            Serial.write(frame, size);
            Serial.print("\n");
        }

        int read_frame(char *frame, int *size)
        {
            *size = Serial.readBytesUntil('\n', frame, 100);

            if (*size)
            {
                //Serial.print("MvCom read_frame ");
                //Serial.print(*size);
                //Serial.print(":");
                Serial.write(frame, *size);
                Serial.print("\n");
            }

            return 0;
        }

        int set_mode(enum mvCom_mode mode)
        {
            Serial.print("MvCom set_mode\n");
            // We don't support changing the mode
            return ANS_NACK_UNKNOWN_CMD;
        }

        enum mvCom_mode get_mode(void)
        {
            //Serial.print("MvCom get_mode\n");
            return MVCOM_ASCII;
        }
};

// -------------------------------------------------

struct live_ctl
{
    int en;
    unsigned long period;
    unsigned long time_stamp;
} g_live_ctl;

MvCom *g_com;
MvFrameHandler *g_fhandler;
MvAccGyro g_accgyro;
struct frame g_frame;


void setup()
{
    g_live_ctl.en = 0;
    g_live_ctl.time_stamp = 0;
    g_live_ctl.period = 500000; // in micro seconds

    Serial.begin(38400);

    g_com = new DummyMvCom;

    g_fhandler = new MvFrameHandler(&g_com, 1);
}

void send_ack_nack(struct frame *frame, MvFrameHandler *fhandler, int ans_err)
{
    int write_err;

    if(ans_err)
    {
        frame->answer.id = ANS_ID_NACK;
        frame->answer.sub.nack_value = ans_err;
    }
    else
        frame->answer.id = ANS_ID_ACK;

    write_err = fhandler->write_frame(frame);

    if (write_err < 0)
    {
        // This should never happen
        Serial.print("PANIC!!! Write error");
        while(1);
    }
}

bool check_live_time(unsigned long time_stamp, unsigned long period)
{
    // TODO: in the complete app, we need to check the overflow
    if(micros() >= time_stamp + period)
        return true;

    return false;
}

void set_live_sampling_rate(unsigned int hz)
{
    g_live_ctl.period = 1000000/hz;
}

void loop()
{
    int ans_err = ANS_NACK_UNKNOWN_CMD;
    int read_err = g_fhandler->read_frame(&g_frame);

    if (SUCCESS_FRAME_READ == read_err)
    {
        switch(g_frame.cmd.id)
        {
            case CMD_PING:
                Serial.print("Mv Live en:");
                Serial.println(g_live_ctl.en);
                Serial.print("Mv Live period:");
                Serial.println(g_live_ctl.period);
                Serial.print("Mv Live ts:");
                Serial.println(g_live_ctl.time_stamp);
                Serial.print("Mv Time now:");
                Serial.println(micros());

                send_ack_nack(&g_frame, g_fhandler, 0);
                break;

            case CMD_LIVE_START:
                ans_err = g_accgyro.open();
                if (!ans_err) g_live_ctl.en = true;
                send_ack_nack(&g_frame, g_fhandler, ans_err);
                break;

            case CMD_LIVE_STOP:
                ans_err = g_accgyro.close();
                if (!ans_err) g_live_ctl.en = false;
                send_ack_nack(&g_frame, g_fhandler, ans_err);
                break;

            case CMD_CONFIG_SET:
                switch(g_frame.cmd.sub.cfg.id)
                {
                    // TODO: in the real app, write the config into flash and change
                    case CFG_ID_ACC_SENS:
                        ans_err = g_accgyro.set_acc_sens(g_frame.cmd.sub.cfg.value);
                        send_ack_nack(&g_frame, g_fhandler, ans_err);
                        break;
                    case CFG_ID_GYRO_SENS:
                        ans_err = g_accgyro.set_gyro_sens(g_frame.cmd.sub.cfg.value);
                        send_ack_nack(&g_frame, g_fhandler, ans_err);
                        break;

                    case CFG_ID_SAMPING_RATE:
                        set_live_sampling_rate(g_frame.cmd.sub.cfg.value);
                        send_ack_nack(&g_frame, g_fhandler, 0);
                        break;

                    case CFG_ID_LIVE_ACC_RAW_EN:
                    case CFG_ID_LIVE_GYRO_RAW_EN:
                    case CFG_ID_LIVE_QUATERNION_EN:
                    case CFG_ID_LIVE_EULER_EN:
                    case CFG_ID_LIVE_GRAVITY_EN:
                    case CFG_ID_LIVE_ALL_EN:
                    default:
                        send_ack_nack(&g_frame, g_fhandler, ANS_NACK_UNKNOWN_CFG);
                        break;
                }
                break;

            case CMD_SWITCH_MODE:
                // This is a command to the frame handler
                ans_err = g_fhandler->exec_com_cmd(&g_frame);
                send_ack_nack(&g_frame, g_fhandler, ans_err);
                // TODO: write the new mode into the flash
                break;

            // TODO: in the real app support flash instructions
            case CMD_REC_START:
            case CMD_REC_STOP:
            case CMD_REC_PLAY:
            case CMD_REC_CLEAR:
            case CMD_HELP:
            case CMD_VERSION_GET:
            case CMD_CONFIG_GET:
            default:
                send_ack_nack(&g_frame, g_fhandler, ANS_NACK_UNKNOWN_CMD);
                break;
        }
    }
    else if(ANS_NACK_BAD_FRAME_FORMAT == read_err)
    {
        send_ack_nack(&g_frame, g_fhandler, read_err);
    }
    else if(ANS_NACK_INTERNAL_ERR == read_err)
    {
        // This should never happen
        Serial.print("PANIC!!! READ ERROR!");
        while(1);
    }

    // Deal with live
    if (g_live_ctl.en)
    {
        // Check if its time to print the next data
        if(check_live_time(g_live_ctl.time_stamp, g_live_ctl.period))
        {
            unsigned long old_ts = g_live_ctl.time_stamp;
            // Set new time_stamp
            g_live_ctl.time_stamp = micros();

            Serial.print("Mv interval:");
            Serial.println(g_live_ctl.time_stamp - old_ts);

            // Prepare values
            g_accgyro.read();

            // Prepare live frames
            g_frame.answer.id = ANS_ID_LIVE;

            // TODO: in the real app, we should verify if each
            // one of them are enabled

            // Send raw acc data
            g_frame.answer.sub.sensor_data.type = SENS_ACC_RAW;
            g_frame.answer.sub.sensor_data.data.raw = g_accgyro.get_raw_acc();
            g_fhandler->write_frame(&g_frame);

            // Send raw gyro data
            g_frame.answer.sub.sensor_data.type = SENS_GYRO_RAW;
            g_frame.answer.sub.sensor_data.data.raw = g_accgyro.get_raw_gyro();
            g_fhandler->write_frame(&g_frame);

            // Send quat data
            g_frame.answer.sub.sensor_data.type = SENS_QUAT;
            g_frame.answer.sub.sensor_data.data.quat = g_accgyro.get_quat();
            g_fhandler->write_frame(&g_frame);

            // Send euler data
            g_frame.answer.sub.sensor_data.type = SENS_EULER;
            g_frame.answer.sub.sensor_data.data.euler = g_accgyro.get_euler();
            g_fhandler->write_frame(&g_frame);

            // Send gravity data
            g_frame.answer.sub.sensor_data.type = SENS_GRAVITY;
            g_frame.answer.sub.sensor_data.data.gravity = g_accgyro.get_gravity();
            g_fhandler->write_frame(&g_frame);

            //Serial.print("Mv etime:");
            //Serial.println(micros() - g_live_ctl.time_stamp);
        }
    }
}
