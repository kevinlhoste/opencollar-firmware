// This includes are needed because otherwise the Arduino cannot find the headers
// for internal use of the libs
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#include "SPI.h"

#include "MvCore.h"
#include "SerialMvCom.h"
#include "Storage.h"

#define BUTTON_PIN 0
#define WRITE_LED 13

MvCore g_core;

void setup()
{
    int i;
    MvCom *com[1];

    Serial1.begin(9600);

    com[0] = new SerialMvCom(&Serial1);

    MvFrameHandler *fhandler = new MvFrameHandler(com, 1);

    /* Initialize the storage if it is not yet initialized */
    Storage *storage = new Storage();
    if(storage->status() < 0)
        storage->reset();

    /* Initialize the core app */
    g_core.setup(storage, fhandler, MPU6050_ADDRESS_AD0_LOW, BUTTON_PIN, WRITE_LED);
}

void loop()
{
    g_core.loop();
}