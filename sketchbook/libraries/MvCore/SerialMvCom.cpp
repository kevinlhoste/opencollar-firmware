#include "SerialMvCom.h"

/**
 * SerialMvCom
 *
 * @brief initialize a structure to be used as a MvCom with a serial port
 */
SerialMvCom::SerialMvCom(SERIAL_CLASS *serial) : GenMvCom()
{
    this->serial = serial;
}

char SerialMvCom::read_byte(void)
{
    return this->serial->read();
}

void SerialMvCom::write_bytes(char *string)
{
    this->serial->write(string);
}

void SerialMvCom::write_bytes(char c)
{
    this->serial->write(c);
}

void SerialMvCom::write_bytes(char *bytes, int size)
{
    this->serial->write((STR_CAST)bytes, size);
}

void SerialMvCom::write_bytes(int n)
{
    this->serial->print(n);
}

bool SerialMvCom::available_byte(void)
{
    return this->serial->available();
}

void SerialMvCom::flush_bytes(void)
{
    this->serial->flush();
}
