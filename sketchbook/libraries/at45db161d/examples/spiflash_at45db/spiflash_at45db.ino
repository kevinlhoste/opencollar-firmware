/*
 * SPI FLASH AT45DB081D (Atmel)
 *   8Mbit
 */
#include "mbed.h"
#include "at45db161d.h"

#undef PAGE_SIZE
#define PAGE_SIZE 264 // AT45DB081D (1MB)
//#define PAGE_SIZE 528 // AT45DB321D (4MB)
//#define PAGE_NUM 4095 // AT45DB081D (1MB)
//#define PAGE_NUM 8192 // AT45DB321D (4MB)

#define WRITE_BUFFER 1
#define READ_BUFFER 2

//DigitalOut myled(LED1);
Serial pc(USBTX, USBRX);

SPI spi(P0_20, P0_22, P0_25); // mosi, miso, sclk
ATD45DB161D memory(spi, P0_3);

void flash_write (int addr, char *buf, int len) {
    int i;
    memory.BufferWrite(WRITE_BUFFER, addr % PAGE_SIZE);
    for (i = 0; i < len; i ++) {
        spi.write(buf[i]);
    }
    memory.BufferToPage(WRITE_BUFFER, addr / PAGE_SIZE, 1);
}

void flash_read (int addr, char *buf, int len) {
    int i;
    memory.PageToBuffer(addr / PAGE_SIZE, READ_BUFFER);
    memory.BufferRead(READ_BUFFER, addr % PAGE_SIZE, 1);
    for (i = 0; i < len; i ++) {
        buf[i] = spi.write(0xff);
    }
}

int main() {
    int i;
    char buf[PAGE_SIZE];
    Timer t;
    ATD45DB161D::ID id;

    pc.baud(9600);
    pc.printf("\r\nStart\r\n");
    spi.frequency(16000000);
    wait_ms(500);
    
    memory.ReadManufacturerAndDeviceID(&id);
    printf("RAM Manufacturer ID : %02x\r\n", id.manufacturer);
    printf("RAM Device ID : %02x %02x\r\n", id.device[0], id.device[1]);
    wait_ms(10);

    printf("\r\nHELLO test\r\n");
    
    printf("RAM write\r\n");
    strcpy(buf, "Hello!");
    flash_write(0, buf, 6);
    
    for (i = 0; i < PAGE_SIZE; i ++) {
        buf[i] = i;
    }
    flash_write(6, buf, PAGE_SIZE - 6);

    wait(1);
    memset(buf, 0, PAGE_SIZE);
    
    printf("RAM read\r\n");
    flash_read(0, buf, PAGE_SIZE);
    for (i = 0; i < PAGE_SIZE; i ++) {
        printf(" %02x", buf[i]);
        if ((i & 0x0f) == 0x0f)
            printf("\r\n");
    }

    wait(1);

    printf("\r\nWrite/Read time\r\n");

    printf("RAM write\r\n");
    t.reset();
    t.start();
    for (i = 0; i < 0x20000; i += PAGE_SIZE) {
        buf[0] = (i >> 8) & 0xff;
        flash_write(i, buf, PAGE_SIZE);
        if ((i & 0x0fff) == 0) printf(".");
    }
    t.stop();
    printf("\r\ntime %f, %f KBytes/sec\r\n", t.read(), (float)0x20000 / 1024 / t.read());

    wait(1);

    printf("RAM read\r\n");
    t.reset();
    t.start();
    for (i = 0; i < 0x20000; i += PAGE_SIZE) {
        flash_read(i, buf, PAGE_SIZE);
        if (buf[0] != ((i >> 8) & 0xff)) {
            printf("error %d\r\n", i);
            break;
        }
        if ((i & 0x0fff) == 0) printf(".");
    }
    t.stop();
    printf("\r\ntime %f, %f KBytes/sec\r\n", t.read(), 0x20000 / 1024 / t.read());

}
