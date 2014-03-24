#ifndef FLASHMEM_H
#define FLASHMEM_H

/*
* The first page of the memory will be used to store meta data
*/


#define CHAR_MODE 0
#define BYTE_MODE 1

void
print_accelgyro(int mode)
{
    if(mode == CHAR_MODE)
    {
        Serial.print(accelgyro.ax);
        Serial.print(' ');
        Serial.print(accelgyro.ay);
        Serial.print(' ');
        Serial.print(accelgyro.az);
        Serial.print(' ');
        Serial.print(accelgyro.gx);
        Serial.print(' ');
        Serial.print(accelgyro.gy);
        Serial.print(' ');
        Serial.println(accelgyro.gz);
    }
    else if(mode == BYTE_MODE)
    {
        /* TODO */
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
flash_write_meta_data(void)
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

void
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
    }
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
    Serial.print('i');
    Serial.print(' ');
    Serial.print((int)flashMem.acce_scale);
    Serial.print(' ');
    Serial.print((int)flashMem.gyro_scale);
    Serial.print(' ');
    Serial.println(flashMem.sampling);
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
            Serial.print("r ");
            print_accelgyro(mode);

        }
    }
    flashMem.dataflash.pageToBuffer(i,0);
    flashMem.dataflash.bufferRead(0,0);
    j = (flashMem.n*12)%528;
    for(i = 1; i < j; i+=12)
    {
        Serial.print("r ");
        read_accelgyro();
        print_accelgyro(mode);
    }
}

#endif
