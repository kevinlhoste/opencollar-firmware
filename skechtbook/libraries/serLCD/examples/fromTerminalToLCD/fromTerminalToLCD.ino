/*
  serLCD - Example
  > Reading characters from the Terminal prompt.
  > Printing the characters on the LCD
  > ...
  J.A. Korten / 03 - 2013
 */
#include <SoftwareSerial.h>
#include <serLCD.h>

// Set pin to the LCD's rxPin
int pin = 2;

serLCD lcd(pin);

void setup (){
  Serial.begin(9600);
  Serial.flush();
  lcd.clear();
  //digitalWrite (13, HIGH);      //turn on debugging LED
}

void loop (){

  int i=0;
  char commandbuffer[100];

  if(Serial.available()){
     delay(100);
     while( Serial.available() && i< 99) {
        commandbuffer[i++] = Serial.read();
     }
     commandbuffer[i-1]='\0'; // stripping end of line character
  }

  if(i>0) {
     lcd.clear();
     lcd.print((char*)commandbuffer);
  }

}
