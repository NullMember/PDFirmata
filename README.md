# PDFirmata
Firmata Client for Pure Data

# Current state
As of 06.2020 this external almost completely rewritten. Included pd-lib-builder and Makefile for compiling external without headaches. I hope I will release stable before 07.2020.

# TODO

- [Implement OneWire](https://github.com/firmata/protocol/blob/master/onewire.md)
- [Implement Scheduler](https://github.com/firmata/protocol/blob/master/scheduler.md)
- [Implement custom floating point format used by accelstepper](https://github.com/firmata/protocol/blob/master/accelStepperFirmata.md#accelstepperfirmatas-custom-float-format)

# Description

PDFirmata is Firmata Client for Miller S. Puckette's Pure Data. PDFirmata supports most of features of Firmata protocol. Up to 4 software and 4 hardware UART, 1 I2C, 5 Encoder, 6 Stepper. Also Servos, Digital and Analog IO, PWM Output and so.

# Arduino Instructions:

* Depending on your board you might need to install a driver for the serial port. (see http://arduino.cc/en/Guide/HomePage )
* Connect USB cable to Arduino and your computer.
* Start Arduino software.
* Select your board from Tools -> Board
* Select your Serial connection from Tools -> Serial Port
* Open the Firmata program by going to File -> Examples -> Firmata -> StandardFirmata or StandardFirmataPlus (Recommended)
* Upload the Firmata program to your board by pressing the upload button or File -> Upload
* When the upload is finished, you can close the arduino software.

# PDFirmata Instructions:

If you are using Pure Data Vanilla, please install comport external (You can use Help-Find externals). 
Look at the help file provided in this repo.

# Compiling from source

If you're cloning this repository with git command line tool don't forget to use `--recursive` option to fetch all submodules.

## For Windows targets

Check [bin](bin/) folder before compiling this project.
