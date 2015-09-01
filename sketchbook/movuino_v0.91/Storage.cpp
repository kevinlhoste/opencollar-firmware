#include "Storage.h"

/**
 * read_storage_data
 *
 * @brief read all the data from the persistent memory
 */
int Storage::read_storage_data(void)
{
    int i;

    this->flash.autoErase();
    this->flash.pageToBuffer(0,0);
    this->flash.bufferRead(0,0);
    for (i = 0; i < sizeof(this->data); i++)
    {
        (((char*)(&(this->data)))[i]) = SPI.transfer(0xff);
    }

    return 0;
}

/**
 * Storage
 *
 * @brief Initialize the object
 * This constructor can fail to initialize, the user must
 * use the status method in order to verify if it was initialized
 * or not. In the case when it wasn't, the user should do a reset
 */
Storage::Storage(void)
{
    /* start the SPI for the external flash */
    SPI.begin();
    this->flash.setup(5,6,7);
    delay(10);
    /* read data from the memory */
    this->read_storage_data();
    /* initialize variables */
    this->page = 1;
    this->offset = 0;
}

/**
 * status
 *
 * @brief inform if the object is working or not
 * return 0 when it is and -1 when it is not
 */
int Storage::status(void)
{
    if (this->data.init_key == INIT_KEY)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * soft_reset
 *
 * @brief reset the values of all variables but doesn't write it at the
 * persistent memory
 */
void Storage::soft_reset(void)
{
    unsigned int i;

    for (i = 0; i < CFG_ID_LIST_SIZE; i++)
    {
        this->data.value[i] = cfg_id_list[i].default_val;
    }
}

/**
 * write_storage_data
 *
 * @brief write all the data to the persistent memory.
 */
int Storage::write_storage_data(void)
{
    int i;

    this->flash.pageToBuffer(0,0);
    this->flash.bufferWrite(0,0);
    for (i = 0; i < sizeof(this->data); i++)
    {
        SPI.transfer(((char*)(&(this->data)))[i]);
    }
    this->flash.bufferToPage(0,0);

    return 0;
}

/**
 * clear_recordings
 *
 * @brief clear recorded data
 */
void Storage::clear_recordings(void)
{
    /* write the record part to say it is empty */
    this->flash.pageToBuffer(1,0);
    this->flash.bufferWrite(0,0);
    SPI.transfer(0);
    this->flash.bufferToPage(0,1);
}

/**
 * reset
 *
 * @brief reset all the variable values and tries to write it in the persistent memory.
 * Return 0 when succeded and -1 when the persistent memory is unreachable
 */
int Storage::reset(void)
{
    /* reset variables */
    this->data.init_key = INIT_KEY;
    this->soft_reset();
    /* write in memory */
    this->write_storage_data();
    /* clear the recorded data space */
    this->clear_recordings();
    /* read the data that was writen */
    this->data.init_key = 0;
    this->read_storage_data();
    /* verify if it was done properly */
    if(this->status() < 0)
    {
        /* if failed to write, reset the variables in ram and return error 
         * obs: the object can still be used, but will fail to access the persistent 
         * memory */
        this->soft_reset();
        return -1;
    }

    return 0;
}

int Storage::set_cfg(enum cfg_id id, uint8_t value)
{
    int index = cfg_id_get_index(id); 
    if (index < 0)
        return index;
    this->data.value[index] = value;
    return this->write_storage_data();
}

uint8_t Storage::get_cfg(enum cfg_id id)
{
    int index = cfg_id_get_index(id); 
    if (index < 0)
        return index;
    return this->data.value[index];
}

/**
 * rewind
 *
 * @brief rewinds the external memory address for the movement recording
 * must be called before starting to write or read a recording.
 * */
void Storage::rewind(void)
{
    this->page = 1;
    this->offset = 0;
}

/**
 * set_mode
 *
 * @brief implement interface from MvCom
 * Fails whenever the mode is different from binary
 */
int Storage::set_mode(enum mvCom_mode mode)
{
    if(mode != MVCOM_BINARY)
    {
        return -1;
    }
    return 0;
}

/**
 * get_mode
 *
 * @brief implement interface from MvCom
 * Always returns MVCOM_BINARY
 */
enum mvCom_mode Storage::get_mode(void)
{
    return MVCOM_BINARY;
}

/**
 * write_frame
 *
 * @brief implement interface from MvCom
 * write a frame at the persistent memory
 *
 * This function assumes a frame will never be bigger than a PAGE_SIZE
 * (it can be divided in a maximum of 2 pages)
 */
int Storage::write_frame(char *frame, int size)
{
    int 
        i,
        first_part;

    /* check if frame fits in memory */
    if (((size + 2 + this->offset) >= PAGE_SIZE) && (this->page == (TOTAL_PAGES - 1)))
    {
        /* return error if it doesn't */
        return -1;
    }

    /* prepare memory to write */
    this->flash.pageToBuffer(this->page,0);
    this->flash.bufferWrite(0,this->offset);
    /* write the size of the frame */
    SPI.transfer(size & 0xff);
    /* copy to memory until the frame end or until reach the end of the page */
    for (i = 0; ((i + this->offset + 1) < PAGE_SIZE) && (i < size); i++)
    {
        SPI.transfer(frame[i]);
    }
    /* if reached the end of the page */
    if((i + this->offset + 1) == PAGE_SIZE)
    {
        /* write the page to memory */
        this->flash.bufferToPage(0,this->page);
        this->page++;
        /* load the new page */
        this->flash.pageToBuffer(this->page,0);
        this->flash.bufferWrite(0,0);
    }
    /* write the rest of the frame */
    for (; i < size; i++)
    {
        SPI.transfer(frame[i]);
    }
    /* write a frame with size 0 (indicates the last frame) */
    SPI.transfer(0);
    /* write page to memory */
    this->flash.bufferToPage(0, this->page);
    /* update offset */
    this->offset = (this->offset + size + 1) % PAGE_SIZE;

    return 0;
}

/**
 * read_frame
 *
 * @brief implements function from MvCom
 * read a frame from memory and offset the internal pointer to the next frame
 */
int Storage::read_frame(char *frame, int *size)
{
    unsigned char read_size;
    int i;

    /* load page to buffer */
    this->flash.pageToBuffer(this->page, 0);
    this->flash.bufferRead(0, this->offset);
    /* read the size of the frame */
    read_size = SPI.transfer(0xff);
    *size = read_size;
    /* if size == 0 return error */
    if(!read_size)
    {
        return -1;
    }
    /* read frame while still in the same page */
    for(i = 0; (i < read_size) && ((i + this->offset + 1) < PAGE_SIZE); i++)
    {
        frame[i] = SPI.transfer(0xff);
    }
    /* if reached the end of the page */
    if((i + this->offset + 1) == PAGE_SIZE)
    {
        /* load next page */
        this->page++;
        this->flash.pageToBuffer(this->page, 0);
        this->flash.bufferRead(0,0);
        /* read the rest of the frame */
        for(; i < read_size; i++)
        {
            frame[i] = SPI.transfer(0xff);
        }
    }
    /* update the offset */
    this->offset = (this->offset + read_size + 1) % PAGE_SIZE;
    /* if frame ended in the exact end of a page update page */
    if(!this->offset)
    {
        this->page++;
    }

    return 0;
}

