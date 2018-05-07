//**************************************************************************
// GPIO(MCU pin)     NodeMCU V3 pin                        Pull Up / Down
//    0              D3  Boot select - Use if it works ok      Pull Up
//    1              D10 UART0 TX Serial - Avoid it            Pull Up
//    2              D4  Boot select - Use if it works ok      Pull Up
//    3              D9  UART0 RX Serial - Avoid               Pull Up
//    4              D2  Normally for I2C SDA                  Pull Up
//    5              D1  Normally for I2C SCL                  Pull Up
//    6              --- For SDIO Flash - Not useable          Pull Up
//    7              --- For SDIO Flash - Not useable          Pull Up
//    8              --- For SDIO Flash - Not useable          Pull Up
//    9              D11 For SDIO Flash - May not work         Pull Up
//   10              D12 For SDIO Flash - May work             Pull Up
//   11              --- For SDIO Flash - Not useable          Pull Up
//   12              D6  Ok Use                                Pull Up
//   13              D7  Ok Use                                Pull Up
//   14              D5  Ok Use                                Pull Up
//   15              D8  Boot select - Use if it works ok      Pull Up (N/A)
//   16              D0  Wake up - May cause reset - Avoid it  Pull Down
//
// The value of the internal pull-resistors are between 30k and 100k ohms.
// A google search shows that using them is not the way to go (difficult).
// The 3.3V supply may deliver up to 800mA (also from google).
//**************************************************************************
// Logic input/output levels with 3.3V power supply to ESP8266:
// LOW: -0.3V -> 0.825V, HiGH: 2.475V -> 3.6V
//**************************************************************************
// BOOT SELECT ALTERNATIVES:
// GPIO15   GPIO0    GPIO2         Mode
// 0V       0V       3.3V          UART Bootloader
// 0V       3.3V     3.3V          Boot sketch (SPI flash)
// 3.3V     x        x             SDIO mode (not used for Arduino)
//**************************************************************************
// BME280 I2C address changed from default 0x77 to 0x76 in Adafruit_BME280.h
//**************************************************************************

#include <ESP8266WiFi.h>
//#include <WiFiClient.h> // Already included in ESP8266WiFi.h
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
// #include <Wire.h> // Already included in Adafruit_BME280.h
#include <Servo.h>

// #define SEALEVELPRESSURE_HPA 1013.25 // Not used
#define MOTOR_LOW 900
#define MOTOR_NEUTRAL 1500
#define MOTOR_HIGH 2100

MDNSResponder mdns;
static void writeLED0(bool);
static void writeLED1(bool);
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
Adafruit_BME280 bme;
Servo myServo;

static const char ssid[] = "Lamb112015";
static const char password[] = "152KUEgT";

// Define GPIOs
const int LEDPIN0 = 12; // D6 on LoLin NodeMCU v3
const int LEDPIN1 = 14; // D5       - " -
const int INPIN0 = 13;  // D7       - " -
const int INPIN1 = 15;  // D8       - " -
const int MOTORPIN = 2; // D4       - " -
bool indOk0 = false;    // LED0 send missing indication popup only once
bool indOk1 = false;    // LED1                - " -

// Variables to keep current LED status: false = OFF, true = ON
bool LEDStatus0;
bool LEDStatus1;

// C string arrays used as command words sent through the Web Socket
const char LEDON0[] = "ledon0";
const char LEDOFF0[] = "ledoff0";
const char LEDON1[] = "ledon1";
const char LEDOFF1[] = "ledoff1";

int adcVal = 0;            // ADC value from analog input on pin A0
float voltage = 0;         // Calculated voltage based on max value 3.3V
long currMillis = 0;       // Milliseconds since ESP8266 started
long oldMillis = 0;        // Container for millisecond value at last poll
int counter = 0;           // To sequence polling of inputs in the loop()
long millisInterval = 600; // Duration between pollings
float temperature = 0;
float pressure = 0;
// float altitude = 0;     // Not used
float humidity = 0;
int motorVal;

// The following lines declare a string that goes into flash memory:
// The string is defined by: "rawliteral(... any text / characters... )rawliteral"
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>ESP8266 WebSocket Demo</title>
<style>
"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
</style>

<script>
// Create a Websock object (variable?) to allow communication between server and client
var websock;
function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    console.log(evt);
    var e = document.getElementById('ledstatus0');
    if (evt.data === 'ledon0') {
      e.style.color = 'Red';
    }
    else if (evt.data === 'ledoff0') {
      e.style.color = 'black';
    }
    else if (evt.data === '04#') {
      e.style.color = 'LightGray';
      window.alert('LED0 indikering feil');
    }
    else {
      console.log('unknown event');
    }
    var f = document.getElementById('ledstatus1');
    if (evt.data === 'ledon1') {
      f.style.color = 'LimeGreen';
    }
    else if (evt.data === 'ledoff1') {
      f.style.color = 'black';
    }
    else if (evt.data === '05#') {
      f.style.color = 'LightGray';
      window.alert('LED1 indikering feil');
    }
    else {
      console.log('unknown event');
    }
    // Handling the measurands by cyclic generating of broadcast event
    var g = evt.data;
    var h;
    if (g.substring(0, 3) === '00#') {
      h = g.substring(3)
      document.getElementById('volts').innerHTML = h;
    }
    var i;
    if (g.substring(0, 3) === '01#') {
      i = g.substring(3)
      document.getElementById('temp').innerHTML = i;
    }
    var j;
    if (g.substring(0, 3) === '02#') {
      j = g.substring(3)
      document.getElementById('press').innerHTML = j;
    }
    var k;
    if (g.substring(0, 3) === '03#') {
      k = g.substring(3)
      document.getElementById('hum').innerHTML = k;
    }
  };
}
// The following functions are called from the client html page
// LED On - Off buttons
function commandButtonclick(clickVal) {
  websock.send(clickVal.id);
}
// Direct positioning of servo motor 
function servoButtonClickL() {
  var lNum = '06#';
  //console.log('lNum: ' + lNum);
  websock.send(lNum);
}
function servoButtonClickC() {
  var cNum = '07#';
  //console.log('cNum: ' + cNum);
  websock.send(cNum);
}
function servoButtonClickR() {
  var rNum = '08#';
  //console.log('rNum: ' + rNum);
  websock.send(rNum);
}
// Controlling servo motor with slider
function sendSlider() {
  var o = parseInt(document.getElementById('sliderPos').value).toString();
  var sNum = '09#' + o;
  //console.log('sNum: ' + sNum);
  websock.send(sNum);
}
</script>
</head>

<!-- Starting the javascript -->
<body onload="javascript:start();">

<!-- Content shown on the client screen -->
<table>
<tr><td><b>ESP8266</b></td><td><b> WEBSOCKET</b></td></tr>
<tr><td>NodeMCU v3</td><td> dev board</td></tr>
<tr><td><br></td></tr>
<tr><td><div id="ledstatus0"><b>LED0_RED</b></div></td></tr>
<tr><td><button id="ledon0"  type="button" onclick="commandButtonclick(this);">On</button></td>
<td><button id="ledoff0" type="button" onclick="commandButtonclick(this);">Off</button></td></tr>
<tr><td><div id="ledstatus1"><b>LED1_GREEN</b></div></td></tr>
<tr><td><button id="ledon1"  type="button" onclick="commandButtonclick(this);">On</button></td>
<td><button id="ledoff1" type="button" onclick="commandButtonclick(this);">Off</button></td></tr>
<tr><td><br></td></tr>
<tr><td><b>WEATHER:</b></td></tr>
<tr><td><b>Temperature</b>&nbsp</td>
<td align="right"><div id="temp">xxxxxxx</div></td>
<td>C</td></tr>
<tr><td><b>Pressure</b>&nbsp</td>
<td align="right"><div id="press">xxxxxxx</div></td>
<td>hPa</td></tr>
<tr><td><b>Humidity</b>&nbsp</td>
<td align="right"><div id="hum">xxxxxxx</div></td>
<td>% RH</td></tr>
<tr><td><br></td></tr>
<tr><td><b>VOLTAGE:</b></td></tr>
<tr><td><b>Pin A0</b></td>
<td align="right"><div id="volts">xxxxxxx</div></td>
<td>V (0-3.3V)</td></tr>
<tr><td><br></td></tr>
<tr><td><b>MOTOR:</b></td></tr>
<tr><td align = "right"><button id="servoLeft" type="button" onclick="servoButtonClickL(this);">Left</button></td>
<td align = "center"><button id="servoCenter" type="button" onclick="servoButtonClickC(this);">Centre</button></td>
<td><button id="servoRight" type="button" onclick="servoButtonClickR(this);">Right</button></td></tr>
<tr><td></td><td><input id="sliderPos" type="range" min="0" max="1023" step="1" oninput="sendSlider(this);"></input></td></tr>
</table>
</body>
</html>
)rawliteral";
//End of what goes into flash memory



void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.println();
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // Send the current LED status
        if (LEDStatus0) {
          webSocket.sendTXT(num, LEDON0, strlen(LEDON0));
        }
        else {
          webSocket.sendTXT(num, LEDOFF0, strlen(LEDOFF0));
        }
        if (LEDStatus1) {
          webSocket.sendTXT(num, LEDON1, strlen(LEDON1));
        }
        else {
          webSocket.sendTXT(num, LEDOFF1, strlen(LEDOFF1));
        }
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\r\n", num, payload);
      // Set pin GPIO12 (D6) high
      if (strcmp(LEDON0, (const char *)payload) == 0) {
        writeLED0(true);
        delay(50); //Simulate time to operate a relay
        // Check expected back-indication on GPIO13 (D7)
        if (digitalRead(INPIN0) == 1) {
          // send data to all connected clients
          webSocket.broadcastTXT(payload, length);
          Serial.println("Command successfully executed, back-indication ok.");
        }
      }
      // Set pin GPIO12 (D6) low
      else if (strcmp(LEDOFF0, (const char *)payload) == 0) {
        writeLED0(false);
        delay(50); //Simulate time to operate a relay
        // Check expectedback-indication on GPIO13 (D7)
        if (digitalRead(INPIN0) == 0) {
          // send data to all connected clients
          webSocket.broadcastTXT(payload, length);
          Serial.println("Command successfully executed, back-indication ok.");
        }
      }
      // Set pin GPIO14 (D5) high
      else if (strcmp(LEDON1, (const char *)payload) == 0) {
        writeLED1(true);
        delay(50); //Simulate time to operate a relay
        // Check expected back-indication on GPIO15 (D8)
        if (digitalRead(INPIN1) == 1) {
          // send data to all connected clients
          webSocket.broadcastTXT(payload, length);
          Serial.println("Command successfully executed, back-indication ok.");
        }
      }
      // Set pin GPIO14 (D5) low
      else if (strcmp(LEDOFF1, (const char *)payload) == 0) {
        writeLED1(false);
        delay(50); //Simulate time to operate a relay
        // Check expected back-indication on GPIO15 (D8)
        if (digitalRead(INPIN1) == 0) {
          // send data to all connected clients
          webSocket.broadcastTXT(payload, length);
          Serial.println("Command successfully executed, back-indication ok.");
        }
      }
      // Convert left endpoint button click string to PWM value for motor control
      else if ((payload[0] == '0') && (payload[1] == '6') && (payload[2] == '#')) {
        myServo.writeMicroseconds(MOTOR_LOW);
      }
      // Convert midpoint button click string to PWM value for motor control
      else if ((payload[0] == '0') && (payload[1] == '7') && (payload[2] == '#')) {
        myServo.writeMicroseconds(MOTOR_NEUTRAL);
      }
      // Convert right endpoint button click string to PWM value for motor control
      else if ((payload[0] == '0') && (payload[1] == '8') && (payload[2] == '#')) {
        myServo.writeMicroseconds(MOTOR_HIGH);
      }
      // Convert slider input string to PWM value for motor control
      else if ((payload[0] == '0') && (payload[1] == '9') && (payload[2] == '#')) {
        String motor1 = (const char *)payload;
        String motor2 = motor1.substring(3);
        motorVal = motor2.toInt();
        myServo.writeMicroseconds((int)(900 + (motorVal / 1023.) * (MOTOR_HIGH - MOTOR_LOW)));
      }
      // If user input does not fit any of the tests above
      else {
        Serial.println();
        Serial.println("User input rejected, unknown error.");
      }
      // send data to all connected clients
      // webSocket.broadcastTXT(payload, length);
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);
      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}



// Write command to output pin LEDPIN0
static void writeLED0(bool LEDon0)
{
  LEDStatus0 = LEDon0;
  // Note inverted logic for Adafruit HUZZAH board
  if (LEDon0) {
    digitalWrite(LEDPIN0, 1);
  }
  else {
    digitalWrite(LEDPIN0, 0);
  }
}



// Write command to output pin LEDPIN1
static void writeLED1(bool LEDon1)
{
  LEDStatus1 = LEDon1;
  // Note inverted logic for Adafruit HUZZAH board
  if (LEDon1) {
    digitalWrite(LEDPIN1, 1);
  }
  else {
    digitalWrite(LEDPIN1, 0);
  }
}



void handleRoot()
{
  server.send_P(200, "text/html", INDEX_HTML);
}



void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}



void setup()
{
  // Define and initialize command pins
  pinMode(LEDPIN0, OUTPUT);
  writeLED0(false);
  pinMode(LEDPIN1, OUTPUT);
  writeLED1(false);

  // Define back-indication  pins
  pinMode(INPIN0, INPUT);
  pinMode(INPIN1, INPUT);

  Serial.begin(115200);

  //Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  // Confirm valid BME280 available
  bool status;
  status = bme.begin();  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  // Set position of servo motor to midpoint at startup
  myServo.attach(MOTORPIN);
  myServo.writeMicroseconds(MOTOR_NEUTRAL);

  WiFiMulti.addAP(ssid, password);

  while(WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("espWebSock", WiFi.localIP())) {
    Serial.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }
  else {
    Serial.println("MDNS.begin failed");
  }
  Serial.print("Connect to http://espWebSock.local or http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}



void loop()
{
  webSocket.loop();
  server.handleClient();

  // Poll input pins cyclic and update values on webpage
  currMillis = millis(); // Total milliseconds since last boot
  if ((currMillis - oldMillis) > millisInterval) {
    oldMillis = currMillis;
    // Read voltage from hardware and broadcast to clients
    if (counter == 0) {
      adcVal = analogRead(A0); // 10 bits - returns int in [0, 1023]
      voltage = 3.3 * adcVal / 1023;
      String str00 = "00#";
      String str01 = String(voltage);
      String str02 = str00 + str01;
      webSocket.broadcastTXT(str02);
    // Read temperature from hardware and broadcast to clients
    } else if (counter == 1) {
      temperature = bme.readTemperature();
      String str10 = "01#";
      String str11 = String(temperature);
      String str12 = str10 + str11;
      webSocket.broadcastTXT(str12);
    // Read pressure from hardware and broadcast to clients
    } else if (counter == 2) {
      pressure = bme.readPressure() / 100.0F;
      String str20 = "02#";
      String str21 = String(pressure);
      String str22 = str20 + str21;
      webSocket.broadcastTXT(str22);
    // Read humidity from hardware and broadcast to clients
    } else if (counter == 3) {
      humidity = bme.readHumidity();
      String str30 = "03#";
      String str31 = String(humidity);
      String str32 = str30 + str31;
      webSocket.broadcastTXT(str32);
    // Check that LED0 indication is correct
    } else if (counter == 4) {
      if (digitalRead(INPIN0) == LEDStatus0) {
        indOk0 = true;
        if (LEDStatus0 == true) {
          webSocket.broadcastTXT("ledon0");
        } else {
          webSocket.broadcastTXT("ledoff0");
        }
      }
      if ((digitalRead(INPIN0) != LEDStatus0) && (indOk0 == true)) {
        String str42 = "04#";
        webSocket.broadcastTXT(str42);
        indOk0 = false;
      }
    // Check that LED1 indication is correct
    } else if (counter == 5) {
      if (digitalRead(INPIN1) == LEDStatus1) {
        indOk1 = true;
        if (LEDStatus1 == true) {
          webSocket.broadcastTXT("ledon1");
        } else {
          webSocket.broadcastTXT("ledoff1");
        }
      }
      if ((digitalRead(INPIN1) != LEDStatus1) && (indOk1 == true)) {
        String str52 = "05#";
        webSocket.broadcastTXT(str52);
        indOk1 = false;
      }
      counter = -1;
    } 
    counter++;
    /* Not used in this application:
    altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);*/
  }
}
