#include "GenMvCom.h"

#include "mv_crc.h"

/**
 * _update_byte
 *
 * @brief private function called when there is data at the com
 * port and the object is in MVCOM_BINARY mode
 */
void GenMvCom::_update_byte(void)
{
    switch(this->state)
    {
        /** Receives the first sync byte */
        case GMC_EMPTY:
        case GMC_READY:
            this->shift_byte = this->read_byte();
            if(this->shift_byte == GMC_SYNC_BYTE1)
            {
                this->state = GMC_SYNC1;
            }
            break;
        /** Receives the second sync byte */
        case GMC_SYNC1:
            this->shift_byte = this->read_byte();
            if(this->shift_byte == GMC_SYNC_BYTE2)
            {
                this->state = GMC_SYNC2;
            }
            else
            {
                this->state = GMC_EMPTY;
            }
            break;
        /** Receives the frame size */
        case GMC_SYNC2:
            this->total_frame_size = this->read_byte() + 2;
            this->frame_size = 0;
            if(this->total_frame_size > BUFFER_SIZE)
            {
                //TODO: Error case
            }
            this->state = GMC_INCOMING;
            break;
        /** Receives the rest of the frame and verify the crc */
        case GMC_INCOMING:
            if(this->frame_size < this->total_frame_size)
            {
                this->buffer[this->frame_size++] = this->read_byte();
            }
            if(this->frame_size == this->total_frame_size)
            {
                uint16_t crc;
                uint16_t *crc_p;
                crc_p = (uint16_t*)&(this->buffer[this->frame_size - 3]);
                crc = mv_crc(0xFFFF,(unsigned char*)this->buffer, this->frame_size -2);
                if(crc != *crc_p)
                {
                    this->state = GMC_EMPTY;
                }
                this->state = GMC_READY;
            }
            break;
    }
}

/**
 * _update_ascii
 *
 * @brief private function called when there is data at the com
 * port and the object is in MVCOM_ASCII mode
 */
void GenMvCom::_update_ascii(void)
{
    switch(this->state)
    {
        /** receives incoming characters until '\n' is found */
        case GMC_INCOMING:
            this->buffer[this->frame_size] = this->shift_byte;
            this->frame_size++;
            this->shift_byte = this->read_byte();
            if(this->shift_byte == '\n' || this->shift_byte == ';')
            {
                this->state = GMC_READY;
            }
            break;
        /** Receives the first character from a frame */
        case GMC_READY:
        case GMC_EMPTY:
            this->shift_byte = this->read_byte();
            this->state = GMC_INCOMING;
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
 * @brief Function called when there is data available at the com port
 */
void GenMvCom::update(void)
{
    while(this->available_byte())
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
void GenMvCom::_read_frame_ascii(char *frame, int *size)
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
void GenMvCom::_read_frame_byte(char *frame, int *size)
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
int GenMvCom::read_frame(char *frame, int *size)
{
    this->update();
    if(this->state == GMC_READY)
    {
        if(this->mode == MVCOM_ASCII)
        {
            this->_read_frame_ascii(frame,size);
        }
        else
        {
            this->_read_frame_byte(frame,size);
        }
        this->state = GMC_EMPTY;
        return 0;
    }
    *size = 0;
    return -1;
}

/**
 * _write_frame_ascii
 *
 * @brief Send a user frame to the com in MVCOM_ASCII mode
 */
int GenMvCom::_write_frame_ascii(char *frame, int size)
{
    /* print the sequencer */
    this->write_bytes("S: ");
    this->write_bytes((int)this->sequencer);
    this->write_bytes(' ');
    /* print the frame */
    this->write_bytes(frame, size);
    /* print the footer */
    this->write_bytes('\n');
    this->flush_bytes();
    /* update the sequencer */
    this->sequencer++;
    return 0;
}

/**
 * _write_frame_byte
 *
 * @brief Send a user frame to the com in MVCOM_BINARY mode
 */
int GenMvCom::_write_frame_byte(char *frame, int size)
{
    uint16_t crc;

    /* calculate crc */
    crc = mv_crc(0xFFFF, &this->sequencer, 1);
    crc = mv_crc(crc, (unsigned char*)frame, size);
    /* print the sync bytes */
    this->write_bytes(GMC_SYNC_BYTE1);
    this->write_bytes(GMC_SYNC_BYTE2);
    /* print the size of the frame (size + sequencer byte) */
    this->write_bytes(size + 1);
    /* print the sequencer */
    this->write_bytes(this->sequencer);
    /* print the frame */
    this->write_bytes(frame, size);
    /* print the crc (footer) */
    this->write_bytes((char*)&crc, sizeof(crc));
    this->flush_bytes();
    /* update the sequencer */
    this->sequencer++;
    return 0;
}

/**
 * @see MvCom::write_frame
 */
int GenMvCom::write_frame(char *frame, int size)
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
int GenMvCom::set_mode(enum mvCom_mode mode)
{
    this->mode = mode;
    this->state = GMC_EMPTY; 
    return 0;
}

/**
 * @see MvCom::get_mode
 */
enum mvCom_mode GenMvCom::get_mode(void)
{
    return this->mode;
}

/**
 * GenMvCom
 *
 * @brief initialize a structure to be used as a MvCom with a com port
 */
GenMvCom::GenMvCom(void)
{
    this->frame_size = 0;
    this->mode = MVCOM_ASCII;
    this->total_frame_size = 0;
    this->state = GMC_EMPTY;
    this->sequencer = 0;
}
