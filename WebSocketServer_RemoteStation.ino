//**************************************************************************
// Code for the LoLin NodeMCU V3 board with onboard ESP8266-12E WiFi module.
// The views and controls are presented on a webpage.
// + Control two digital outputs
// + Read corresponding back-indications on two digital inputs
// + Read one analog measurand
// + Control one R/C servo motor
// + Read barometric pressure, temperature and humidity from a BME280
//   weather sensor
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
//
// The 3.3V supply on the board may deliver up to 800mA (also from google).
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
// #include <WiFiClient.h> // Already included in ESP8266WiFi.h
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
// #include <Wire.h> // Already included in Adafruit_BME280.h
#include <Servo.h>

// #define SEALEVELPRESSURE_HPA 1013.25 // Not used in this program
#define MOTOR_LOW 900
#define MOTOR_NEUTRAL 1500
#define MOTOR_HIGH 2100

MDNSResponder mdns;
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
Adafruit_BME280 bme;
Servo myServo;

static const char ssid[] = "Your_ssid";
static const char password[] = "Your_WiFi_password";

// Define GPIOs
const int OUTPIN0 = 12; // D6   On LoLin NodeMCU V3
const int OUTPIN1 = 14; // D5          - " -
const int INPIN0 = 13;  // D7          - " -
const int INPIN1 = 15;  // D8          - " -
const int MOTORPIN = 2; // D4          - " -
bool indOk0 = false;    // LED0 Indication error popup only once
bool indOk1 = false;    // LED1        - " -

// Variables to keep output channel status: false = OFF, true = ON
bool ch0Status;
bool ch1Status;

// C char arrays serving as command words sent through the Web Socket
const char CH0ON[] = "ch0On";
const char CH0OFF[] = "ch0Off";
const char CH1ON[] = "ch1On";
const char CH1OFF[] = "ch1Off";

int adcVal = 0;            // ADC value from analog input on pin A0
float voltage = 0;         // Calculated voltage based on max value 3.3V
long currMillis = 0;       // Milliseconds since ESP8266 started
long oldMillis = 0;        // Container for millisecond value at last poll
int counter = 0;           // Sequence polling of inputs in the loop()
long millisInterval = 150; // Milliseconds between pollings
float temperature = 0;
float pressure = 0;
// float altitude = 0;     // Not used in this program
float humidity = 0;
int motorVal;

// The following lines declare a string that goes into flash memory.
// Defined by: "rawliteral(... any text / characters... )rawliteral"
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
// Create a Websock object for the javascript
var websock;

function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  
  // The onmessage function below is called when a message
  // is sent to the server from the Arduino hardware code.
  // Example call: webSocket.broadcastTXT(str52);
  
  websock.onmessage = function(evt) {
    console.log(evt);
    var g = evt.data;
    // Cyclic update of the measurands
    if (g.substring(0, 3) === '00#') {
      var a = g.substring(3);
      document.getElementById('volts').innerHTML = a;
    }
    if (g.substring(0, 3) === '01#') {
      var b = g.substring(3);
      document.getElementById('temp').innerHTML = b;
    }
    if (g.substring(0, 3) === '02#') {
      var c = g.substring(3);
      document.getElementById('press').innerHTML = c;
    }
    if (g.substring(0, 3) === '03#') {
      var d = g.substring(3);
      document.getElementById('hum').innerHTML = d;
    }
    // Ch0 Red LED
    var e = document.getElementById('ch0Status');
    if (g === 'ch0On') {
      e.style.color = 'Red';
    }
    else if (g === 'ch0Off') {
      e.style.color = 'black';
    }
    else if (g === '04#') {
      e.style.color = 'LightGray';
      window.alert('CH0 indikering feil');
    }
    else {
      console.log('unknown event');
    }
    // Ch1 Green LED
    var f = document.getElementById('ch1Status');
    if (g === 'ch1On') {
      f.style.color = 'LimeGreen';
    }
    else if (g === 'ch1Off') {
      f.style.color = 'black';
    }
    else if (g === '05#') {
      f.style.color = 'LightGray';
      window.alert('CH1 indikering feil');
    }
    else {
      console.log('unknown event');
    }
    // Progressbar for servo position
    if (g.substring(0, 3) === '06#') {
      var m = g.substring(3);
      document.getElementById('progBar').value = m;
    }
  }; // End of onmessage function
} // End of start function

// The following functions are called from button clicks on the client
// html page. Each websock.send(...) generates a websocket event and
// calls the webSocketEvent(...) method in the C code.

// LED On - Off
function commandButtonclick(clickVal) {
  websock.send(clickVal.id);
}
// Servo positioning
function servoMin() {
  var numMin = '08#';
  //console.log('numMin: ' + numMin);
  websock.send(numMin); // Send only the message header
}
function servoDn() {
  var numA = '09#';
  //console.log('numA: ' + numA);
  websock.send(numA); // Send only the message header
}
function servoCenter() {
  var numCenter = '10#';
  //console.log('numCenter: ' + numCenter);
  websock.send(numCenter); // Send only the message header
}
function servoUp() {
  var numF = '11#';
  //console.log('numF: ' + numF);
  websock.send(numF); // Send only the message header
}
function servoMax() {
  var maxNum = '12#';
  //console.log('maxNum: ' + maxNum);
  websock.send(maxNum); // Send only the message header
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
<tr><td><div id="ch0Status"><b>CH0_RED_LED</b></div></td></tr>
<tr><td><button id="ch0On" type="button" onclick="commandButtonclick(this);">On</button></td>
<td><button id="ch0Off" type="button" onclick="commandButtonclick(this);">Off</button></td></tr>
<tr><td><div id="ch1Status"><b>CH1_GREEN_LED</b></div></td></tr>
<tr><td><button id="ch1On" type="button" onclick="commandButtonclick(this);">On</button></td>
<td><button id="ch1Off" type="button" onclick="commandButtonclick(this);">Off</button></td></tr>
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
<tr><td><b>MOTOR PWM:</b></td></tr>
<tr><td align="center"><button id="Mi" type="button" onclick="servoMin(this);">L</button>
<button id="Dn" type="button" onclick="servoDn(this);"><</button>
<button id="Ce" type="button" onclick="servoCenter(this);">N</button>
<button id="Up" type="button" onclick="servoUp(this);">></button>
<button id="Ma" type="button" onclick="servoMax(this);">R</button></td></tr>
</table>
<table>
<tr><td align="center"><progress id="progBar" value="0" max="1200">50 %</progress></td></tr>
</table>
</body>
</html>
)rawliteral";
// End of what goes into flash memory



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
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\r\n", num, payload);

      // Set pin GPIO12 (D6) high
      if (strcmp(CH0ON, (const char *)payload) == 0) {
        ch0Status = true;
        digitalWrite(OUTPIN0, 1);
        delay(20); //Simulate time to operate a relay
        // Check back-indication on GPIO13 (D7)
        if (digitalRead(INPIN0) == 1) {
          // send data to all connected clients
          webSocket.broadcastTXT(payload, length);
          Serial.println("Command successfully executed, back-indication ok.");
        }
      }
      // Set pin GPIO12 (D6) low
      else if (strcmp(CH0OFF, (const char *)payload) == 0) {
        ch0Status = false;
        digitalWrite(OUTPIN0, 0);
        delay(20); //Simulate time to operate a relay
        // Check back-indication on GPIO13 (D7)
        if (digitalRead(INPIN0) == 0) {
          // send data to all connected clients
          webSocket.broadcastTXT(payload, length);
          Serial.println("Command successfully executed, back-indication ok.");
        }
      }
      // Set pin GPIO14 (D5) high
      else if (strcmp(CH1ON, (const char *)payload) == 0) {
        ch1Status = true;
        digitalWrite(OUTPIN1, 1);
        delay(20); //Simulate time to operate a relay
        // Check back-indication on GPIO15 (D8)
        if (digitalRead(INPIN1) == 1) {
          // send data to all connected clients
          webSocket.broadcastTXT(payload, length);
          Serial.println("Command successfully executed, back-indication ok.");
        }
      }
      // Set pin GPIO14 (D5) low
      else if (strcmp(CH1OFF, (const char *)payload) == 0) {
        ch1Status = false;
        digitalWrite(OUTPIN1, 0);
        delay(20); //Simulate time to operate a relay
        // Check back-indication on GPIO15 (D8)
        if (digitalRead(INPIN1) == 0) {
          // send data to all connected clients
          webSocket.broadcastTXT(payload, length);
          Serial.println("Command successfully executed, back-indication ok.");
        }
      }
      // Servo command L
      else if ((payload[0] == '0') && (payload[1] == '8') && (payload[2] == '#')) {
        myServo.writeMicroseconds(MOTOR_LOW); // 900us
        motorVal = MOTOR_LOW;
      }
      // Servo command <
      else if ((payload[0] == '0') && (payload[1] == '9') && (payload[2] == '#')) {
        if (motorVal >= (MOTOR_LOW + 150)) {
          motorVal -= 150;
          myServo.writeMicroseconds(motorVal); // Decrease by 150us
        }
      }
      // Servo command N
      else if ((payload[0] == '1') && (payload[1] == '0') && (payload[2] == '#')) {
        myServo.writeMicroseconds(MOTOR_NEUTRAL); // 1500us
        motorVal = MOTOR_NEUTRAL;
      }
      // Servo command >
      else if ((payload[0] == '1') && (payload[1] == '1') && (payload[2] == '#')) {
        if (motorVal <= (MOTOR_HIGH - 150)) {
           motorVal += 150;
           myServo.writeMicroseconds(motorVal); // Increase by 150us
        }
      }
      // Servo command R
      else if ((payload[0] == '1') && (payload[1] == '2') && (payload[2] == '#')) {
        myServo.writeMicroseconds(MOTOR_HIGH); // 2100us
        motorVal = MOTOR_HIGH;
      }
      // If user input does not fit any of the above tests
      else {
        Serial.println();
        Serial.println("User input rejected, unknown error.");
      }
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
  pinMode(OUTPIN0, OUTPUT);
  digitalWrite(OUTPIN0, 0);
  ch0Status = false;
  pinMode(OUTPIN1, OUTPUT);
  digitalWrite(OUTPIN1, 0);
  ch1Status = false;
  

  // Define back-indication  pins
  pinMode(INPIN0, INPUT);
  pinMode(INPIN1, INPUT);

  Serial.begin(115200);

  // Serial.setDebugOutput(true);

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
    Serial.println("Could not find a valid BME280 sensor, check address and/or wiring!");
    while (1);
  }

  // Set position of servo motor to midpoint on startup
  myServo.attach(MOTORPIN);
  myServo.writeMicroseconds(MOTOR_NEUTRAL);
  motorVal = 1500;

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
    // Read voltage and broadcast to clients
    if (counter == 0) {
      adcVal = analogRead(A0); // 10 bits - returns int in [0, 1023]
      voltage = 3.3 * adcVal / 1023;
      String str00 = "00#";
      String str01 = String(voltage);
      String str02 = str00 + str01;
      webSocket.broadcastTXT(str02);
    // Read temperature and broadcast to clients
    } else if (counter == 1) {
      temperature = bme.readTemperature();
      String str10 = "01#";
      String str11 = String(temperature);
      String str12 = str10 + str11;
      webSocket.broadcastTXT(str12);
    // Read pressure and broadcast to clients
    } else if (counter == 2) {
      pressure = bme.readPressure() / 100.0F;
      String str20 = "02#";
      String str21 = String(pressure);
      String str22 = str20 + str21;
      webSocket.broadcastTXT(str22);
    // Read humidity and broadcast to clients
    } else if (counter == 3) {
      humidity = bme.readHumidity();
      String str30 = "03#";
      String str31 = String(humidity);
      String str32 = str30 + str31;
      webSocket.broadcastTXT(str32);
    // Check ch0 indication
    } else if (counter == 4) {
      if (digitalRead(INPIN0) == ch0Status) {
        indOk0 = true;
        if (ch0Status == true) {
          webSocket.broadcastTXT("ch0On");
        } else {
          webSocket.broadcastTXT("ch0Off");
        }
      }
      if ((digitalRead(INPIN0) != ch0Status) && (indOk0 == true)) {
        String str42 = "04#";
        webSocket.broadcastTXT(str42);
        indOk0 = false;
      }
    // Check ch1 indication
    } else if (counter == 5) {
      if (digitalRead(INPIN1) == ch1Status) {
        indOk1 = true;
        if (ch1Status == true) {
          webSocket.broadcastTXT("ch1On");
        } else {
          webSocket.broadcastTXT("ch1Off");
        }
      }
      if ((digitalRead(INPIN1) != ch1Status) && (indOk1 == true)) {
        String str52 = "05#";
        webSocket.broadcastTXT(str52);
        indOk1 = false;
      }
    // Check position of motor (motorVal) and broadcast
    } else if (counter == 6) {
      String str60 = "06#";
      int progBarPos = motorVal - 900;
      String str61 = String(progBarPos);
      String str62 = str60 + str61;
      webSocket.broadcastTXT(str62);
      counter = -1;
    }
    counter++;
    /* Not used in this program:
    altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);*/
  }
}