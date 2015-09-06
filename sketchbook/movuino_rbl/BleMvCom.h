#ifndef BLEMVCOM_H
#define BLEMVCOM_H

#include "GenMvCom.h"

/**
 * BleMvCom
 *
 * @brief Implements the abstract class GenMvCom for the bluetooth
 */
class BleMvCom : public GenMvCom
{
    public:
        BleMvCom(void);

    private:
        char read_byte(void);
        void write_bytes(char *string);
        void write_bytes(char c);
        void write_bytes(char *bytes, int size);
        void write_bytes(int n);
        bool available_byte(void);
        void flush_bytes(void);
};

#endif //BLEMVCOM_H
