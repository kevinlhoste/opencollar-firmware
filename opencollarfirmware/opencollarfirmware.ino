

/* ============================================
License infos
===============================================
*/
/*
gyro : 250Â°s
*/
/*
V0004 add serial
V0005 add gyro
*/


// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#include "Wire.h"
#include <SPI.h>
#include "DataFlash.h"
#include <EEPROM.h>

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"


// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;

int16_t ax, ay, az;
int16_t gx, gy, gz;

#define LED_PIN 13
bool blinkState = false;
char receivedChar=0;
int mode_op=0;
//flash
//general page sur 16 bits
uint16_t page   = 0;
DataFlash dataflash;
int buffercount=0;
uint8_t buffer = 0;
int currentPage=0;
int currentBlock1=0;
int currentBlock2=0;
int16_t myData[6];
unsigned long time0 =0;
unsigned long time1 =0;
/*autostart variables*/
unsigned long startupTime=0;
int autoStart=1;
int autoStart_mode=2;

int cycle_duration=10000; //default 10hz
int sampling_rate=100; //in hz
/* Acc range 
 * 0 = +/- 2g
 * 1 = +/- 4g
 * 2 = +/- 8g
 * 3 = +/- 16g
 */
uint8_t acc_range = 1; //+-2g
/* Gyro range 
 * 0 = +/- 250 degrees/sec
 * 1 = +/- 500 degrees/sec
 * 2 = +/- 1000 degrees/sec
 * 3 = +/- 2000 degrees/sec
 */
uint8_t gyro_range = 0; 

void setup() {
    // join I2C bus (I2Cdev library doesn't do this automatically)
    Wire.begin();
    SPI.begin();
    dataflash.setup(5,6,7);
    delay(10);
    dataflash.autoErase();
    // initialize serial communication
    // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
    // it's really up to you depending on your project)
    Serial.begin(38400);
    // initialize device
    //  Serial.println("Initializing I2C devices...");
    accelgyro.initialize();
    accelgyro.setFullScaleAccelRange(acc_range);
    accelgyro.setFullScaleGyroRange(gyro_range);
    // verify connection
    //  Serial.println("Testing device connections...");
    //   Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
    // configure Arduino LED for
    //pinMode(LED_PIN, OUTPUT);
    Serial.println("toto"); 
    startupDelay();
    //establishContact();
}

void loop() {
  
    checkSerial();  
    if(mode_op==1) {
      //initialize cycle duration
      sampling_rate=100;
      cycle_duration=1000000/sampling_rate;
      // read raw accel/gyro measurements from device
      accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
      // these methods (and a few others) are also available
      //accelgyro.getAcceleration(&ax, &ay, &az);
      //accelgyro.getRotation(&gx, &gy, &gz);
      // display tab-separated accel/gyro x/y/z values
      //Serial.print("a/g:\t");
      Serial.println(ax); //Serial.print("\t");
      Serial.println(ay); //Serial.print("\t");
      Serial.println(az); //Serial.print("\t");
      Serial.println(gx); //Serial.print("\t");
      Serial.println(gy); //Serial.print("\t");
      Serial.println(gz);
      time0=time1;
      time1=micros();
     // Serial.println(time1-time0);
      optimizeTime();
      //Serial.println(time1-time0);
      time1=micros();
      /*
      // blink LED to indicate activity
      blinkState = !blinkState;
      digitalWrite(LED_PIN, blinkState);*/
    }
    //save
     else if(mode_op==2) {
      uint8_t MSB;
      uint8_t LSB;
      // read raw accel/gyro measurements from device
      accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
      //dataflash.bufferWrite(buffer, 0);
      //put data in buffer
  //    Serial.println(ax);
      MSB=ax & 0xff;
      LSB=(ax >> 8);
      SPI.transfer(MSB);
      SPI.transfer(LSB);
  //    Serial.println(ay);
      MSB=ay & 0xff;
      LSB=(ay >> 8);
  //    Serial.println(az);
      SPI.transfer(MSB);
      SPI.transfer(LSB);
      MSB=az & 0xff;
      LSB=(az >> 8);
      SPI.transfer(MSB);
      SPI.transfer(LSB);
   //   Serial.println(gx);
      MSB=gx & 0xff;
      LSB=(gx >> 8);
   //   Serial.println(gy);
      SPI.transfer(MSB);
      SPI.transfer(LSB);
      MSB=gy & 0xff;
      LSB=(gy >> 8);
 //     Serial.println(gz);
      SPI.transfer(MSB);
      SPI.transfer(LSB);
      MSB=gz & 0xff;
      LSB=(gz >> 8);
      SPI.transfer(MSB);
      SPI.transfer(LSB);
      buffercount+=12;
     //if the buffer is full transfer buffer to page
     if(buffercount == 528) {
    //  Serial.println("transfer page");
      dataflash.bufferToPage(0, page);
    //  Serial.println(page);
      page ++;
      buffercount=0;
      buffer=0;
      dataflash.bufferWrite(buffer, 0);
      if(currentPage==8 && currentBlock1<=254){
        currentPage=0;
        currentBlock1++;
        EEPROM.write(0,currentBlock1);
        EEPROM.write(2, currentPage);
      /*  Serial.print("Block");
        Serial.print(currentBlock1);
        Serial.print("\n");*/
        } 
      else if(currentPage<8 && currentBlock1<=255){
        currentPage++;
        EEPROM.write(0,currentBlock1);
        EEPROM.write(2, currentPage);
        }
      else if(currentPage<8 && currentBlock1==255 && currentBlock2<=255){
        currentPage++;
        EEPROM.write(0,currentBlock1);
        EEPROM.write(2, currentPage);
        }  
      else if(currentPage=8 && currentBlock1==255 && currentBlock2<=255){
        currentPage=0;
        currentBlock2++;
       /* EEPROM.write(1,currentBlock2);
        EEPROM.write(2, currentPage);
        Serial.print("Block2");
        Serial.print(currentBlock2);
        Serial.print("\n");*/
        }   
    }
     time0=time1;
      time1=micros();
     // Serial.println(time1-time0);
      optimizeTime();
      //Serial.println(time1-time0);
      time1=micros();
     }
}
void test_write()
  {
    
  myData[0]=4388;
  myData[1]=-16498;
  myData[2]=-125;
  myData[3]=122;
  myData[4]=-298;
  myData[5]=-3256;
  myData[6]=4389;
  myData[7]=-16499;
  myData[8]=-126;
  myData[9]=123;
  myData[10]=-299;
  myData[11]=-3257;
  uint8_t MSB;
  uint8_t LSB;
  dataflash.bufferWrite(buffer, 0);
  for(int i=0;i<=11;i++)
    {
    MSB=myData[i] & 0xff;
    LSB=(myData[i] >> 8);
    SPI.transfer(MSB);
    SPI.transfer(LSB);
    buffercount+=2;
    }
  dataflash.bufferToPage(0, 0);
  }

void test_write2()
  {
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); 
  Serial.println(ax);
  myData[0]=ax;
  Serial.println(ay);
  myData[1]=ay;
  Serial.println(az);
  myData[2]=az;
  Serial.println(gx);
  myData[3]=gx;
  Serial.println(gy);
  myData[4]=gy;
  Serial.println(gz);
  myData[5]=gz;
  uint8_t MSB;
  uint8_t LSB;
  buffercount=0;
  dataflash.bufferWrite(buffer, 0);
 for(int j=0;j<=43;j++){
  for(int i=0;i<=5;i++)
    {
    MSB=myData[i] & 0xff;
    LSB=(myData[i] >> 8);
    SPI.transfer(MSB);
    SPI.transfer(LSB);
    buffercount+=2;
    Serial.println(buffercount);
    }
 } 
  if(buffercount == 528) {
      Serial.println("transfer page");
      dataflash.bufferToPage(0, page);
      Serial.println(page);
      page ++;
  }
  }  
  
void checkSerial(){
if (Serial.available() > 0) {
    // get incoming byte:
    receivedChar = Serial.read();
    //Live mode
    if(receivedChar=='l'){
      //Serial.println("Live mode");
      mode_op=1;
    }
    /*mode infos*/
      //Serial.println(acc_range);
      if(receivedChar=='i'){
      acc_range=accelgyro.getFullScaleAccelRange();
      gyro_range=accelgyro.getFullScaleGyroRange();
      Serial.println("Accelerometer range: ");
      Serial.print(acc_range);
      Serial.println("Gyro range: ");
      Serial.print(gyro_range);
      mode_op=0;
      }
    /*acknowledge of connection*/
    else if(receivedChar=='A'){
      Serial.println("A");
       mode_op=0;
      //mode_op=1;
    }
    else if(receivedChar=='w'){
      Serial.println("Write mode");
      //test_write2();
      EEPROM.write(511, 1);
      buffer=0;
      dataflash.bufferWrite(buffer, 0);
      mode_op=2;
    }
    //read
    else if(receivedChar=='r'){
      //Serial.println("Read");
      currentPage = EEPROM.read(2);
      currentBlock1=EEPROM.read(0);
     /* Serial.print("Block:");
      Serial.print(currentBlock1);
      Serial.print("\n");
      Serial.print("Page:");
      Serial.print(currentPage);
      Serial.print("\n");*/
      if(currentBlock1==0) page=currentPage;
      else page=(currentBlock1*8)+currentPage;
     /* Serial.print("Page total:");
      Serial.print(page);
      Serial.print("\n");*/
      //mode_op=3;
      uint8_t MSB;
      uint8_t LSB;
      int16_t value;
      uint16_t k=0;
      buffer=0;
      for(k=0;k<page;k++){
      dataflash.pageRead(k, 0); 
     /* Serial.print("*************************");
      Serial.println(k);*/
        for(int j=0;j<=263;j++) { 
          MSB = SPI.transfer(0xff);
          // Serial.println(toto0);
          LSB = SPI.transfer(0xff);
          //Serial.println(toto1);
          value=LSB;
          value <<= 8;
          value |=MSB;
          Serial.println(value);
         // delay(2);
          }
      }
    mode_op=0;
    startupDelay();
    }
    //erase
    else if(receivedChar=='e'){
      Serial.println("Erasing");
      EEPROM.write(0, 0);
      EEPROM.write(1, 0);
      EEPROM.write(2, 0);
      EEPROM.write(511, 0);
      mode_op=0;
    }
    //go back : q
    else if(receivedChar=='q'){
      Serial.println("Quit");
	//  establishContact();
      mode_op=0;
      startupDelay();
    }
}
}
void optimizeTime(){
  time1=micros();
  while ((time1-time0)<cycle_duration){
    time1=micros();
    }
}
void establishContact() {
  while (Serial.available() <= 0){
    Serial.write('A');   // send a capital A
    delay(300);
    }
}
void startupDelay() {
  startupTime=millis();
  while (startupTime <= 30000 && mode_op==0){
     startupTime=millis();
     Serial.println(startupTime); 
     // Serial.write('a'); 
    checkSerial();
    delay(300);
    }
  if(mode_op==0 && autoStart==1)
     {
      Serial.println("Write mode"); 
      EEPROM.write(0, 0);
      EEPROM.write(1, 0);
      EEPROM.write(2, 0);
      EEPROM.write(511, 1);
      buffer=0;
      dataflash.bufferWrite(buffer, 0);
      mode_op=2; 
     }
}
