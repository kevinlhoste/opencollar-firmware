opencollar-firmware by Kevin Lhoste
===================

Firmware for the open collar

Commands list 
================

When connected on the usb port the arduino send a series of AAAAAAA for handshake -> the arduino is listening for commands.
Each command has to be followed by ENTER (\n)

l  : Live Mode - the raw data from sensors is send to the serial port in raw

w  : Write mode - the data is written on the flash chip

r  : Read mode - the data is read from the serial chip

q  : Quit - interrupt the precedent command




internal version reference : mpu6050V006flash (flash version)
