void
setup(void)
{
  Serial.begin(38400);
  Serial1.begin(9600);
}

void
loop(void)
{
  char buffer;
  //if(Serial1.available()) Serial.println("BLE available");
  //if(Serial.available()) Serial.println("Serial available");
  if(Serial.available() > 0) { Serial1.print((char)Serial.read()); }
  if(Serial1.available() > 0) { Serial.print((char)Serial1.read()); }
}
