# PDFirmata
Firmata Client for Pure Data

# Current state
As of 06.2020 this external almost completely rewritten. Included pd-lib-builder and Makefile for compiling external without headaches. New stepper protocol is not implemented yet but in TODO list for pdfirmata version 2 stable release (planned to release before 07.2020). I will provide binaries for all architectures in next days.

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
Most of features are implemented but not documented (like I2C, Stepper, Serial etc.). 
You can find some usage notes in the PDFirmata.pd

# Compiling from source

## For Windows target

Check [libs](libs/) folder before compiling this project.