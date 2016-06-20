/*
   Schematic:
     [Ground] -- [10k-pad-resistor] -- | -- [thermistor] --[Vcc (5 or 3.3v)]
                                       |
                                  Analog Pin 0
   measure the voltage between 0v -> Vcc which for an Arduino is a nominal 5v, but for (say) a JeeNode, is a nominal 3.3v.   */

#include <Time.h>
#include <TimeAlarms.h>
#include <math.h>
#include <PString.h>
#include "ESP8266.h"
#include <SoftwareSerial.h>

#define PIN_VALUE 1          // numeric pin value (0 through 9) for digital output or analog input
#define ACTION_TYPE 0        // 'D' for digtal write, 'A' for analog read
#define DIGITAL_SET_VALUE 2  // Value to write (only used for digital, ignored for analog)

#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits

#define ThermistorPIN 0                 // Analog Pin 0
#define SampleInterval 50              // the value of the DISPLAY INTERVAL
#define NUMSAMPLES 10                   // how many samples to take and average, more takes longer
// but is more 'smooth'
float vcc = 4.91;                       // only used for display purposes, if used  // set to the measured Vcc.
float pad = 9500;                       // balance/pad resistor value, set this to  // the measured resistance of your pad resistor
float thermr = 10000;                   // thermistor nominal resistance

unsigned char incomingByte = 0;

int samples0[NUMSAMPLES];
int samples1[NUMSAMPLES];
int samples2[NUMSAMPLES];
int samples3[NUMSAMPLES];

int onHour = 20;
int onMinute = 0;
int offHour = 8;
int offMinute = 0;

char CS[20];                          //Return Command String
char dataBuffer[6];                   //Return Command String from /Nutricalc/Controler
PString DataStr(dataBuffer, sizeof(dataBuffer));
char bufferS[256];
PString sendStr(bufferS, sizeof(bufferS));

#define MY_SSID     "JohanWiFi"
#define PASSWORD    "M9st3rGu1"
#define HOST_NAME   "192.168.60.210"
#define QUERY_STR   "/NUTRICALC/CONTROLER"
#define HOST_PORT   (80)

SoftwareSerial mySerial(2, 3);      // RX, TX // Softserial for ESP8266
ESP8266 wifi(mySerial);

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);           // Softserial for ESP8266

  Serial.print(F("WiFi setup begin\r\n"));
  Serial.print(F("FW Version:"));
  Serial.println(wifi.getVersion().c_str());

  if (wifi.setOprToStationSoftAP()) {
    Serial.print(F("to station + softap ok\r\n"));
  } else {
    Serial.print(F("to station + softap err\r\n"));
  }

  if (wifi.joinAP(MY_SSID, PASSWORD)) {
    Serial.print(F("Join AP success\r\n"));
    Serial.print(F("IP:"));
    Serial.println( wifi.getLocalIP().c_str());
  } else {
    Serial.print(F("Join AP failure\r\n"));
  }

  if (wifi.disableMUX()) {
    Serial.print(F("single ok\r\n"));
  } else {
    Serial.print(F("single err\r\n"));
  }
  Serial.print(F("WiFi setup end\r\n"));

  setTime(0, 39, 0, 6, 3, 16); // set time to  0:39:00am Jun 3 2012
  Alarm.timerRepeat(30, SampleTemps);            // timer for every 30 seconds
  //Alarm.timerRepeat(5, GetSerial);            // timer for every 5 seconds
  Alarm.timerOnce(10, OnceOnly);             // called once after 10 seconds

  pinMode(6, OUTPUT);      // sets the digital pin as output
  pinMode(7, OUTPUT);      // sets the digital pin as output
  pinMode(8, OUTPUT);      // sets the digital pin as output
  pinMode(9, OUTPUT);      // sets the digital pin as output
}

void loop() {
  //SampleWater;
  // GetSerial;  //

  digitalClockDisplay();
  //Alarm.delay(2000); // wait one 2 second between clock display
  if (hour() == onHour &&  minute() == onMinute)    {
    LightsOn();
  }
  if (hour() == offHour &&  minute() == offMinute)    {
    LightsOff();
  }

  readStringFromSerial();


  if (CS[ACTION_TYPE] != 0)   {
    int pinValue = CS[PIN_VALUE] - '0';  // Convert char to int

    if (CS[ACTION_TYPE] == 'A')
      Serial.println(analogRead(pinValue));
    else if (CS[ACTION_TYPE] == 'D') {
      if (CS[DIGITAL_SET_VALUE] == 'H')
        digitalWrite(pinValue, HIGH);
      else if (CS[DIGITAL_SET_VALUE] == 'L')
        digitalWrite(pinValue, LOW);
      Serial.print(CS);
      Serial.println(" OK");
    }
    else if (CS[ACTION_TYPE] == 'T') {
      SetTime();
      Serial.println(" Time Set OK");
    }
    else if (CS[ACTION_TYPE] == 'L') {
      SetLights();
      Serial.println(" Lights Set OK");
    }
    else if (CS[ACTION_TYPE] == '1') {
      //printOneWireHex();
    }
    else if (CS[ACTION_TYPE] == 'V')   {
      Serial.println("VERSION_1_0_0_0");
    }
    else if (CS[ACTION_TYPE] == 'P') {
      Serial.println("PONG");
    }
    // Clean Array
    for (int i = 0; i <= 20; i++)
      CS[i] = 0;
  }

  Alarm.delay(1000);  // wait a little time
}

void SampleTemps() {

  float temp0;
  float temp1;
  float temp2;
  float temp3;

  uint8_t i;
  float average;

  // read ADC and  take N samples in a row, with a slight delay
  for (i = 0; i < NUMSAMPLES; i++) {
    samples0[i] = analogRead(0);
    samples1[i] = analogRead(1);
    samples2[i] = analogRead(2);
    samples3[i] = analogRead(3);
    Alarm.delay(SampleInterval);
  }

  // average all the samples out
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samples0[i];
  }
  average /= NUMSAMPLES;

  temp0 = Thermistor(average);     //  convert it to Celsius
  sendStr.begin();
  sendStr.print(F("GET /nutricalc/controler?A0="));
  sendStr.print(temp0, 1);     // display Celsius
  sendStr.print("&");
  // average all the samples out
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samples1[i];
  }
  average /= NUMSAMPLES;

  temp0 = Thermistor(average);     // read ADC and  convert it to Celsius
  sendStr.print("A1=");
  sendStr.print(temp0, 1);
  sendStr.print("&");
  // average all the samples out
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samples2[i];
  }
  average /= NUMSAMPLES;

  temp0 = average;     // read ADC
  sendStr.print("A2=");
  sendStr.print(temp0, 1);
  sendStr.print("&");
  // average all the samples out
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samples3[i];
  }
  average /= NUMSAMPLES;

  temp0 = average;     // read ADC
  sendStr.print("A3=");
  sendStr.print(temp0, 1);
  sendStr.print("&");

  sendStr.print("D6=");   sendStr.print(digitalRead(6));    sendStr.print("&");
  sendStr.print("D7=");   sendStr.print(digitalRead(7));    sendStr.print("&");
  sendStr.print("D8=");   sendStr.print(digitalRead(8));    sendStr.print("&");
  sendStr.print("D9=");   sendStr.print(digitalRead(9));    sendStr.print("&");

  //Serial.println(sendStr);
  sendStr.print(F(" HTTP/1.1\r\nHost: "));
  sendStr.print(HOST_NAME);
  sendStr.print(F("\r\nConnection: close\r\n\r\n"));

  Serial.println(bufferS);
  Serial.println(sizeof(bufferS));
  httpGet();
  setRelays();

}

void httpGet()
{
  //uint8_t buffer[256] = {0};
  int marker = 0;
  char chMisc;

  if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
    Serial.print(F("create tcp ok\r\n"));
  } else {
    Serial.print(F("create tcp err\r\n"));
  }

  const char *hello = bufferS;

  //    sendStr = "GET /nutricalc/controler" + sendStr + " HTTP/1.1\r\nHost: 192.168.60.210\r\nConnection: close\r\n\r\n";
  //char *hello = "GET /nutricalc/controler?A0=10 HTTP/1.1\r\nHost: 192.168.60.210\r\nConnection: close\r\n\r\n";
  //char *hello = "GET http://192.168.60.210/nutricalc/controler.aspx HTTP/1.0\r\n\r\n";
  wifi.send(( uint8_t*)hello, strlen(hello));

  //bufferS[256] = {0};
  uint32_t len = wifi.recv(( uint8_t*)bufferS, sizeof(bufferS), 10000);
  if (len > 0) {
    Serial.print("len:");
    Serial.println(len);
    Serial.print("Received:[");
    DataStr = "";
    for (uint32_t i = 0; i < len; i++) {
      Serial.print((char)bufferS[i]);
      chMisc = (char)bufferS[i];
      if (marker > 0 )
      { DataStr += chMisc;
        marker = marker - 1;
      }
      if (chMisc == '#')
      { if (marker > 0) {
          marker = 0;
        } else {
          marker = 4;
        }
      }
    }
    Serial.print("]\r\n");
  }

  if (wifi.releaseTCP()) {
    Serial.print(F("release tcp ok\r\n"));
  } else {
    Serial.print(F("release tcp err\r\n"));
  }
  //delay(5000);
}


void SetTime() {
  time_t pctime = 0;
  for (int i = 1; i < TIME_MSG_LEN ; i++) {

    if ( CS[i] >= '0' && CS[i] <= '9') {
      pctime = (10 * pctime) + (CS[i] - '0') ; // convert digits to a number
    }
  }
  setTime(pctime);   // Sync Arduino clock
  Serial.println(pctime);
}

void SetLights() {

  if (CS[1] == 'N') {
    //  On
    onHour = (( CS[2] - '0') * 10 ) + ( CS[3] - '0' );
    onMinute = (( CS[4] - '0') * 10 ) + ( CS[5] - '0' );
    Serial.print("ON Set ");
    Serial.print(onHour);          Serial.print(":");
    Serial.println(onMinute);
  }
  else if (CS[1] == 'F') {
    // Off
    offHour = (( CS[2] - '0') * 10 ) + ( CS[3] - '0' );
    offMinute = (( CS[4] - '0') * 10 ) + ( CS[5] - '0' );
    Serial.print("OFF Set ");
    Serial.print(offHour);        Serial.print(":");
    Serial.println(offMinute);
  }
  //Serial.println(CS);
}


//--------------------------------------------------------------------------------

void LightsOn() {
  if (digitalRead(7)) {
    pinMode(6, OUTPUT);      // sets the digital pin as output
    digitalWrite(6, LOW);   // sets the LED off Relay is NC
    pinMode(7, OUTPUT);      // sets the digital pin as output
    digitalWrite(7, LOW);   // sets the LED off Relay is NC
    pinMode(8, OUTPUT);      // sets the digital pin as output
    digitalWrite(8, LOW);   // sets the LED off Relay is NC
    pinMode(9, OUTPUT);      // sets the digital pin as output
    digitalWrite(9, LOW);   // sets the LED off Relay is NC
  }
}

void LightsOff() {
  if (!digitalRead(7)) {
    pinMode(6, OUTPUT);      // sets the digital pin as output
    digitalWrite(6, HIGH);   // sets the LED on
    pinMode(7, OUTPUT);      // sets the digital pin as output
    digitalWrite(7, HIGH);   // sets the LED on
    pinMode(8, OUTPUT);      // sets the digital pin as output
    digitalWrite(8, HIGH);   // sets the LED on
    pinMode(9, OUTPUT);      // sets the digital pin as output
    digitalWrite(9, HIGH);   // sets the LED on
  }
}


void setRelays() {
  Serial.println("Start Relays Set.") ;
  Serial.println(DataStr) ;
  //Serial << F("Beginning of relays DataStr:") << DataStr << endl ;
  if (DataStr[0] == '0') {
    if (digitalRead(6)) {
      pinMode(6, OUTPUT);      // sets the digital pin as output
      digitalWrite(6, LOW);   // sets the LED off Relay is NC
    }
  }
  if (DataStr[0] == '1') {
    if (!digitalRead(6)) {
      pinMode(6, OUTPUT);      // sets the digital pin as output
      digitalWrite(6, HIGH);   // sets the LED off Relay is NC
    }
  }
  if (DataStr[1] == '0') {
    if (digitalRead(7)) {
      pinMode(7, OUTPUT);      // sets the digital pin as output
      digitalWrite(7, LOW);   // sets the LED off Relay is NC
    }
  }
  if (DataStr[1] == '1') {
    if (!digitalRead(7)) {
      pinMode(7, OUTPUT);      // sets the digital pin as output
      digitalWrite(7, HIGH);   // sets the LED off Relay is NC
    }
  }
  if (DataStr[2] == '0') {
    if (digitalRead(8)) {
      pinMode(8, OUTPUT);      // sets the digital pin as output
      digitalWrite(8, LOW);   // sets the LED off Relay is NC
    }
  }
  if (DataStr[2] == '1') {
    if (!digitalRead(8)) {
      pinMode(8, OUTPUT);      // sets the digital pin as output
      digitalWrite(8, HIGH);   // sets the LED off Relay is NC
    }
  }
  if (DataStr[3] == '0') {
    if (digitalRead(9)) {
      pinMode(9, OUTPUT);      // sets the digital pin as output
      digitalWrite(9, LOW);   // sets the LED off Relay is NC
    }
  }
  if (DataStr[3] == '1') {
    if (!digitalRead(9)) {
      pinMode(9, OUTPUT);      // sets the digital pin as output
      digitalWrite(9, HIGH);   // sets the LED off Relay is NC
    }
  }
  Serial.println("Relays was Set.") ;
}

void OnceOnly() {
  //check alles
}

void readStringFromSerial() {
  int i = 0;
  if (Serial.available()) {
    while (Serial.available()) {
      CS[i] = Serial.read();
      i++;
    }
  }
}




void SampleWater() {
  int temp4 ;       // count water charge

  //temp4 = GetWaterCount(10);       // count water charge
  Serial.print("D10=");
  Serial.print(temp4);
  Serial.println("& ");

}


float Thermistor(int RawADC) {
  long Resistance;
  float Temp;  // Dual-Purpose variable to save space.

  Resistance = ((1024 * pad / RawADC) - pad);
  Temp = log(Resistance); // Saving the Log(resistance) so not to calculate  it 4 times later
  Temp = 1 / (0.001129148 + (0.000234125 * Temp) + (0.0000000876741 * Temp * Temp * Temp));
  Temp = Temp - 273.15;  // Convert Kelvin to Celsius

  // BEGIN- Remove these lines for the function not to display anything
  //Serial.print("ADC: ");
  //Serial.print(RawADC);
  //Serial.print("/1024");                           // Print out RAW ADC Number
  //Serial.print(", vcc: ");
  //Serial.print(vcc,2);
  //Serial.print(", pad: ");
  //Serial.print(pad/1000,3);
  //Serial.print(" Kohms, Volts: ");
  //Serial.print(((RawADC*vcc)/1024.0),3);
  //Serial.print(", Resistance: ");
  //Serial.print(Resistance);
  //Serial.print(" ohms, ");
  // END- Remove these lines for the function not to display anything

  // Uncomment this line for the function to return Fahrenheit instead.
  //temp = (Temp * 9.0)/ 5.0 + 32.0;                  // Convert to Fahrenheit
  return Temp;                                      // Return the Temperature
}


int GetWaterCount(int digitalPin) {
  int cnt ;
  cnt = 0;
  boolean wet = false;

  pinMode(digitalPin, OUTPUT);      // sets the digital pin as output
  digitalWrite(digitalPin, LOW);   // drain cap
  Alarm.delay(5000);                                      // Delay a bit...
  pinMode(digitalPin, INPUT);      // sets the digital pin as input
  //digitalWrite(digitalPin, HIGH);

  //  while( digitalRead(digitalPin) == false){
  //    Alarm.delay(10);
  //    cnt = cnt + 1;
  ////    if (cnt > 30000){
  ////     // break;
  ////    )
  //  }

  for (int i = 0; i < 1000; i++) {
    wet = digitalRead(digitalPin);
    cnt = i;
    if (wet) {
      break;
    }
    Alarm.delay(10);
  }
  return cnt;

}


void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print("Time=");
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();
}

void printDigits(int digits)
{
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
