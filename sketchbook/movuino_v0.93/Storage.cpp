#include "Storage.h"
#include <SerialFlash.h>

#define CS 5 // digital pin for flash chip CS pin

/**
 * read_storage_data
 *
 * @brief read all the data from the persistent memory
 */
int Storage::read_storage_data(void)
{
    int i;
    SerialFlash.read(0, (char*)(&(this->data)), sizeof(this->data));
    while (!SerialFlash.ready());
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
    uint8_t id[3];

    /* setup flash */
    SerialFlash.begin(CS);
    SerialFlash.readID(id);
    // TODO: the blockSize returns 64k, but it seems the chip is
    // configured with an erase block of 256k
    this->page_size = 262144; //256k
    //this->page_size = SerialFlash.blockSize();
    this->capacity= SerialFlash.capacity(id);

    /* read data from the memory */
    this->read_storage_data();
    /* Go to the end of the written flash */
    // TODO: is this a good solution?
    if(!status())
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
    SerialFlash.eraseBlock(0);
    while (!SerialFlash.ready());

    SerialFlash.write(0, (char*)(&(this->data)), sizeof(this->data));
    while (!SerialFlash.ready());
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
    /* write the record part to say it is empty
     * the lengh is xor'ed with 0xFF, thus the
     * first frame will be read as size zero */
    SerialFlash.eraseBlock(this->page_size);
    while (!SerialFlash.ready());
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
        Serial.println("Fail to reset storage");
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
    /* Point to the second page (the first one is reserved for the cfgs) */
    this->data.addr = this->page_size;
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
    unsigned long wsize;
    bool page_changed = false;
    uint8_t tmp;

    /* check if frame fits in memory */
    if (size + 1 + this->data.addr >= this->capacity)
        return -1;

    /* write the size of the frame xor'ed with 0xFF */
    tmp = (size ^ 0xff) & 0xff;
    SerialFlash.write(this->data.addr++, &tmp, sizeof(tmp));

    /* write to memory until the frame end or until reach the end of the page */
    wsize = this->page_size - (this->data.addr % this->page_size);
    wsize = wsize < size ? wsize : size;

    /* write in the current page */
    if (wsize)
    {
        SerialFlash.write(this->data.addr, frame, wsize);
        this->data.addr += wsize;
    }

    /* We need to write in the next page */
    if (wsize != size)
    {
        page_changed = true;
        /* Erase the next page */
        SerialFlash.eraseBlock(this->data.addr);
        while (!SerialFlash.ready());
        /* Write in the next page */
        SerialFlash.write(this->data.addr, &frame[wsize], size - wsize);
    }

    /* If the page has changed, save a checkpoint of the page and
     * offset, do this only in page multiples of 4 so we won't write
     * that often in the storage data location */
    if (page_changed && (this->data.addr/this->page_size) % 4 == 0)
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

    /* read the size of the frame */
    SerialFlash.read(this->data.addr, &read_size, sizeof(read_size));
    read_size = read_size ^ 0xff; // The size is stored xor'ed with 0xff
    if (size) *size = read_size;
    /* if size == 0 return error */
    if(!read_size)
    {
        return -1;
    }

    if (frame) SerialFlash.read(++this->data.addr, frame, read_size);
    this->data.addr += read_size;

    return 0;
}

