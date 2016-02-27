/*
 * SPI FLASH AT45DB081D (Atmel)
 *   8Mbit
 */
#include "mbed.h"
#include "at45db161d.h"

#undef PAGE_SIZE
#include "Storage.h"

#define BUFFER_1 1
#define BUFFER_2 2

#define BUFFER_READ(mem, off) mem.BufferRead(BUFFER_1,off,1)
#define BUFFER_WRITE(mem, off) mem.BufferWrite(BUFFER_1,off)
#define BUFFER_TO_PAGE(mem, page) mem.BufferToPage(BUFFER_1,page,1)
#define PAGE_TO_BUFFER(mem, page) mem.PageToBuffer(page,BUFFER_1)

static SPI spi(P0_20, P0_22, P0_25); // mosi, miso, sclk
static ATD45DB161D flash(spi, P0_3);


/**
 * read_storage_data
 *
 * @brief read all the data from the persistent memory
 */
int Storage::read_storage_data(void)
{
    int i;

    PAGE_TO_BUFFER(flash, 0);
    BUFFER_READ(flash, 0);
    for (i = 0; i < sizeof(this->data); i++)
    {
        (((char*)(&(this->data)))[i]) = spi.write(0xff);
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
    /* start the spi for the external flash */
    spi.frequency(16000000);
    wait_ms(50);
    /* read data from the memory */
    this->read_storage_data();
    /* Go to the end of the written flash */
    // TODO: is this a good solution?
    while(!read_frame(NULL, NULL));

}

/**
 * status
 *
 * @brief inform if the object is working or not
 * return 0 when it is and -1 when it is not
 */
int Storage::status(void)
{
    if (this->data.cfg.init_key == INIT_KEY)
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
        this->data.cfg.value[i] = cfg_id_list[i].default_val;
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

    PAGE_TO_BUFFER(flash, 0);
    BUFFER_WRITE(flash, 0);
    for (i = 0; i < sizeof(this->data); i++)
    {
        spi.write(((char*)(&(this->data)))[i]);
    }
    BUFFER_TO_PAGE(flash, 0);

    return 0;
}

/**
 * clear_recordings
 *
 * @brief clear recorded data
 */
void Storage::clear_recordings(void)
{
    /* Reset the page and offset */
    this->rewind();
    this->write_storage_data();
    /* write the record part to say it is empty */
    PAGE_TO_BUFFER(flash, 1);
    BUFFER_WRITE(flash,0);
    spi.write(0);
    BUFFER_TO_PAGE(flash, 1);
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
    this->data.cfg.init_key = INIT_KEY;
    this->soft_reset();
    /* write in memory */
    this->write_storage_data();
    /* clear the recorded data space */
    this->clear_recordings();
    /* read the data that was writen */
    this->data.cfg.init_key = 0;
    this->read_storage_data();
    /* verify if it was done properly */
    if(this->status() < 0)
    {
        /* if failed to write, reset the variables in ram and return error 
         * obs: the object can still be used, but will fail to access the persistent 
         * memory */
        Serial1.println("Fail to reset storage");
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
    this->data.cfg.value[index] = value;
    return this->write_storage_data();
}

uint8_t Storage::get_cfg(enum cfg_id id)
{
    int index = cfg_id_get_index(id); 
    if (index < 0)
        return index;
    return this->data.cfg.value[index];
}

/**
 * rewind
 *
 * @brief rewinds the external memory address for the movement recording
 * must be called before starting to write or read a recording.
 * */
void Storage::rewind(void)
{
    this->data.page = 1;
    this->data.offset = 0;
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
    bool page_changed = false;

    /* check if frame fits in memory */
    if (((size + 2 + this->data.offset) >= PAGE_SIZE) && (this->data.page == (TOTAL_PAGES - 1)))
    {
        /* return error if it doesn't */
        return -1;
    }

    /* prepare memory to write */
    PAGE_TO_BUFFER(flash, this->data.page);
    BUFFER_WRITE(flash, this->data.offset);
    /* write the size of the frame */
    spi.write(size & 0xff);
    /* copy to memory until the frame end or until reach the end of the page */
    for (i = 0; ((i + this->data.offset + 1) < PAGE_SIZE) && (i < size); i++)
    {
        spi.write(frame[i]);
    }
    /* if reached the end of the page */
    if((i + this->data.offset + 1) == PAGE_SIZE)
    {
        page_changed = true;
        /* write the page to memory */
        BUFFER_TO_PAGE(flash, this->data.page);
        this->data.page++;
        /* load the new page */
        PAGE_TO_BUFFER(flash, this->data.page);
        BUFFER_WRITE(flash, 0);
    }
    /* write the rest of the frame */
    for (; i < size; i++)
    {
        spi.write(frame[i]);
    }
    /* write a frame with size 0 (indicates the last frame) */
    spi.write(0);
    /* write page to memory */
    BUFFER_TO_PAGE(flash, this->data.page);
    /* update offset */
    this->data.offset = (this->data.offset + size + 1) % PAGE_SIZE;

    /* If the page has changed, save a checkpoint of the page and
     * offset, do this only in page multiples of 4 so we won't write
     * that often in the storage data location */
    if (page_changed && this->data.page % 4 == 0)
        this->write_storage_data();

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
    PAGE_TO_BUFFER(flash, this->data.page);
    BUFFER_READ(flash, this->data.offset);
    /* read the size of the frame */
    read_size = spi.write(0xff);
    if (size) *size = read_size;
    /* if size == 0 return error */
    if(!read_size)
    {
        return -1;
    }
    /* read frame while still in the same page */
    for(i = 0; (i < read_size) && ((i + this->data.offset + 1) < PAGE_SIZE); i++)
    {
        if (frame) // TODO: check this before the loop
            frame[i] = spi.write(0xff);
    }
    /* if reached the end of the page */
    if((i + this->data.offset + 1) == PAGE_SIZE)
    {
        /* load next page */
        this->data.page++;
        PAGE_TO_BUFFER(flash, this->data.page);
        BUFFER_READ(flash, 0);
        /* read the rest of the frame */
        for(; i < read_size; i++)
        {
            if (frame) // TODO: check this before the loop
                frame[i] = spi.write(0xff);
        }
    }
    /* update the offset */
    this->data.offset = (this->data.offset + read_size + 1) % PAGE_SIZE;

    return 0;
}

