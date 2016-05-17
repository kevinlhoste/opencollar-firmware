#include "MvCore.h"

#include "MvCom.h"
#include "MvSens.h"

// TODO: add this struct in the class
static struct
{
    struct live_ctl
    {
        unsigned long period;
        unsigned long samp_time;
        MvCom *com_list[MAX_COM_PORTS];
        /* ref counts how many com ports are in the live mode com list*/
        int ref;
    } live_ctl;

    MvFrameHandler *fhandler;
    struct frame frame;
    MvStorage *storage;
    uint8_t version[3] = {0, 0, 1};
    int pin_button;
    int pin_led;
    int led_logicOn;
    int pin_vibrate;
    bool recording;
    unsigned long vibrate_time_stamp;
    int sens_addr;
} g_ctx;

void MvCore::setupLed(int pin, int logicOn)
{
    g_ctx.pin_led = pin;
    g_ctx.led_logicOn = logicOn;
    pinMode(g_ctx.pin_led, OUTPUT);
}


void MvCore::setup(MvStorage *storage, MvFrameHandler *fhandler,
                   int sens_addr, int pin_button, int pin_vibrate)
{
    int i;

    /* Initialize context */
    g_ctx.live_ctl.ref = 0;
    g_ctx.live_ctl.samp_time = 0;
    g_ctx.storage = storage;
    g_ctx.pin_button = pin_button;
    g_ctx.pin_vibrate = pin_vibrate;
    g_ctx.fhandler = fhandler;
    g_ctx.sens_addr = sens_addr;
    g_ctx.vibrate_time_stamp = 0;

    if (g_ctx.pin_button > 0)
        pinMode(g_ctx.pin_button, INPUT_PULLUP);
    pinMode(g_ctx.pin_vibrate, OUTPUT);

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

#define _REC_TIMEOUT (1000UL*60*REC_MIN_TIMEOUT)
struct
{
    unsigned long int accumulated;
    unsigned long int last_check;
} rec_timeout_ctx;

static int start_rec(void)
{
    MvCom *aux_com;
    int ans_err = 0;
    /* Check if the acc has already been initialized */
    if (!g_ctx.live_ctl.ref)
        ans_err = MvSens::open(g_ctx.sens_addr);
    // g_ctx.storage->rewind(); Do not overwrite the old data
    aux_com = g_ctx.frame.com;
    g_ctx.frame.com = g_ctx.storage;
    send_config();
    g_ctx.frame.com = aux_com;
    add_com_to_live_list(g_ctx.storage);
    g_ctx.recording = true;

    rec_timeout_ctx.accumulated = 0;
    rec_timeout_ctx.last_check = millis();
    return ans_err;
}

static void stop_rec(void)
{
    remove_com_from_live_list(g_ctx.storage);
    g_ctx.recording = false;
    /* If there is no more com ports in live mode */
    if (!g_ctx.live_ctl.ref)
        MvSens::close();
}

static void rec_timeout(void)
{
    unsigned long int now = millis();
    if(g_ctx.recording)
    {
        rec_timeout_ctx.accumulated += now - rec_timeout_ctx.last_check;
        rec_timeout_ctx.last_check = now;

        if (rec_timeout_ctx.accumulated >= _REC_TIMEOUT)
        {
            stop_rec();
            g_ctx.storage->sleep();
        }
    }
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

// TODO: clean all this code
static bool button_pressed(int bt_state)
{
    static bool pres = false;
    static unsigned int ts;

    // First press
    if (!pres && bt_state == LOW)
    {
        pres = true;
        ts = millis();
    }
    // Release time after one second
    else if (pres && bt_state == HIGH)
    {
        pres = false;
        if (millis() - ts > 50)
            return true;
    }
    return false;
}

// TODO: clean this code
static void led_pattern(int pin, int logicOn, bool rec)
{
    static unsigned int ts = millis();
    static bool state = false;
    static bool first = true;

    // Turn off
    if (state)
    {
        if ((unsigned int)(millis() - ts) > 25)
        {
            digitalWrite(pin, !logicOn);
            state = false;
            ts = millis();
        }
    }
    // Turn on
    else
    {
        unsigned int per;
        if (first && rec)
            per = 100;
        else
            per = 2000;

        if ((unsigned int)(millis() - ts) > per)
        {
            digitalWrite(pin, logicOn);
            state = true;
            ts = millis();
            if (first && rec)
                first = false;
            else
                first = true;
        }
    }
}

void MvCore::loop()
{
    int ans_err = ANS_NACK_UNKNOWN_CMD;
    int read_err = g_ctx.fhandler->read_frame(&g_ctx.frame);

    led_pattern(g_ctx.pin_led, g_ctx.led_logicOn, g_ctx.recording);

    rec_timeout();

    /* Check if its time to turn off pin_vibrate */
    if (g_ctx.vibrate_time_stamp &&
       (micros() - g_ctx.vibrate_time_stamp < VIBRATE_TIMEOUT))
    {
        digitalWrite(g_ctx.pin_vibrate, LOW);
    }

    /* check button state */
    if(g_ctx.pin_button > 0 &&
       g_ctx.storage->status() == 0 &&
       button_pressed(digitalRead(g_ctx.pin_button)))
    {
        if(g_ctx.recording)
            stop_rec();
        else
            start_rec();
    }

    if (SUCCESS_FRAME_READ == read_err)
    {
        switch(g_ctx.frame.cmd.id)
        {
            case CMD_PING:
                //Serial.print("Mv Live period:");
                //Serial.println(g_ctx.live_ctl.period);
                //Serial.print("Mv Live ts:");
                //Serial.println(g_ctx.live_ctl.samp_time);
                //Serial.print("Mv Time now:");
                //Serial.println(micros());

                send_ack_nack(&g_ctx.frame, g_ctx.fhandler, 0);
                break;

            case CMD_VIBRATE:
                digitalWrite(g_ctx.pin_vibrate, HIGH);
                g_ctx.vibrate_time_stamp = micros();
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
                g_ctx.live_ctl.samp_time = micros();
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
                    stop_rec();
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
                        // delay(15);
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
        // FIXME
        // Prepare values
        // This is called every loop because if the DMP is being used, we must call this function
        // to read from the FIFO
        // MvSens::read();

        // Check if its time to print the next data
        if(micros() - g_ctx.live_ctl.samp_time < ((unsigned long)(-1))/2)
        {
            // FIXME
            MvSens::read();

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

            // Send raw alt data
            if(g_ctx.storage->get_cfg(CFG_ID_LIVE_ALT_RAW_EN))
            {
                g_ctx.frame.answer.sub.sensor_data.type = SENS_ALT_RAW;
                g_ctx.frame.answer.sub.sensor_data.data.single = MvSens::get_raw_alt();
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
            //Serial.println(micros() - g_ctx.live_ctl.samp_time);

            // Update timestamp
            g_ctx.live_ctl.samp_time += g_ctx.live_ctl.period;
            // If it is a past time, set it to now
            if(micros() - g_ctx.live_ctl.samp_time < ((unsigned long)(-1))/2)
            {
                g_ctx.live_ctl.samp_time = micros();
            }
        }
    }
}
