

/* ============================================
I2Cdev device library code is placed under the MIT license
Copyright (c) 2011 Jeff Rowberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/



// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#include "Wire.h"
#include <SPI.h>
#include "DataFlash.h"
#include <EEPROM.h>

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"

#include "accelgyro.h"
#include "flashMem.h"
#include "serial_frame.h"

/* Global variables */

#define MY_ID 0

char frame_type;

/* operating modes */
#define STANDBY_MODE 0
#define LIVE_MODE (STANDBY_MODE + 1)
#define WRITE_MODE (LIVE_MODE + 1)
#define LIVE_SERIAL_MODE (WRITE_MODE + 1)
char run_mode;
char is_write_mode;
char button;

void
startupDelay()
{
    unsigned long int startupTime;
    startupTime = millis();
    if(startupTime < 3000) delay(3000 - startupTime);
}

void 
setup(void)
{
    pinMode(4, OUTPUT);
    pinMode(8, INPUT);
    
    button = 0;
    sf_setup();
    accelgyro_setup();
    flash_setup();
    
    run_mode = STANDBY_MODE;
    is_write_mode = 0;

    startupDelay();
}


void
mode_handler(void)
{
    unsigned long int time0;
    unsigned long int time1;
    unsigned long int delay_time;

    if(run_mode == STANDBY_MODE)
    {
        /* TODO */
        return;
    }

    if((run_mode == LIVE_MODE) || 
       (run_mode == WRITE_MODE) ||
       (run_mode == LIVE_SERIAL_MODE))
    {
        time0 = micros();
        accelgyro_get();
        switch(run_mode)
        {
            case LIVE_MODE:
                serial_print_str("L ");
                print_accelgyro(BYTE_MODE);
                break;
            case LIVE_SERIAL_MODE:
                serial_print_str("l ");
                print_accelgyro(CHAR_MODE);
                break;
            case WRITE_MODE:
                if(!is_write_mode)
                {
                    is_write_mode = 1;
                    digitalWrite(4,1);
                    flash_write_mode_start();
                }
                flash_write_accelgyro();
                break;
        }

        delay_time = 1000000/accelgyro.sampling_rate;
        time1 = micros();
        if((time1 - time0) < delay_time) {
            delayMicroseconds((delay_time - (time1 - time0)));
            //delay((delay_time- (time1 - time0))/1000);
        }
    }
}

void
frame_handler(void)
{
    int aux;
    switch(frame_type)
    {
        case LIVE_MODE_FRAME:
            run_mode = LIVE_MODE;
            break;

        case INFORMATION_FRAME:
            /* send message with informations */
            serial_print_char(INFORMATION_FRAME);
            serial_print_char(' ');
            serial_print_int(MY_ID);
            serial_print_char(' ');
            serial_print_int(accelgyro.acc_range);
            serial_print_char(' ');
            serial_print_int(accelgyro.gyro_range);
            serial_print_char(' ');
            serial_println_int(accelgyro.sampling_rate);
            break;

        case PING_FRAME:
            /* send a pong */
            serial_println_char(PING_FRAME);
            break;

        case WRITE_MODE_FRAME:
            run_mode = WRITE_MODE;
        case ERASE_MEMORY_FRAME:
            flash_erase();
            break;

        case READ_MEMORY_FRAME:
            flash_read_accelgyro(CHAR_MODE);
            run_mode = STANDBY_MODE;
            break;

        case READ_MEMORY_BYTE_FRAME:
            flash_read_accelgyro(BYTE_MODE);
            run_mode = STANDBY_MODE;
            break;

        case QUIT_FRAME:
            run_mode = STANDBY_MODE;
            break;

        case LIVE_SERIAL_FRAME:
            run_mode = LIVE_SERIAL_MODE;
            break;

        case ACCEL_RANGE_FRAME:
            aux = SF_RANGE;
            ACCELGYRO_SET_ACCELRANGE(aux);
            serial_println_char(PING_FRAME);
            break;
 
        case GYRO_RANGE_FRAME:
            aux = SF_RANGE;
            ACCELGYRO_SET_GYRORANGE(aux);
            serial_println_char(PING_FRAME);
            break;

        case SAMPLING_RATE_FRAME:
            accelgyro.sampling_rate = sf_get_sampling_rate();
            serial_println_char(PING_FRAME);
            break;

        default:
            /* Send unknown frame message */
            serial_println_char(UNKNOWN_FRAME);
            break;

    }
}

int loop_counter = 0;


void
loop()
{
    //loop_counter++;
    //if(loop_counter == 100000) loop_counter = 0;
    //if(loop_counter == 0) Serial.println("loop");
    if(digitalRead(8) == HIGH && button == 0)
    {
        button = 1;
        frame_type = WRITE_MODE_FRAME;
    }
    else if(digitalRead(8) == LOW && button == 1)
    {
        button = 0;
        frame_type = QUIT_FRAME;
    }
    else
    {
        frame_type = sf_getFrame();
    }
    if(frame_type)
    {
        if(is_write_mode)
        {
            is_write_mode = 0;
            digitalWrite(4,0);
            flash_write_buffer();
        }
        frame_handler();
    }
    mode_handler();
}
