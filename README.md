# MAIN README

NodeMCU V3 Remote Control Station
=================================

This project is developed for a test stand (see schematic) explore some of the features of ESP8266-12E WiFi module. Hopefully you will be able to modify the code and adapt it to your own needs.

Communication is based on the Websock protocol. To connect from the outside world, you need to forward ports 80 and 81 in your router.

Required software
-----------------
The file WebSocketServer_RemoteStation.ino plus the necessary driver and libraries (see below).

How to get the board working (Windows 10)
-----------------------------------------
Install the latest Arduino for Windows https://www.arduino.cc/en/Main/Software

Configuration of the Arduino application:
-----------------------------------------

In File->Preferences->Settings, enter additional Boards Manager URL:
http://arduino.esp8266.com/stable/package_esp8266com_index.json

In the Boards manager, find and install library "esp8266 by ESP8266 Community".

In Tools->Boards, choose "NodeMCU 1.0 (ESP8266-12E Module)"
CPU Frequency: "80 MHz"
Flash Size: "4M (3M SPIFFS)"
Upload Speed: "115200"

Connect NodeMCU V3 to the computer.
-----------------------------------

Download the COM/Serial port driver CH341SER_WINDOWS.zip from https://github.com/nodemcu/nodemcu-devkit/tree/master/Drivers
Extract and install the driver. Note this is for the CH340 serial chip. Check the board you have purchased. Another alternative is the CP2102 chip which need another driver (not covered here).

Download arduinoWebSockets from https://github.com/Links2004/arduinoWebSockets
Extract the folder and move it to the Arduino library folder.

Download Adafruit common sensor library from https://github.com/adafruit/Adafruit_Sensor
Extract the folder and move it to the Arduino library folder.

Download Adafruit Arduino Library for BME280 sensors from https://github.com/adafruit/Adafruit_BME280_Library
Extract the folder and move it to the Arduino library folder.
In the file Adafruit_BME280.h, in line 32, change the I2C address from 0x77 to 0x76.



