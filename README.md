# MAIN README

NodeMCU V3 Remote Control Station
=================================

This code is developed for a test stand (see schematic) to explore some of the features of the ESP8266-12E WiFi module. Hopefully you will be able to download it and do necessary modifications according to your own needs.

Communication is based on the Websocket protocol (ws). To connect from the outside world, you need to forward ports 80 and 81 in your router.

With this setup you can send two commands on separate digital outputs, get back-indications on two separate digital inputs, read an analog measurand, get some weather data, and control a servo motor. The motor can be controlled to both end positions and midpoint, and also in nine smaller steps, all with buttons. The current servo position is shown in a progress bar below the buttons.

Hardware
--------
* LoLin NodeMCU V3 breakout board
* 10k potmeter
* R/C servo (eg. HEXTRONIK HXT900)
* BME280 digital sensor module (Temperature, Humidity, Pressure)
* 2 x 270ohm resistors
* 2 x 1.5kohm resistors
* 2 x 15kohm resistors
* One green LED and one red LED
* A breadboard
* Some connecting wire
* A suitable USB cable
* Optional: A 5V power source to make the arrangement stand-alone

All items are cheap, and can be found on Ebay.

Required software
-----------------
The sketch "WebSocketServer_RemoteStation.ino", the COM/serial driver for the USB to serial chip, and the necessary Arduino libraries (see below).

Get started (Windows 10)
------------------------
Install the latest Arduino for Windows from here: https://www.arduino.cc/en/Main/Software

Configure the Arduino application
---------------------------------
Start the Arduino application.

In File->Preferences->Settings, enter this additional Boards Manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json .

In Tools->Board:->Boards Manager, find and install the library "esp8266 by ESP8266 Community".

In Tools, choose Board: "NodeMCU 1.0 (ESP8266-12E Module)", Flash Size: "4M (3M SPIFFS)", CPU Frequency: "80 MHz", Upload Speed: "115200".

Connect NodeMCU V3 to the computer
----------------------------------
Download COM/Serial port file CH341SER_WINDOWS.zip from https://github.com/nodemcu/nodemcu-devkit/tree/master/Drivers . Choose "Download ZIP", extract and install the driver. Note this is for the CH340 serial chip. Check the board you purchased. Your board may have the CP2102 chip, which needs another driver (not covered here). You should be able to find that driver online, and continue with this project.

Download arduinoWebSockets library from https://github.com/Links2004/arduinoWebSockets . Choose "Download ZIP", extract the .zip file and move it to the Arduino library folder.

Download Adafruit common sensor library from https://github.com/adafruit/Adafruit_Sensor . Choose "Download ZIP", extract the .zip file and move it to the Arduino library folder.

Download Adafruit BME280 sensor library from https://github.com/adafruit/Adafruit_BME280_Library . Choose "Download ZIP", extract the .zip file and move it to the Arduino library folder. 

Note: The file Adafruit_BME280.h contains the BME280 sensor I2C address on line 32. To be able to read data from it, you may have to change the address from 0x77 to 0x76.

Connect to the webpage
----------------------
You need to update the Arduino code with the ssid and WiFi password for your local network. You will find these variables on lines 71 and 72. Compile and upload the sketch. As soon as the upload reaches 100%, open the serial monitor and check that the board connects to your local WiFi network.

The ip address of the board is reported on the serial monitor. Type this address into the address field of your browser on a PC or mobile phone. From the outside world you need to type your public ip address. This requires that you have performed the port forwarding mentioned above.

You should now have a simple webpage on your browser, allowing you to manipulate the controls and view the data readings.
