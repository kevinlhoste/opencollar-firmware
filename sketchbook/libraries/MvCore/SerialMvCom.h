#ifndef SERIALMVCOM_H
#define SERIALMVCOM_H

#include "GenMvCom.h"

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
 * @brief Implements the abstract class GenMvCom for the serial ports
 */
class SerialMvCom : public GenMvCom
{
    public:
        SerialMvCom(SERIAL_CLASS *serial);

    private:
        SERIAL_CLASS *serial;
        char read_byte(void);
        void write_bytes(char *string);
        void write_bytes(char c);
        void write_bytes(char *bytes, int size);
        void write_bytes(int n);
        bool available_byte(void);
        void flush_bytes(void);
};

#endif //SERIALMVCOM_H
