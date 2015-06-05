#include <SPI.h>
#include "DataFlash.h"
#include "MvStorage.h"
#include "MPU6050.h"
#include "I2Cdev.h"
#include "Wire.h"

MvStorage *storage;
char buff[100];
int fsize;

void setup() {
  // put your setup code here, to run once:
  storage = new MvStorage();
  storage->reset();
  Serial.begin(38400);
}

void loop() {
  // put your main code here, to run repeatedly:
  while(!Serial.available()){}
  Serial.print("get_live_acc ");
  Serial.println((int)storage->get_live_acc());
  Serial.print("status ");
  Serial.println((int)storage->status());
  storage->rewind();
  storage->write_frame("1234",4);
  storage->write_frame("5678",4);
  storage->rewind();
  while(storage->read_frame(buff,&fsize) >= 0)
  {
    Serial.println("stored frame:");
    Serial.write(buff,fsize);
    Serial.println("");
  }
  //Serial.print();
  //Serial.print();
  delay(3000);
}
