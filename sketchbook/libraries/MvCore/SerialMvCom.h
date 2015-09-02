#ifndef SERIALMVCOM_H
#define SERIALMVCOM_H

#include "mv_crc.h"
#include "MvCom.h"
#include "definitions.h"
#include "Arduino.h"

#define SMC_SERIAL_OBJECTS 2
#define SMC_SYNC_BYTE1 0x55
#define SMC_SYNC_BYTE2 0x36


// TODO: Check if this macro is the best one to check the board we are compiling
#ifdef __NRF51822_H__
    #define SERIAL_CLASS UARTClass
    #define STR_CAST uint8_t*
#else
    #include "Stream.h"
    #define SERIAL_CLASS Stream
    #define STR_CAST char*
#endif

/**
 * SerialMvCom
 *
 * @brief Implements the abstract class MvCom for the serial ports
 */
class SerialMvCom : public MvCom
{
    public:
        SerialMvCom(SERIAL_CLASS *serial);
        int write_frame(char *frame, int size);
        int read_frame(char *frame, int *size);
        int set_mode(enum mvCom_mode mode);
        enum mvCom_mode get_mode(void);
        /** This variable contains all serial ports that are being used */
        static SerialMvCom *Smc_list[SMC_SERIAL_OBJECTS];
        /** Contains the amount of serial ports being used */
        static int Smc_list_size;
        void update(void);
    private:
        /** Bytes that comes from a serial port and might go to the buffer */
        char shift_byte;
        /** Store the received frame */
        char buffer[BUFFER_SIZE];
        /** Sequence number of the next frame to be sent */
        unsigned char sequencer;
        /** Store the curent mode of the object */
        enum mvCom_mode mode;
        /** Current size of the incoming frame */
        uint8_t frame_size;
        /** Total size of the incoming frame (when known before complete reception) */
        uint8_t total_frame_size;
        /** Keep the state of the object */
        enum {
            SMC_EMPTY,
            SMC_SYNC1,
            SMC_SYNC2,
            SMC_INCOMING,
            SMC_READY
        } state;
        /** Pointer to the serial port that will be used */
        SERIAL_CLASS *serial;
        void _update_byte(void);
        void _update_ascii(void);
        void _read_frame_ascii(char *frame, int *size);
        void _read_frame_byte(char *frame, int *size);
        int _write_frame_ascii(char *frame, int size);
        int _write_frame_byte(char *frame, int size);
};

#endif //SERIALMVCOM_H
