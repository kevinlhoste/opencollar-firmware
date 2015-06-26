#ifndef MVSTORAGE_H
#define MVSTORAGE_H

#include <SPI.h>
#include "DataFlash.h"
#include "frame_struct.h"
#include "MvCom.h"

#define INIT_KEY 0x7777
#define PAGE_SIZE 528
#define TOTAL_PAGES 100

/**
 * storage_data
 *
 * @brief Contain all the data that must be stored in a persistent memory.
 */
struct storage_data
{
    /** Used to verify if the memory has been initialized or if it is usable */
    uint16_t init_key;
    uint8_t
        live_acc,
        live_gyro,
        live_quat,
        live_euler,
        live_gravity,
        acc_sens,
        gyro_sens, 
        sampling_rate;
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
        MvStorage(void);
        /* McCom methods */
        int write_frame(char *frame, int size);
        int read_frame(char *frame, int *size);
        int set_mode(enum mvCom_mode mode);
        enum mvCom_mode get_mode(void);
        /* Storage methods */
        int status(void);
        int reset(void);
        int set_live_acc(uint8_t value);
        int set_live_gyro(uint8_t value);
        int set_live_quat(uint8_t value);
        int set_live_euler(uint8_t value);
        int set_live_gravity(uint8_t value);
        int set_acc_sens(uint8_t value);
        int set_gyro_sens(uint8_t value);
        int set_sampling_rate(uint8_t value);
        uint8_t get_live_acc(void);
        uint8_t get_live_gyro(void);
        uint8_t get_live_quat(void);
        uint8_t get_live_euler(void);
        uint8_t get_live_gravity(void);
        uint8_t get_acc_sens(void);
        uint8_t get_gyro_sens(void);
        uint8_t get_sampling_rate(void);
        void rewind(void);
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
