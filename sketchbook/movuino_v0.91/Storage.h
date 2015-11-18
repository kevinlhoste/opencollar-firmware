#ifndef STORAGE_H
#define STORAGE_H

#include "MvStorage.h"
#include "DataFlash.h"

#define INIT_KEY 0xdead
#define PAGE_SIZE 528
#define TOTAL_PAGES 100

/**
 * Storage
 *
 * @brief Control the persistent memory and the data that should be stored in it.
 * It also can record one stream of usage by the user, that is why it implements
 * MvCom, the record function works like a live function, but the live package are
 * stored in the persistent memory.
 */
class Storage : public MvStorage
{
    public:
        Storage(void);
        /* McCom methods */
        int write_frame(char *frame, int size);
        int read_frame(char *frame, int *size);
        int set_mode(enum mvCom_mode mode);
        enum mvCom_mode get_mode(void);
        /* Storage methods */
        int status(void);
        int reset(void);
        int set_cfg(enum cfg_id id, uint8_t value);
        uint8_t get_cfg(enum cfg_id id);
        void rewind(void);
        void clear_recordings(void);
    private:
        /** Data that is in the persistent memory */
        struct storage_data data;
        /** Controls the persistent memoryy */
        DataFlash flash;
        /** Used at read and write of a record, maps the current memory position */
        uint16_t page;
        /** Used at read and write of a record, maps the current memory position */
        uint16_t offset;
        int write_storage_data(void);
        int read_storage_data(void);
        void soft_reset(void);
};

#endif //MVSTORAGE_H
