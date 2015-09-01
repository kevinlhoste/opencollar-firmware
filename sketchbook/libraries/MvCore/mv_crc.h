#ifndef MV_CRC_H
#define MV_CRC_H

#include "stdint.h"

static inline uint16_t _mv_crc_byte(uint16_t crc, unsigned char byte)
{
    unsigned char aux_byte;
    uint16_t aux_crc;

    aux_byte = (crc >> 8) ^ byte;
    aux_byte ^= (aux_byte >> 4);
    aux_crc = aux_byte;

    return ((crc << 8) ^ (aux_crc << 12) ^ (aux_crc << 5) ^ (aux_crc));
}

static inline uint16_t mv_crc(uint16_t crc, unsigned char *data, int size)
{
    int i;
    for(i = 0; i < size; i++)
    {
        crc = _mv_crc_byte(crc, data[i]);
    }
}

#endif
