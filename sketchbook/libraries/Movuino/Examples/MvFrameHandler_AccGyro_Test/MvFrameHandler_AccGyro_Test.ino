// This includes are needed because otherwise the Arduino cannot find the headers
// for internal use of the libs
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#include "SPI.h"
#include "DataFlash.h"

#include "MvFrameHandler.h"
#include "MvCom.h"
#include "MvAccGyro.h"
#include "SerialMvCom.h"

struct app_context
{
    struct live_ctl
    {
        unsigned long period;
        unsigned long time_stamp;
        MvCom *com_list[MAX_COM_PORTS];
        /* ref counts how many com ports are in the live mode com list*/
        int ref;
    } live_ctl;

    MvFrameHandler *fhandler;
    struct frame frame;
} g_ctx;

void setup()
{
    int i;
    MvCom *com[2];

    g_ctx.live_ctl.ref = 0;
    g_ctx.live_ctl.time_stamp = 0;
    g_ctx.live_ctl.period = 500000; // in micro seconds

    for (i = 0; i < MAX_COM_PORTS; i++)
        g_ctx.live_ctl.com_list[i] == NULL;

    Serial.begin(38400);
    Serial1.begin(9600);

    com[0] = new SerialMvCom(&Serial);
    com[1] = new SerialMvCom(&Serial1);

    g_ctx.fhandler = new MvFrameHandler(com, 2);
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
    g_ctx.live_ctl.period = 1000000/hz;
}

void add_com_to_live_list(MvCom *com)
{
    int i;

    /* Check if it is already on the list */
    for (i = 0; i < MAX_COM_PORTS; i++)
    {
        if (g_ctx.live_ctl.com_list[i] == com)
            return;
    }

    /* Find a null entry */
    for (i = 0; i < MAX_COM_PORTS; i++)
    {
        if (g_ctx.live_ctl.com_list[i] == NULL)
        {
            g_ctx.live_ctl.com_list[i] = com;
            g_ctx.live_ctl.ref++;
            return;
        }
    }
}

void remove_com_from_live_list(MvCom *com)
{
    int i;

    for (i = 0; i < MAX_COM_PORTS; i++)
    {
        if (g_ctx.live_ctl.com_list[i] == com)
        {
            g_ctx.live_ctl.com_list[i] = NULL;
            g_ctx.live_ctl.ref--;
            return;
        }
    }
}

void send_live(struct frame *frame)
{
    int i;

    for (i = 0; i < MAX_COM_PORTS; i++)
    {
        if (g_ctx.live_ctl.com_list[i] != NULL)
        {
            frame->com = g_ctx.live_ctl.com_list[i];
            g_ctx.fhandler->write_frame(frame);
        }
    }
}

void loop()
{
    int ans_err = ANS_NACK_UNKNOWN_CMD;
    int read_err = g_ctx.fhandler->read_frame(&g_ctx.frame);

    if (SUCCESS_FRAME_READ == read_err)
    {
        switch(g_ctx.frame.cmd.id)
        {
            case CMD_PING:
                //Serial.print("Mv Live period:");
                //Serial.println(g_ctx.live_ctl.period);
                //Serial.print("Mv Live ts:");
                //Serial.println(g_ctx.live_ctl.time_stamp);
                //Serial.print("Mv Time now:");
                //Serial.println(micros());

                send_ack_nack(&g_ctx.frame, g_ctx.fhandler, 0);
                break;

            case CMD_LIVE_START:
                ans_err = 0;

                /* Check if the acc has already been initialized */
                if (!g_ctx.live_ctl.ref)
                    ans_err = MvAccGyro::open();

                if (!ans_err)
                    add_com_to_live_list(g_ctx.frame.com);

                send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ans_err);
                break;

            case CMD_LIVE_STOP:
                ans_err = 0;

                remove_com_from_live_list(g_ctx.frame.com);

                /* If there is no more com ports in live mode */
                if (!g_ctx.live_ctl.ref)
                    ans_err = MvAccGyro::close();

                send_ack_nack(&g_ctx.frame, g_ctx.fhandler, 0);
                break;

            case CMD_CONFIG_SET:
                switch(g_ctx.frame.cmd.sub.cfg.id)
                {
                    // TODO: in the real app, write the config into flash and change
                    case CFG_ID_ACC_SENS:
                        ans_err = MvAccGyro::set_acc_sens(g_ctx.frame.cmd.sub.cfg.value);
                        send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ans_err);
                        break;
                    case CFG_ID_GYRO_SENS:
                        ans_err = MvAccGyro::set_gyro_sens(g_ctx.frame.cmd.sub.cfg.value);
                        send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ans_err);
                        break;

                    case CFG_ID_SAMPING_RATE:
                        set_live_sampling_rate(g_ctx.frame.cmd.sub.cfg.value);
                        send_ack_nack(&g_ctx.frame, g_ctx.fhandler, 0);
                        break;

                    case CFG_ID_LIVE_ACC_RAW_EN:
                    case CFG_ID_LIVE_GYRO_RAW_EN:
                    case CFG_ID_LIVE_QUATERNION_EN:
                    case CFG_ID_LIVE_EULER_EN:
                    case CFG_ID_LIVE_GRAVITY_EN:
                    case CFG_ID_LIVE_ALL_EN:
                    default:
                        send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ANS_NACK_UNKNOWN_CFG);
                        break;
                }
                break;

            case CMD_SWITCH_MODE:
                // This is a command to the frame handler
                ans_err = g_ctx.fhandler->exec_com_cmd(&g_ctx.frame);
                send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ans_err);
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
                send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ANS_NACK_UNKNOWN_CMD);
                break;
        }
    }
    else if(ANS_NACK_BAD_FRAME_FORMAT == read_err)
    {
        send_ack_nack(&g_ctx.frame, g_ctx.fhandler, read_err);
    }
    else if(ANS_NACK_INTERNAL_ERR == read_err)
    {
        // This should never happen
        Serial.print("PANIC!!! READ ERROR!");
        while(1);
    }

    // Deal with live
    if (g_ctx.live_ctl.ref)
    {
        // Prepare values
        // This is called every loop because if the DMP is being used, we must call this function
        // to read from the FIFO
        MvAccGyro::read();

        // Check if its time to print the next data
        if(check_live_time(g_ctx.live_ctl.time_stamp, g_ctx.live_ctl.period))
        {
            unsigned long old_ts = g_ctx.live_ctl.time_stamp;
            // Set new time_stamp
            g_ctx.live_ctl.time_stamp = micros();

            //Serial.print("Mv interval:");
            //Serial.println(g_ctx.live_ctl.time_stamp - old_ts);

            // Prepare live frames
            g_ctx.frame.answer.id = ANS_ID_LIVE;

            // TODO: in the real app, we should verify if each
            // one of them are enabled

            // Send raw acc data
            g_ctx.frame.answer.sub.sensor_data.type = SENS_ACC_RAW;
            g_ctx.frame.answer.sub.sensor_data.data.raw = MvAccGyro::get_raw_acc();
            send_live(&g_ctx.frame);

            // Send raw gyro data
            g_ctx.frame.answer.sub.sensor_data.type = SENS_GYRO_RAW;
            g_ctx.frame.answer.sub.sensor_data.data.raw = MvAccGyro::get_raw_gyro();
            send_live(&g_ctx.frame);

#ifdef MV_ACC_GYRO_DMP_EN
            // Send quat data
            g_ctx.frame.answer.sub.sensor_data.type = SENS_QUAT;
            g_ctx.frame.answer.sub.sensor_data.data.quat = MvAccGyro::get_quat();
            send_live(&g_ctx.frame);

#ifdef MV_ACC_GYRO_DMP_EULER_EN
            // Send euler data
            g_ctx.frame.answer.sub.sensor_data.type = SENS_EULER;
            g_ctx.frame.answer.sub.sensor_data.data.euler = MvAccGyro::get_euler();
            send_live(&g_ctx.frame);
#endif //#ifdef MV_ACC_GYRO_DMP_EULER_EN

#ifdef MV_ACC_GYRO_DMP_GRAV_EN
            // Send gravity data
            g_ctx.frame.answer.sub.sensor_data.type = SENS_GRAVITY;
            g_ctx.frame.answer.sub.sensor_data.data.gravity = MvAccGyro::get_gravity();
            send_live(&g_ctx.frame);
#endif //#ifdef MV_ACC_GYRO_DMP_GRAV_EN

#endif //#ifdef MV_ACC_GYRO_DMP_EN

            //Serial.print("Mv etime:");
            //Serial.println(micros() - g_ctx.live_ctl.time_stamp);
        }
    }
}
