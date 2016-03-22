// This includes are needed because otherwise the Arduino cannot find the headers
// for internal use of the libs
#include "I2Cdev.h"
#include "MPU6050.h"
#include "MS5611.h"
#include "Wire.h"

#include "GenMvCom.h"
#include "MvCore.h"
#include "SerialMvCom.h"
#include "at45db161d.h"
#include "Storage.h"

#define BUTTON_PIN 11
#define WRITE_LED 13
#define VIBRATE_PIN 13

MvCore g_core;

void setup()
{
    int i;
    MvCom *com[1];

    Serial1.begin(115200);

    com[0] = new SerialMvCom(&Serial1);

    MvFrameHandler *fhandler = new MvFrameHandler(com, 1);

    Storage *storage = new Storage();

    /* Initialize the core app */
    g_core.setup(storage, fhandler, MPU6050_ADDRESS_AD0_HIGH, BUTTON_PIN, VIBRATE_PIN);
    g_core.setupLed(WRITE_LED, LOW);
}

void loop()
{
    g_core.loop();
}
