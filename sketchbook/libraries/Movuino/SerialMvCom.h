#ifndef SERIALMVCOM_H
#define SERIALMVCOM_H

#include "mv_crc"

#define SMC_SERIAL_OBJECTS 2
#define SMC_SYNC_BYTE1 0x55
#define SMC_SYNC_BYTE2 0x36

/**
 * SerialMvCom
 *
 * @brief Implements the abstract class MvCom for the serial ports
 */
class SerialMvCom : public MvCom {
    public:
        SerialMvCom(HardwareSerial *serial, int baudrate);
        int write_frame(char *frame, int size);
        int read_frame(char *frame, int *size);
        int set_mode(enum mvCom_mode mode);
        enum mvCom_mode get_mode(void);
    private:
        /** This variable contains all serial ports that are being used */
        static SerialMvCom *Smc_list[SMC_SERIAL_OBJECTS];
        /** Contains the amount of serial ports being used */
        static int Smc_list_size = 0;
        void update(void);
        /** Bytes that comes from a serial port and might go to the buffer */
        char shift_byte;
        /** Store the received frame */
        char buffer[BUFFER_SIZE];
        /** Sequence number of the next frame to be sent */
        unsigned char sequencer;
        /** Store the curent mode of the object */
        enum mvCom_mode mode;
        /** Current size of the incoming frame */
        uint8_t frame_size;
        /** Total size of the incoming frame (when known before complete reception) */
        uint8_t total_frame_size;
        /** Keep the state of the object */
        enum {
            SMC_EMPTY,
            SMC_SYNC1,
            SMC_SYNC2,
            SMC_INCOMING,
            SMC_READY
        } state;
        /** Pointer to the serial port that will be used */
        HardwareSerial *serial;
};

/**
 * _update_byte
 *
 * @brief private function called when there is data at the serial
 * port and the object is in MVCOM_BINARY mode
 */
static void SerialMvCom::_update_byte(void)
{
    switch(this->state)
    {
        /** Receives the first sync byte */
        case SMC_EMPTY:
        case SMC_READY:
            this->shift_byte = this->serial->read();
            if(this->shift_byte == SMC_SYNC_BYTE1)
            {
                this->state = SMC_SYNC1;
            }
            break;
        /** Receives the second sync byte */
        case SMC_SYNC1:
            this->shift_byte = this->serial->read();
            if(this->shift_byte == SMC_SYNC_BYTE2)
            {
                this->state = SMC_SYNC2;
            }
            else
            {
                this->state = SMC_EMPTY;
            }
            break;
        /** Receives the frame size */
        case SMC_SYNC2:
            this->total_frame_size = this->serial->read() + 2;
            this->frame_size = 0;
            if(this->total_frame_size > BUFFER_SIZE)
            {
                //TODO: Error case
            }
            this->state = SMC_INCOMING;
            break;
        /** Receives the rest of the frame and verify the crc */
        case SMC_INCOMING:
            if(this->frame_size < this->total_frame_size)
            {
                this->buffer[this->frame_size++] = this->serial->read();
            }
            if(this->frame_size == this->total_frame_size)
            {
                uint16_t crc;
                uint16_t *crc_p;
                crc_p = (uint16_t*)&(this->buffer[this->frame_size - 3]);
                crc = mv_crc(0xFFFF,(uint16_t*)this->buffer, this->frame_size -2);
                if(crc != *crc_p)
                {
                    this->state = SMC_EMPTY;
                }
                this->state = SMC_READY;
            }
            break;
    }
}

/**
 * _update_ascii
 *
 * @brief private function called when there is data at the serial
 * port and the object is in MVCOM_ASCII mode
 */
static void SerialMvCom::_update_ascii(void)
{
    switch(this->state)
    {
        /** receives incoming characters until '\n' is found */
        case SMC_INCOMING:
            this->buffer[this->frame_size] = this->shift_byte;
            this->frame_size++;
            this->shift_byte = this->serial->read();
            if(this->shift_byte == '\n')
            {
                this->state = SMC_READY;
            }
            break;
        /** Receives the first character from a frame */
        case SMC_READY:
        case SMC_EMPTY:
            this->shift_byte = this->serial->read();
            this->state = SMC_INCOMING;
            this->frame_size = 0;
            break;
        default:
            //TODO: treat errors??
            break;
    }
}

/**
 * update
 *
 * @brief Function called when there is data available at the serial port
 */
void SerialMvCom::update(void)
{
    while(this->serial->available())
    {
        if(this->mode == MVCOM_ASCII)
        {
            this->_update_ascii();
        }
        else
        {
            this->_update_byte();
        }
    }
}

/**
 * _read_frame_ascii
 *
 * @brief copy the buffered frame to the user when in mode MVCOM_ASCII 
 */
void SerialMvCom::_read_frame_ascii(char *frame, int *size)
{
    int i;
    for(i = 0; i < this->frame_size; i++)
    {
        frame[i] = this->buffer[i];
    }
    *size = i;
}

/**
 * _read_frame_byte
 *
 * @brief copy the buffered frame to the user when in mode MVCOM_BINARY
 */
void SerialMvCom::_read_frame_byte(char *frame, int *size)
{
    int i;
    for(i = 0; i < this->frame_size - 2; i++)
    {
        frame[i] = this->buffer[i];
    }
    *size = i;
}

/**
 * @see MvCom::read_frame
 */
int SerialMvCom::read_frame(char *frame, int *size)
{
    if(this->state == SMC_READY)
    {
        if(this->mode == MVCOM_ASCII)
        {
            this->_read_frame_ascii(frame,size);
        }
        else
        {
            this->_read_frame_byte(frame,size);
        }
        this->state = SMC_EMPTY;
    }
}

/**
 * _write_frame_ascii
 *
 * @brief Send a user frame to the serial in MVCOM_ASCII mode
 */
SerialMcCom::_write_frame_ascii(char *frame, int size)
{
    /* print the sequencer */
    this->serial->write("S:");
    this->serial->print((int)this->sequencer);
    this->serial->write(' ');
    /* print the frame */
    this->serial->write(frame, size);
    /* print the footer */
    this->serial->write('\n');
    this->serial->flush();
    /* update the sequencer */
    this->sequencer++;
}

/**
 * _write_frame_byte
 *
 * @brief Send a user frame to the serial in MVCOM_BINARY mode
 */
SerialMcCom::_write_frame_ascii(char *frame, int size)
SerialMcCom::_write_frame_byte(char *frame, int size)
{
    uint16_t crc;

    /* calculate crc */
    crc = mv_crc(0xFFFF, &this->sequencer, 1);
    crc = mv_crc(crc, frame, size);
    /* print the sync bytes */
    this->serial->write(SMC_SYNC_BYTE1);
    this->serial->write(SMC_SYNC_BYTE2);
    /* print the size of the frame (size + sequencer byte) */
    this->serial->write(size + 1);
    /* print the sequencer */
    this->serial->write(this->sequencer);
    /* print the frame */
    this->serial->write(frame,size);
    /* print the crc (footer) */
    this->serial->write((char*)&crc,sizeof(crc));
    this->serial->flush();
    /* update the sequencer */
    this->sequencer++;
}

/**
 * @see MvCom::write_frame
 */
SerialMvCom::write_frame(char *frame, int size)
{
    if(this->mode == MVCOM_ASCII)
    {
        this->_write_frame_ascii(frame,size);
    }
    else
    {
        this->_write_frame_byte(frame,size);
    }
}

/**
 * @see MvCom::set_mode
 */
int SerialMvCom::set_mode(enum mvCom_mode mode)
{
    this.mode = mode;
    this.state = SMC_EMPTY; 
    return 0;
}

/**
 * @see MvCom::get_mode
 */
enum mvCom_mode SerialMvCom::get_mode(void)
{
    return this->mode;
}

/**
 * SerialMvCom
 *
 * @brief initialize a structure to be used as a MvCom with a serial port
 */
SerialMvCom::SerialMvCom(HardwareSerial *serial, int baudrate)
{
    this->serial = serial;
    this->frame_size = 0;
    this->mode = MVCOM_ASCII;
    this->total_frame_size = 0;
    this->state = SMC_EMPTY;
    this->sequencer = 0;

    /* if there is still space at the list of SerialMvCom, add this port */
    if(SerialMvCom.Smc_list_size < SMC_SERIAL_OBJECTS)
    {
        SerialMvCom.Smc_list[SerialMvCom.Smc_list_size] = this;
        SerialMvCom.Smc_list_size++;
        serial->begin(baudrate);
    }
}

/**
 * SerialEvent
 *
 * @brief Function called automaticaly when a byte is received at a serial port.
 * Verifies for each of the inscribed ports which one must be updated
 */
void SerialEvent(void)
{
    int i;
    for(i = 0; i < SerialMvCom.Smc_list_size; i++)
    {
        SerialMvCom.Smc_list[i]->update();
    }
}

#endif //SERIALMVCOM_H
