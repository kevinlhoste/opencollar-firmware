#include "Storage.h"
#include "frame_struct.h"

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
    /* TODO */
}

/**
 * status
 *
 * @brief inform if the object is working or not
 * return 0 when it is and -1 when it is not
 */
int Storage::status(void)
{
    /* TODO */
    return 0;
}

/**
 * clear_recordings
 *
 * @brief clear recorded data
 */
void Storage::clear_recordings(void)
{
    /* TODO */
}

/**
 * reset
 *
 * @brief reset all the variable values and tries to write it in the persistent memory.
 * Return 0 when succeded and -1 when the persistent memory is unreachable
 */
int Storage::reset(void)
{
    /* TODO */
    return 0;
}

int Storage::set_cfg(enum cfg_id id, uint8_t value)
{
    /* TODO */
    return 0;
}

uint8_t Storage::get_cfg(enum cfg_id id)
{
    /* TODO */
    if (id == CFG_ID_SAMPLING_RATE)
        return 2;
    if (id == CFG_ID_LIVE_MAG_RAW_EN)
        return 0;
    return 1;
}

/**
 * rewind
 *
 * @brief rewinds the external memory address for the movement recording
 * must be called before starting to write or read a recording.
 * */
void Storage::rewind(void)
{
    /* TODO */
}

/**
 * set_mode
 *
 * @brief implement interface from MvCom
 * Fails whenever the mode is different from binary
 */
int Storage::set_mode(enum mvCom_mode mode)
{
    /* TODO */
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
    /* TODO */
    return MVCOM_ASCII;
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
    /* TODO */
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
    /* TODO */
    return 0;
}
