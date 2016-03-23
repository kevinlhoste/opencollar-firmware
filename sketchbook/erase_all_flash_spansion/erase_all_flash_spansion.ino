#include <SerialFlash.h>
#include <SPI.h>

//Special bytes in the communication protocol
#define CSPIN   5
#define LED     12

void setup(){
    Serial.begin(38400);
    pinMode(LED, OUTPUT);
    SerialFlash.begin(CSPIN);

    // Erase all
    delay(500);
    Serial.println("Erasing...");
    SerialFlash.eraseAll();

    //Flash LED at 1Hz while formatting
    while (!SerialFlash.ready()) {
        delay(500);
        digitalWrite(LED, HIGH);
        Serial.println("Erasing.");
        delay(500);
        digitalWrite(LED, LOW);
        Serial.println("Erasing..");
    }

    Serial.println("DONE");

    //Quickly flash LED a few times when completed, then leave the light on solid
    for(uint8_t i = 0; i < 10; i++){
        delay(100);
        digitalWrite(LED, HIGH);
        delay(100);
        digitalWrite(LED, LOW);
    }
    digitalWrite(LED, HIGH);
}

void loop(){
    //Do nothing.
    delay(500);
    Serial.println("DONE");
}
