#ifndef GENMVCOM_H
#define GENMVCOM_H

#include "MvCom.h"
#include "definitions.h"
#include "Arduino.h"

#define GMC_SYNC_BYTE1 0x55
#define GMC_SYNC_BYTE2 0x36

/**
 * GenMvCom
 *
 * @brief Implements the abstract class MvCom for the com ports
 */
class GenMvCom : public MvCom
{
    public:
        GenMvCom(void);
        int write_frame(char *frame, int size);
        int read_frame(char *frame, int *size);
        int set_mode(enum mvCom_mode mode);
        enum mvCom_mode get_mode(void);
        void update(void);
    private:
        /** Bytes that comes from a com port and might go to the buffer */
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
            GMC_EMPTY,
            GMC_SYNC1,
            GMC_SYNC2,
            GMC_INCOMING,
            GMC_READY
        } state;
        void _update_byte(void);
        void _update_ascii(void);
        void _read_frame_ascii(char *frame, int *size);
        void _read_frame_byte(char *frame, int *size);
        int _write_frame_ascii(char *frame, int size);
        int _write_frame_byte(char *frame, int size);

        /** Functions specific to the communication port */
        virtual char read_byte(void) = 0;
        virtual void write_bytes(char *string) = 0;
        virtual void write_bytes(char c) = 0;
        virtual void write_bytes(char *bytes, int size) = 0;
        virtual void write_bytes(int n) = 0;
        virtual bool available_byte(void) = 0;
        virtual void flush_bytes(void) = 0;
};

#endif //GENMVCOM_H
