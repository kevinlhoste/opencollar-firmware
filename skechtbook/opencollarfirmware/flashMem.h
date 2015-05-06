#ifndef FLASHMEM_H
#define FLASHMEM_H

/*
* The first page of the memory will be used to store meta data
*/

#include<string.h>

#define CHAR_MODE 0
#define BYTE_MODE 1

char itoa_buff[20];

void 
divide_int_print(int16_t value)
{
  serial_print_char(value&255);
  //serial_print_char(' ');
  serial_print_char((value >> 8)&255);
  //serial_print_char(' ');
}

void
print_accelgyro(int mode)
{
    if(mode == CHAR_MODE)
    {
        serial_print_str(itoa(accelgyro.ax,itoa_buff,10));
        serial_print_char(' ');
        serial_print_str(itoa(accelgyro.ay,itoa_buff,10));
        serial_print_char(' ');
        serial_print_str(itoa(accelgyro.az,itoa_buff,10));
        serial_print_char(' ');
        serial_print_str(itoa(accelgyro.gx,itoa_buff,10));
        serial_print_char(' ');
        serial_print_str(itoa(accelgyro.gy,itoa_buff,10));
        serial_print_char(' ');
        serial_println_str(itoa(accelgyro.gz,itoa_buff,10));
    }
    else if(mode == BYTE_MODE)
    {
        divide_int_print(accelgyro.ax);
        divide_int_print(accelgyro.ay);
        divide_int_print(accelgyro.az);
        divide_int_print(accelgyro.gx);
        divide_int_print(accelgyro.gy);
        divide_int_print(accelgyro.gz);
        serial_print_char('\n');
    }
}

#define FLASH_SYNC 2765

struct FlashMem{
    DataFlash dataflash;
    int16_t page;
    int16_t offset;
    int16_t n;
    int16_t buffered_bytes;
    int16_t sampling;
    char gyro_scale;
    char acce_scale;
} flashMem;

#define FLASH_READ_INT16(value)         \
{                                       \
    value = 0;                          \
    value = SPI.transfer(0xff);         \
    value = value << 8;                 \
    value += SPI.transfer(0xff);        \
}

#define FLASH_WRITE_INT16(value)        \
{                                       \
    SPI.transfer((value >> 8) & 0xff);  \
    SPI.transfer(value & 0xff);         \
}

void
flash_write_meta_data()
{
    flashMem.dataflash.pageToBuffer(0,0);
    flashMem.dataflash.bufferWrite(0,0);
    FLASH_WRITE_INT16(FLASH_SYNC);
    FLASH_WRITE_INT16(flashMem.n);
    FLASH_WRITE_INT16(flashMem.sampling)
    SPI.transfer(flashMem.acce_scale);
    SPI.transfer(flashMem.gyro_scale);
    flashMem.dataflash.bufferToPage(0,0);
}

void
flash_write_config(char acce_scale, char gyro_scale, int sampling)
{
    flashMem.dataflash.pageToBuffer(0,0);
    flashMem.dataflash.bufferWrite(0,8);
    FLASH_WRITE_INT16(FLASH_SYNC);
    FLASH_WRITE_INT16(sampling)
    SPI.transfer(acce_scale);
    SPI.transfer(gyro_scale);
    flashMem.dataflash.bufferToPage(0,0);
}

int
flash_read_meta_data(void)
{
    int16_t synch_value;
    flashMem.dataflash.pageToBuffer(0,0);
    flashMem.dataflash.bufferRead(0,0);

    FLASH_READ_INT16(synch_value);
    FLASH_READ_INT16(flashMem.n);
    FLASH_READ_INT16(flashMem.sampling);
    flashMem.acce_scale = SPI.transfer(0xff);
    flashMem.gyro_scale = SPI.transfer(0xff);
    
    if(synch_value != FLASH_SYNC)
        return 1;
    else return 0;
}

int
flash_read_config(char *acce_scale, char *gyro_scale, int *sampling)
{
    int16_t synch_value;
    flashMem.dataflash.pageToBuffer(0,0);
    flashMem.dataflash.bufferRead(0,8);
    FLASH_READ_INT16(synch_value);
    FLASH_READ_INT16(*sampling)
    *acce_scale = SPI.transfer(0xff);
    *gyro_scale = SPI.transfer(0xff);
    flashMem.dataflash.bufferToPage(0,0);
    if(synch_value != FLASH_SYNC)
        return 1;
    else return 0;
}

int
flash_setup(void)
{
    int16_t synch_value; 
    //Start SPI for the external flash
    SPI.begin();
    flashMem.dataflash.setup(5,6,7);
    delay(10);
    flashMem.dataflash.autoErase();

    if(flash_read_meta_data())
    {
        Serial.println("M problem to read memory");
        flash_write_meta_data();
        return 1;
    }
    return 0;
}

void
flash_erase(void)
{
    flashMem.page = 1;
    flashMem.offset = 0;
    flashMem.n = 0;
    flash_write_meta_data();
}

void
flash_write_mode_start(void)
{
    flashMem.sampling = accelgyro.sampling_rate;
    flashMem.acce_scale = accelgyro.acc_range;
    flashMem.gyro_scale = accelgyro.gyro_range;
    flash_erase();
    flashMem.dataflash.bufferWrite(0,0);
    flashMem.offset = 0;
}

void
flash_write_buffer(void)
{
    flashMem.dataflash.bufferToPage(0,flashMem.page);
    flashMem.page++;
    flashMem.n += (flashMem.offset/12);
    flashMem.offset = 0;
    flash_write_meta_data();
    flashMem.dataflash.bufferWrite(0,0);
}

void
flash_write_accelgyro(void)
{
    //print_accelgyro(0);
    FLASH_WRITE_INT16(accelgyro.ax);
    FLASH_WRITE_INT16(accelgyro.ay);
    FLASH_WRITE_INT16(accelgyro.az);
    FLASH_WRITE_INT16(accelgyro.gx);
    FLASH_WRITE_INT16(accelgyro.gy);
    FLASH_WRITE_INT16(accelgyro.gz);
    flashMem.offset += 12;
    if(flashMem.offset == 528) flash_write_buffer(); 
}

void
read_accelgyro(void)
{
    FLASH_READ_INT16(accelgyro.ax);
    FLASH_READ_INT16(accelgyro.ay);
    FLASH_READ_INT16(accelgyro.az);
    FLASH_READ_INT16(accelgyro.gx);
    FLASH_READ_INT16(accelgyro.gy);
    FLASH_READ_INT16(accelgyro.gz);
}

void
flash_read_accelgyro(int mode)
{
    if(flash_read_meta_data())
        { Serial.println("M corrupted memory"); return; }
    serial_print_char('i');
    serial_print_char(' ');
    serial_print_int((int)flashMem.acce_scale);
    serial_print_char(' ');
    serial_print_int((int)flashMem.gyro_scale);
    serial_print_char(' ');
    serial_println_int(flashMem.sampling);
    int i, j;
    flashMem.page = (flashMem.n*12)/528;

    //start from page 1 because page 0 is used to store variables
    for(i = 1; i <= flashMem.page; i++)
    {
        flashMem.dataflash.pageToBuffer(i,0);
        flashMem.dataflash.bufferRead(0,0);
        for(j = 0; j < 528; j+=12)
        {
            read_accelgyro();
            if(mode == CHAR_MODE)
              serial_print_str("r ");
            else
              serial_print_str("R ");
            print_accelgyro(mode);
            if(used_serial)delay(30);
        }
    }
    flashMem.dataflash.pageToBuffer(i,0);
    flashMem.dataflash.bufferRead(0,0);
    j = (flashMem.n*12)%528;
    for(i = 1; i < j; i+=12)
    {
        if(mode == CHAR_MODE)
          serial_print_str("r ");
        else
          serial_print_str("R ");
        read_accelgyro();
        print_accelgyro(mode);
        delay(10);
    }
}

#endif
