// This includes are needed because otherwise the Arduino cannot find the headers
// for internal use of the libs
#include "I2Cdev.h"
#include "MPU6050.h"
#include "MS5611.h"
#include "Wire.h"

#include "GenMvCom.h"
#include "MvCore.h"
#include "SerialMvCom.h"
#include "BleMvCom.h"
#include "Storage.h"

#define BUTTON_PIN -1
#define WRITE_LED 13
#define VIBRATE_PIN 13

MvCore g_core;

void setup()
{
    int i;
    MvCom *com[2];

    Serial1.begin(9600);

    com[0] = new SerialMvCom(&Serial1);
    com[1] = new BleMvCom();

    MvFrameHandler *fhandler = new MvFrameHandler(com, 2);

    Storage *storage = new Storage();

    /* Initialize the core app */
    g_core.setup(storage, fhandler, MPU6050_ADDRESS_AD0_HIGH, BUTTON_PIN, WRITE_LED, VIBRATE_PIN);
}

void loop()
{
    g_core.loop();
}
