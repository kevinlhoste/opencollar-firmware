#include "MvFrameHandler.h"
#include "MvCom.h"
//#include "MvAccGyro.h"

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

        void set_mode(enum mvCom_mode mode)
        {
            Serial.print("MvCom set_mode\n");
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
//MvAccGyro g_accgyro;
struct frame g_frame;


void setup()
{
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

void loop()
{
    int ans_err = ANS_NACK_UNKNOWN_CMD;
    int read_err = g_fhandler->read_frame(&g_frame);

    if (SUCCESS_FRAME_READ == read_err)
    {
        switch(g_frame.cmd.id)
        {
            case CMD_PING:
                send_ack_nack(&g_frame, g_fhandler, 0);
                break;

            case CMD_LIVE_START:
                //ans_err = g_accgyro.live_start();
                send_ack_nack(&g_frame, g_fhandler, ans_err);
                break;

            case CMD_LIVE_STOP:
                //ans_err = g_accgyro.live_stop();
                send_ack_nack(&g_frame, g_fhandler, ans_err);
                break;

            case CMD_CONFIG_SET:
                switch(g_frame.cmd.sub.cfg.value)
                {
                    case CFG_ID_ACC_SENS:
                    case CFG_ID_GYRO_SENS:
                    case CFG_ID_SAMPING_RATE:
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
                break;

            case CMD_REC_START:
                // TODO
            case CMD_REC_STOP:
                // TODO
            case CMD_REC_PLAY:
                // TODO
            case CMD_REC_CLEAR:
                // TODO
            case CMD_HELP:
                // TODO
            case CMD_VERSION_GET:
                // TODO
            case CMD_CONFIG_GET:
                // TODO
            default:
                send_ack_nack(&g_frame, g_fhandler, ANS_NACK_UNKNOWN_CMD);
                break;
        }
    }
    else if(ERR_BAD_FRAME == read_err)
    {
        send_ack_nack(&g_frame, g_fhandler, ANS_NACK_BAD_FRAME_FORMAT);
    }
    else if(ERR_BAD_PARAM == read_err)
    {
        // This should never happen
        Serial.print("PANIC!!! READ ERROR!");
        while(1);
    }
}
