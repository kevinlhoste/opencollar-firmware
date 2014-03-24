#ifndef SERIAL_FRAME_H
#define SERIAL_FRAME_H

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
#define ERASE_MEMORY_FRAME 'e'
#define CHECK_MEMORY_FRAME 'x'
#define CHECK_PROSSING_TRASNFERT_FRAME 't'
#define QUIT_FRAME 'q'

#define ACCEL_RANGE_FRAME '1'
#define GYRO_RANGE_FRAME '2'
#define SAMPLING_RATE_FRAME '3'

#define FRAME_BUFFER_SIZE 32
char frame[FRAME_BUFFER_SIZE];

void sf_setup(void)
{
    Serial.begin(38400);
}

/*
* Place received frame at frame buffer and return the type of the frame
*/
char sf_getFrame(void)
{
    char frame_type;
    int i;
    if(!Serial.available()) return NO_FRAME;
    
    frame_type = Serial.read();
    switch(frame_type)
    {
        case ACCEL_RANGE_FRAME:
        case GYRO_RANGE_FRAME:
            frame[0] = Serial.read(); //first byte is useless
            if(Serial.available()) 
            { 
                frame[0] = Serial.read(); 
                if(frame[0] < '0' || frame[0] > '3')
                {
                    Serial.println("M bad format");
                    frame_type = NO_FRAME;
                }
            } 
            else 
            { 
                Serial.println("M bad format");
                frame_type = NO_FRAME; 
            }
            break;

        case SAMPLING_RATE_FRAME:
            frame[0] = Serial.read(); //first byte is useless
            i = 0;
            for(i = 0; Serial.available(); i++)
            {
                frame[i] = Serial.read();
                if(frame[i] < '0' || frame[i] > '9') { break; }
            }
            if(i == 0)
            {
                Serial.println("M bad format");
                frame_type = NO_FRAME; 
            }
            else { frame[i] = '\0'; }
            break;
            
        case LIVE_MODE_FRAME:
        case LIVE_SERIAL_FRAME:
        case INFORMATION_FRAME:
        case PING_FRAME:
        case WRITE_MODE_FRAME:
        case READ_MEMORY_FRAME:
        case ERASE_MEMORY_FRAME:
        case QUIT_FRAME:
            break;
        
        //test case
        case '\n':
            Serial.println("M ooops, \\n");
            frame_type = NO_FRAME;
        default:
            Serial.print("M unknown frame header ");
            Serial.println(frame_type);
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
