# opencollar-firmware by Kevin Lhoste

Firmware for the open collar

 ## Quick Start

1. Open the IDE, go to File->Preferences and inside the Sketchbook location, add the path to < project_dir >\opencollar-firmware\sketchbook
2. Reopen the IDE
3. Go to Tools->Board and select *SparkFun Pro Micro 3.3V/8Mhz*
4. Connect the Movuino with your PC and upload the movuino aplication under < project_dir >\opencollar-firmware\sketchbook\movuino
5. Go to Tools->Serial Monitor, select 38400 as the baudrate and Newline as the caracter to send when pressing Enter
6. See if it works:

Send ping:

    ?

You should receive:

    S: 0 F: A DESC: OK


-------------------------------------------------------
## Commands list 

There are two types of commands:

* Simple commands format: < cmd_id >
* Configuration commands format: c < cfg_id > < value >

### Simple commands

Ping cmd, demands a simple answer from the other device

    ?

Enter in live mode, i.e., start reading the sensors data

    l

Stop live mode

    L

Enter recording mode, i.e., record sensors data into flash

    r

Stop recording

    R

Read the recorded data (it was > before)

    p

Clear the recorded data (it was < before)

    P

Get the Version

    V

Read the Movuino's configuration

    C

Switch between ascii and binary mode

    m

### Configuration commands

**Change the accelerometer sensibility**


    c A < value >

The < value > can be:
 * 0 // for 2G
 * 1 // for 4G
 * 2 // for 8G
 * 3 // for 16G

**Change the gyroscope sensibility**


    c G < value >

The < value > can be:
 * 0 // for 250DS
 * 1 // for 500DS
 * 2 // for 1000DS
 * 3 // for 2000DS

**Change the sampling rate**

    c s < value >

The < value > is the sampling rate in Hertz

**Enable raw accelerometer at live mode**


    c a < value >

The < value > can be:
 * 0 // disable
 * 1 // enable

**Enable raw gyroscope at live mode**


    c g < value >

The < value > can be:
 * 0 // disable
 * 1 // enable

**Enable raw magnetometer at live mode**


    c m < value >

The < value > can be:
 * 0 // disable
 * 1 // enable


## Responses list

The format of the responses are composed by a tag followed by a colon, a space and its value.

    < tag >: < value > < tag >: < value > < tag >: < value > ...

The spaces are useful to export the data to a cvs file.

The tags:  
* **S:** is the sequence number, useful to detect if a packet was lost
* **F:** the frame id
 * A - for acnoledgement
 * N - for nack
 * C - for configuration
 * l - for live mode
 * \> - for rec play
* **ERR:** the error id
* **DESC:** a description
* **CFG:** the configuration id
* **VAL:** the value of the configuration
* **VER:** the version of the firmware
* **SENS:** the sensor where the data cames from
 * a - accelerometer
 * g - gyroscope
 * m - magnetometer
 * q - quaternion
 * e - euler
 * y - gravity
