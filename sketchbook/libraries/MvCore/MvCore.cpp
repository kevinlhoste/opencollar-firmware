#include "MvCore.h"

#include "MvCom.h"
#include "MvSens.h"

// TODO: add this struct in the class
static struct
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
    MvStorage *storage;
    uint8_t version[3] = {0, 0, 1};
    char button;
    int pin_button;
    int pin_led;
    int sens_addr;
} g_ctx;

void MvCore::setup(MvStorage *storage, MvFrameHandler *fhandler,
                   int sens_addr, int pin_button, int pin_led)
{
    int i;

    /* Initialize context */
    g_ctx.live_ctl.ref = 0;
    g_ctx.live_ctl.time_stamp = 0;
    g_ctx.storage = storage;
    g_ctx.pin_button = pin_button;
    g_ctx.pin_led = pin_led;
    g_ctx.fhandler = fhandler;
    g_ctx.sens_addr = sens_addr;

    g_ctx.button = 0;
    if (g_ctx.pin_button > 0)
        pinMode(g_ctx.pin_button, INPUT);
    pinMode(g_ctx.pin_led, OUTPUT);

    /* Initialize the storage if it is not yet initialized */
    if(storage->status() < 0)
        storage->reset();

    /* Set the stored configuration */
    g_ctx.live_ctl.period = 1000000/storage->get_cfg(CFG_ID_SAMPLING_RATE);
    MvSens::set_acc_sens(storage->get_cfg(CFG_ID_ACC_SENS));
    MvSens::set_gyro_sens(storage->get_cfg(CFG_ID_GYRO_SENS));
    /* Save the real value in case of error */
    storage->set_cfg(CFG_ID_ACC_SENS, MvSens::get_acc_sens());
    storage->set_cfg(CFG_ID_GYRO_SENS, MvSens::get_gyro_sens());
}

static void send_ack_nack(struct frame *frame, MvFrameHandler *fhandler, int ans_err)
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
        // Serial.print("PANIC!!! Write error");
        while(1);
    }
}

static bool check_live_time(unsigned long time_stamp, unsigned long period)
{
    if(((time_stamp + period) < time_stamp) //overflow case
       &&(micros() > time_stamp))
    {
        return false;
    }
    if(micros() >= (time_stamp + period))
    {
        return true;
    }
    return false;
}

static void add_com_to_live_list(MvCom *com)
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

static void remove_com_from_live_list(MvCom *com)
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

static void send_config(void)
{
    unsigned int i;

    g_ctx.frame.answer.id = ANS_ID_CONFIG_GET;

    for (i = 0; i < CFG_ID_LIST_SIZE; i++)
    {
        g_ctx.frame.answer.sub.cfg.id = cfg_id_list[i].id;
        g_ctx.frame.answer.sub.cfg.value = g_ctx.storage->get_cfg(cfg_id_list[i].id);
        g_ctx.fhandler->write_frame(&g_ctx.frame);

    }
}

static void start_rec(void)
{
    MvCom *aux_com;
    g_ctx.storage->rewind();
    aux_com = g_ctx.frame.com;
    g_ctx.frame.com = g_ctx.storage;
    send_config();
    g_ctx.frame.com = aux_com;
    add_com_to_live_list(g_ctx.storage);
    digitalWrite(g_ctx.pin_led,1);
}

static void send_live(struct frame *frame)
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

void MvCore::loop()
{
    int ans_err = ANS_NACK_UNKNOWN_CMD;
    int read_err = g_ctx.fhandler->read_frame(&g_ctx.frame);

    /* check button state */
    if(g_ctx.pin_button > 0 && g_ctx.button != digitalRead(g_ctx.pin_button))
    {
        g_ctx.button = digitalRead(g_ctx.pin_button);
        if(g_ctx.button)
        {
            start_rec();
        }
        else
        {
            remove_com_from_live_list(g_ctx.storage);
            digitalWrite(g_ctx.pin_led,0);
        }
    }

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
                    ans_err = MvSens::open(g_ctx.sens_addr);

                if (!ans_err)
                    add_com_to_live_list(g_ctx.frame.com);

                send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ans_err);
                break;

            case CMD_LIVE_STOP:
                ans_err = 0;

                remove_com_from_live_list(g_ctx.frame.com);

                /* If there is no more com ports in live mode */
                if (!g_ctx.live_ctl.ref)
                    ans_err = MvSens::close();

                send_ack_nack(&g_ctx.frame, g_ctx.fhandler, 0);
                break;

            case CMD_CONFIG_SET:
                ans_err = 0;

                // Change configuration on the accgyro sensor
                switch(g_ctx.frame.cmd.sub.cfg.id)
                {
                    case CFG_ID_ACC_SENS:
                        ans_err = MvSens::set_acc_sens(g_ctx.frame.cmd.sub.cfg.value);
                        break;
                    case CFG_ID_GYRO_SENS:
                        ans_err = MvSens::set_gyro_sens(g_ctx.frame.cmd.sub.cfg.value);
                        break;
                    case CFG_ID_SAMPLING_RATE:
                        g_ctx.live_ctl.period = 1000000/g_ctx.frame.cmd.sub.cfg.value;
                        break;
                }

                // Change the config on the flash
                if (!ans_err)
                    ans_err = g_ctx.storage->set_cfg((enum cfg_id)g_ctx.frame.cmd.sub.cfg.id, g_ctx.frame.cmd.sub.cfg.value);

                send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ans_err);

                break;

            case CMD_SWITCH_MODE:
                // This is a command to the frame handler
                ans_err = g_ctx.fhandler->exec_com_cmd(&g_ctx.frame);
                send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ans_err);
                // TODO: write the new mode into the flash
                break;

            // TODO: in the real app support flash instructions
            case CMD_REC_START:
                if(g_ctx.storage->status() != 0)
                 {
                        send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ANS_NACK_MEMORY_UNAVAILABLE);
                 }
                 else
                 {
                    start_rec();
                    send_ack_nack(&g_ctx.frame, g_ctx.fhandler, 0);
                 }
                 break;
            case CMD_REC_STOP:
                if(g_ctx.storage->status() != 0)
                {
                        send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ANS_NACK_MEMORY_UNAVAILABLE);
                }
                else
                {
                    remove_com_from_live_list(g_ctx.storage);
                    digitalWrite(g_ctx.pin_led,0);
                    send_ack_nack(&g_ctx.frame, g_ctx.fhandler, 0);
                }
                break;
            case CMD_REC_PLAY:
                if(g_ctx.storage->status() != 0)
                {

                    send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ANS_NACK_MEMORY_UNAVAILABLE);
                }
                else
                {
                    g_ctx.storage->rewind();
                    while(g_ctx.fhandler->read_answer_frame(&g_ctx.frame,g_ctx.storage) == SUCCESS_FRAME_READ)
                    {
                        if(g_ctx.frame.answer.id == ANS_ID_LIVE)
                        {
                            g_ctx.frame.answer.id = ANS_ID_REC_PLAY;
                        }
                        g_ctx.fhandler->write_frame(&g_ctx.frame);
                        // this is too fast causing overflow on the serial, delay a little
                        // TODO: check a better way to avoid overflow
                        delay(1);
                    }
                    send_ack_nack(&g_ctx.frame, g_ctx.fhandler, 0);
                }
                break;
            case CMD_CONFIG_GET:
                    send_config();
                   send_ack_nack(&g_ctx.frame, g_ctx.fhandler, 0);
                break;
            case CMD_REC_CLEAR:
                if(g_ctx.storage->status() != 0)
                {
                    send_ack_nack(&g_ctx.frame, g_ctx.fhandler, ANS_NACK_MEMORY_UNAVAILABLE);
                }
                else
                {
                    g_ctx.storage->clear_recordings();
                    send_ack_nack(&g_ctx.frame, g_ctx.fhandler, 0);
                }
                break;
            case CMD_VERSION_GET:
                g_ctx.frame.answer.id = ANS_ID_VERSION;
                g_ctx.frame.answer.sub.version[0] = g_ctx.version[0];
                g_ctx.frame.answer.sub.version[1] = g_ctx.version[1];
                g_ctx.frame.answer.sub.version[2] = g_ctx.version[2];
                g_ctx.fhandler->write_frame(&g_ctx.frame);
                break;
            default:
                //Serial.println("Couldn't find cmd");
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
        //Serial.print("PANIC!!! READ ERROR!");
        while(1);
    }

    // Deal with live
    if (g_ctx.live_ctl.ref)
    {
        // Prepare values
        // This is called every loop because if the DMP is being used, we must call this function
        // to read from the FIFO
        MvSens::read();

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
            if(g_ctx.storage->get_cfg(CFG_ID_LIVE_ACC_RAW_EN))
            {
                g_ctx.frame.answer.sub.sensor_data.type = SENS_ACC_RAW;
                g_ctx.frame.answer.sub.sensor_data.data.raw = MvSens::get_raw_acc();
                send_live(&g_ctx.frame);
            }
            // Send raw gyro data
            if(g_ctx.storage->get_cfg(CFG_ID_LIVE_GYRO_RAW_EN))
            {
                g_ctx.frame.answer.sub.sensor_data.type = SENS_GYRO_RAW;
                g_ctx.frame.answer.sub.sensor_data.data.raw = MvSens::get_raw_gyro();
                send_live(&g_ctx.frame);
            }
            // Send raw mag data
            if(g_ctx.storage->get_cfg(CFG_ID_LIVE_MAG_RAW_EN))
            {
                g_ctx.frame.answer.sub.sensor_data.type = SENS_MAG_RAW;
                g_ctx.frame.answer.sub.sensor_data.data.raw = MvSens::get_raw_mag();
                send_live(&g_ctx.frame);
            }

#ifdef MV_ACC_GYRO_DMP_EN
            // Send quat data
            if(g_ctx.storage->get_cfg(CFG_ID_LIVE_QUATERNION_EN))
            {
                g_ctx.frame.answer.sub.sensor_data.type = SENS_QUAT;
                g_ctx.frame.answer.sub.sensor_data.data.quat = MvSens::get_quat();
                send_live(&g_ctx.frame);
            }

#endif //#ifdef MV_ACC_GYRO_DMP_EN

            //Serial.print("Mv etime:");
            //Serial.println(micros() - g_ctx.live_ctl.time_stamp);
        }
    }
}
