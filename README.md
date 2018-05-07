# MAIN README

NodeMCU V3 Remote Control Station
=================================

This project is developed for a test stand (see schematic) and explore some of the features of ESP8266-12E WiFi module. Hopefully you will be able to modify the code and adapt it to your own needs.

Communication is based on the Websocket protocol. To connect from the outside world, you need to forward ports 80 and 81 in your router.

With this setup you can send two commands on separate digital outputs, get back-indication on two separate digital inputs, read an analog measurand, and control a servo motor. The motor can be controlled to both ends and midpoint position via buttons, and also in proportional movement via a slider. At the moment there is no feedback to the slider symbol if you use the the buttons to control the motor. This is a work in progress.

Required software
-----------------
The file WebSocketServer_RemoteStation.ino plus the necessary driver and Arduino libraries (see below).

Get started (Windows 10)
------------------------
Install the latest Arduino for Windows from here: https://www.arduino.cc/en/Main/Software

Configuration of the Arduino application:
-----------------------------------------
In File->Preferences->Settings, enter this additional Boards Manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json .

In the Boards manager, find and install library "esp8266 by ESP8266 Community".

In Tools->Boards, choose "NodeMCU 1.0 (ESP8266-12E Module)". Choose the following options for the board: CPU Frequency: "80 MHz", Flash Size: "4M (3M SPIFFS)", Upload Speed: "115200" .

Connect NodeMCU V3 to the computer.
-----------------------------------
Download COM/Serial port file CH341SER_WINDOWS.zip from https://github.com/nodemcu/nodemcu-devkit/tree/master/Drivers . Choose "Download ZIP", extract and install the driver. Note this is for the CH340 serial chip. Check the board you purchased. Your board may have the CP2102 chip, which needs another driver (not covered here). You should be able to find that driver online, and continue with this project.

Download arduinoWebSockets library from https://github.com/Links2004/arduinoWebSockets . Choose "Download ZIP", extract the .zip file and move it to the Arduino library folder.

Download Adafruit common sensor library from https://github.com/adafruit/Adafruit_Sensor . Choose "Download ZIP", extract the .zip file and move it to the Arduino library folder.

Download Adafruit BME280 sensor library from https://github.com/adafruit/Adafruit_BME280_Library . Choose "Download ZIP", extract the .zip file and move it to the Arduino library folder. 

Note: The file Adafruit_BME280.h contains the sensor I2C address on line 32. To be able to read data from the sensor, you may have to change the address from 0x77 to 0x76.

Connect to the webpage
----------------------
When the board starts, the ip address of the board is reported on the serial monitor. Type this address into the address field of your browser. From the outside world you need to type your public ip address. This requires that you have performed the port forwarding mentioned above.