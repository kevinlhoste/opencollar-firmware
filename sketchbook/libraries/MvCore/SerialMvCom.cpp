#include "SerialMvCom.h"

/**
 * _update_byte
 *
 * @brief private function called when there is data at the serial
 * port and the object is in MVCOM_BINARY mode
 */
void SerialMvCom::_update_byte(void)
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
                crc = mv_crc(0xFFFF,(unsigned char*)this->buffer, this->frame_size -2);
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
void SerialMvCom::_update_ascii(void)
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
    this->update();
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
        return 0;
    }
    *size = 0;
    return -1;
}

/**
 * _write_frame_ascii
 *
 * @brief Send a user frame to the serial in MVCOM_ASCII mode
 */
int SerialMvCom::_write_frame_ascii(char *frame, int size)
{
    /* print the sequencer */
    this->serial->write("S: ");
    this->serial->print((int)this->sequencer);
    this->serial->write(' ');
    /* print the frame */
    this->serial->write((STR_CAST)frame, size);
    /* print the footer */
    this->serial->write('\n');
    this->serial->flush();
    /* update the sequencer */
    this->sequencer++;
    return 0;
}

/**
 * _write_frame_byte
 *
 * @brief Send a user frame to the serial in MVCOM_BINARY mode
 */
int SerialMvCom::_write_frame_byte(char *frame, int size)
{
    uint16_t crc;

    /* calculate crc */
    crc = mv_crc(0xFFFF, &this->sequencer, 1);
    crc = mv_crc(crc, (unsigned char*)frame, size);
    /* print the sync bytes */
    this->serial->write(SMC_SYNC_BYTE1);
    this->serial->write(SMC_SYNC_BYTE2);
    /* print the size of the frame (size + sequencer byte) */
    this->serial->write(size + 1);
    /* print the sequencer */
    this->serial->write(this->sequencer);
    /* print the frame */
    this->serial->write((STR_CAST)frame,size);
    /* print the crc (footer) */
    this->serial->write((STR_CAST)&crc,sizeof(crc));
    this->serial->flush();
    /* update the sequencer */
    this->sequencer++;
    return 0;
}

/**
 * @see MvCom::write_frame
 */
int SerialMvCom::write_frame(char *frame, int size)
{
    if(this->mode == MVCOM_ASCII)
    {
        return this->_write_frame_ascii(frame,size);
    }
    else
    {
        return this->_write_frame_byte(frame,size);
    }
}

/**
 * @see MvCom::set_mode
 */
int SerialMvCom::set_mode(enum mvCom_mode mode)
{
    this->mode = mode;
    this->state = SMC_EMPTY; 
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
SerialMvCom::SerialMvCom(SERIAL_CLASS *serial)
{
    this->serial = serial;
    this->frame_size = 0;
    this->mode = MVCOM_ASCII;
    this->total_frame_size = 0;
    this->state = SMC_EMPTY;
    this->sequencer = 0;
}
