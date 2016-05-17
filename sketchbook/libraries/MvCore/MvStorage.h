#ifndef MVSTORAGE_H
#define MVSTORAGE_H

#include "frame_struct.h"
#include "MvCom.h"

/**
 * storage_data
 *
 * @brief Contain all the data that must be stored in a persistent memory.
 */
struct storage_data
{
    /** Used to verify if the memory has been initialized or if it is usable */
    uint16_t init_key;
    uint8_t value[CFG_ID_LIST_SIZE];
};

/**
 * MvStorage
 *
 * @brief Control the persistent memory and the data that should be stored in it.
 * It also can record one stream of usage by the user, that is why it implements
 * MvCom, the record function works like a live function, but the live package are
 * stored in the persistent memory.
 */
class MvStorage : public MvCom
{
    public:
        /* MvCom methods */
        virtual int write_frame(char *frame, int size) = 0;
        virtual int read_frame(char *frame, int *size) = 0;
        virtual int set_mode(enum mvCom_mode mode) = 0;
        virtual enum mvCom_mode get_mode(void) = 0;
        /* Storage methods */
        virtual int status(void) = 0;
        virtual int reset(void) = 0;
        virtual int set_cfg(enum cfg_id id, uint8_t value) = 0;
        virtual uint8_t get_cfg(enum cfg_id id) = 0;
        virtual void rewind(void) = 0;
        virtual void clear_recordings(void) = 0;
        virtual void sleep(void) = 0;
};

#endif //MVSTORAGE_H
