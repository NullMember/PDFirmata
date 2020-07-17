# PDFirmata
Firmata Client for Pure Data

# TODO

- [Implement Scheduler](https://github.com/firmata/protocol/blob/master/scheduler.md)

# Description

PDFirmata is Firmata Client for Miller S. Puckette's Pure Data. PDFirmata supports all features of Firmata protocol except Scheduler.

## Compatibility

Current PDFirmata client is compatible with Firmata protocol 2.6.0

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
