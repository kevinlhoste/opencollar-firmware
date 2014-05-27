#ifndef SERIAL_FUNCTIONS_H
#define SERIAL_FUNCTIONS_H

//#define SERIAL_AVAILABLE (Serial.available() || Serial1.available())


char used_serial = 0;

int serial_available()
{
  if(Serial.available()) { used_serial = 0; return 1; }
  if(Serial1.available()) { used_serial = 1; return 1; }
  if(used_serial == 1)
  {
    delayMicroseconds(10000);
  }
}

char 
serial_read(void)
{
  char ret;
  if(used_serial == 0) { return Serial.read(); }
  ret = Serial1.read();
  if(ret == -1) { used_serial = 0; return 0;}
  return ret;
}

void
serial_println_str(char *value)
{
  if(used_serial) Serial1.println(value);
  else Serial.println(value);
}

void
serial_print_int(long int value)
{
  if(used_serial) Serial1.print(value);
  else Serial.print(value);
}

void
serial_println_int(long int value)
{
  if(used_serial) Serial1.println(value);
  else Serial.println(value);
}

void
serial_print_str(char *value)
{
  if(used_serial) Serial1.print(value);
  else Serial.print(value);
}

void
serial_println_char(char value)
{
  if(used_serial) Serial1.println(value);
  else Serial.println(value);
}

void
serial_print_char(char value)
{
  if(used_serial) Serial1.print(value);
  else Serial.print(value);
}

#endif
