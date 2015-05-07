#ifndef SERIAL_FRAME_H
#define SERIAL_FRAME_H

#include "serial_functions.h"

/* frame identifiers */
#define NO_FRAME 0

/* collar frames */
#define UNKNOWN_FRAME 'U'
#define MESSAGE_FRAME 'M'

/* user frames */
#define LIVE_MODE_FRAME 'L'
#define LIVE_SERIAL_FRAME 'l'
#define INFORMATION_FRAME 'i'
#define PING_FRAME 'A'
#define WRITE_MODE_FRAME 'w'
#define READ_MEMORY_FRAME 'r'
#define READ_MEMORY_BYTE_FRAME 'R'
#define ERASE_MEMORY_FRAME 'e'
#define CHECK_MEMORY_FRAME 'x'
#define CHECK_PROSSING_TRASNFERT_FRAME 't'
#define QUIT_FRAME 'q'

#define ACCEL_RANGE_FRAME '1'
#define GYRO_RANGE_FRAME '2'
#define SAMPLING_RATE_FRAME '3'
#define OUTPUT_AXES_FRAME '4'

#define FRAME_BUFFER_SIZE 32
char frame[FRAME_BUFFER_SIZE];

void sf_setup(void)
{
    Serial.begin(38400);
    Serial1.begin(9600);
}

/*
* Place received frame at frame buffer and return the type of the frame
*/
char sf_getFrame(void)
{
    char frame_type;
    int i;
    if(!serial_available()) return NO_FRAME;
    
    frame_type = serial_read();
    switch(frame_type)
    {
        case ' ':
            return NO_FRAME;
            break;
        case ACCEL_RANGE_FRAME:
        case GYRO_RANGE_FRAME:
            while(!serial_available()){}
            frame[0] = serial_read(); //first byte is useless
            while(!serial_available()){}
            
            frame[0] = serial_read(); 
            if(frame[0] < '0' || frame[0] > '3')
            {
                serial_println_str("M bad format");
                frame_type = NO_FRAME;
            } 

            break;

        case SAMPLING_RATE_FRAME:
            while(!serial_available()){}
            frame[0] = serial_read(); //first byte is useless
            i = 0;
            for(i = 0;; i++)
            {
                if(!serial_available()) continue;
                frame[i] = serial_read();
                if(frame[i] < '0' || frame[i] > '9') { break; }
            }
            if(i == 0)
            {
                serial_println_str("M bad format");
                frame_type = NO_FRAME; 
            }
            else { frame[i] = '\0'; }
            break;
   
        case OUTPUT_AXES_FRAME:
            while(!serial_available()){}
            frame[0] = serial_read(); //first byte is useless
            while(!serial_available()){}
            
            frame[0] = serial_read(); 
            if(frame[0] < '1' || frame[0] > '9')
            {
                serial_println_str("M bad format");
                frame_type = NO_FRAME;
            } 
            break;
            
        case LIVE_MODE_FRAME:
        case LIVE_SERIAL_FRAME:
        case INFORMATION_FRAME:
        case PING_FRAME:
        case WRITE_MODE_FRAME:
        case READ_MEMORY_FRAME:
        case READ_MEMORY_BYTE_FRAME:
        case ERASE_MEMORY_FRAME:
        case QUIT_FRAME:
            break;
        
        //test case
        case '\n':
            serial_println_str("M ooops, \\n");
            frame_type = NO_FRAME;
        default:
            serial_print_str("M unknown frame header ");
            serial_println_char(frame_type);
            frame_type = NO_FRAME;
            break;
    }

    return frame_type;
}

#define SF_RANGE (frame[0] - '0')

int sf_get_sampling_rate()
{
    int samp_rate;
    int i;

    samp_rate = 0;
    for(i = 0; frame[i] != '\0'; i++)
    {
        samp_rate = samp_rate*10 + frame[i] - '0';
    }
    return samp_rate;
}

#endif
