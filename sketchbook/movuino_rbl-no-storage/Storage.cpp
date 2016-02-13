#include "Storage.h"

// TODO: implement a real permanet storage

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
    this->soft_reset();
}

/**
 * status
 *
 * @brief inform if the object is working or not
 * return 0 when it is and -1 when it is not
 */
int Storage::status(void)
{
    return -1;
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
 * clear_recordings
 *
 * @brief clear recorded data
 */
void Storage::clear_recordings(void)
{
    /* write the record part to say it is empty */
}

/**
 * reset
 *
 * @brief reset all the variable values and tries to write it in the persistent memory.
 * Return 0 when succeded and -1 when the persistent memory is unreachable
 */
int Storage::reset(void)
{
    this->soft_reset();
    return 0;
}

int Storage::set_cfg(enum cfg_id id, uint8_t value)
{
    int index = cfg_id_get_index(id);
    if (index < 0)
        return index;
    this->data.value[index] = value;
    return 0;
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
    return -1;
}

/**
 * read_frame
 *
 * @brief implements function from MvCom
 * read a frame from memory and offset the internal pointer to the next frame
 */
int Storage::read_frame(char *frame, int *size)
{
    return -1;
}

