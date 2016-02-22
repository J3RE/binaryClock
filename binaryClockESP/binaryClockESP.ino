/*
  The MIT License (MIT)

  Copyright (c) 2016 J3RE

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// EEPROM
//
// 0 - 31   SSID
// 32 - 95 PW
// 96 - 102 struct time
// 103 max_ntp_tries
// 104 use_ntp
// 105 last_successfull_ntp_tries
// 106 - 111 settings_red
// 112 - 117 settings_white
// 118 utc_timezone
// 119 check_dst
// 

// Serial commands
// 
// 0x5A start
// 0xAA check
// 
// 0xFF fail
// 0xF0 mode
// 0xF1 ldr
// 0xF2 use ntp
// 0xF3 time struct
// 0xF4 settings struct red
// 0xF5 settings struct white
// 0xF6 brightness now red
// 0xF7 brightness now white
// 0xFC start in ap mode
// 0xFB webserver action
// 0xFA shutdown
// 

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <time.h>

#define LED 2

unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress timeServerIP; 
// europe NTP server pool
const char* ntpServerName = "europe.pool.ntp.org";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

MDNSResponder mdns;

ESP8266WebServer server(80);

String open = "<!DOCTYPE html><html><head><meta name = 'viewport' content = 'width = device-width, maximum-scale=1'>\
<style>body{ text-align: center; font-family: Arial, Helvetica, Sans-Serif; box-shadow: 0px 0px 12px #d9d9d9;\
border: 1px solid #DFDFDF; padding: 15px 10px 10px;}\
a:link, a:link, a:visited { text-decoration: none; color: #4d4d4d;}\
a:hover{color: #ccc;} a:active{ color: green;}\
</style>\
<title>Binary Clock</title></head><body><h2><a href='../'>BinaryClock</a></h2>";

String close = "</body></html>";

struct settings
{
  uint8_t min;
  uint8_t max;
  uint16_t ldr_limit_min;
  uint16_t ldr_limit_max;
} settings_red, settings_white;

struct tm timeinfo;

// brightness, ldr data
uint8_t brightness_now_red, brightness_now_white;
uint8_t brightness_ldr_red= 0, brightness_ldr_white = 0;
uint16_t ldr = 0;

unsigned long millis_old;
uint8_t ntp_step, use_ntp, ntp_tries, max_ntp_tries, last_successfull_ntp_tries, start_in_ap_mode, check_dst, wifi_tries, serial_case, serial_byte_counter, mode, led, max_wifi_tries;
int8_t utc_timezone;
uint8_t buffer[2];

// WiFi-STA 
String ssid = "";
String password = "";

// WiFi-AP
const char* ap_ssid = "ESP8266";
const char* ap_password = "binaryClock";

void setup(void)
{
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  
  // start communication
  while((Serial.available() == 0) || (Serial.read() != 0xAA))
  {
    Serial.write(0x5A);
    digitalWrite(LED, 1);
    delay(20);
    digitalWrite(LED, 0);
    delay(130);
  }
  Serial.write(0xAA);
  digitalWrite(LED, 1);

  // read mode
  Serial.write(0xF0);
  while(Serial.available() == 0)
    delay(2);

  mode = Serial.read();
  if((mode >= 0) && (mode <= 2))
    Serial.write(0xAA);
  else
    shutdown(1);

  // start in ap mode?
  if(mode == 0)
  {
    Serial.write(0xFC);
    while(Serial.available() == 0)
      delay(2);

    start_in_ap_mode = Serial.read();
    Serial.write(0xAA);
  }
  else
    start_in_ap_mode = 0;

  // get config
  EEPROM.begin(256);

  for (int i = 0; i < 32; ++i)
  {
    ssid += char(EEPROM.read(i));
  }

  for (int i = 0; i < 64; ++i)
  {
    password += char(EEPROM.read(32 + i));
  }

  timeinfo.tm_sec = EEPROM.read(96);
  timeinfo.tm_min = EEPROM.read(97);
  timeinfo.tm_hour = EEPROM.read(98);
  timeinfo.tm_mday = EEPROM.read(99);
  timeinfo.tm_mon = EEPROM.read(100);
  timeinfo.tm_year = EEPROM.read(101);
  timeinfo.tm_wday = EEPROM.read(102);

  max_ntp_tries = EEPROM.read(103);
  use_ntp = EEPROM.read(104);
  last_successfull_ntp_tries = EEPROM.read(105);

  max_wifi_tries = 15; // change to EEPROM.read(?);

  settings_red.min = EEPROM.read(106);
  settings_red.max = EEPROM.read(107);
  settings_red.ldr_limit_min = EEPROM.read(108) | (EEPROM.read(109) << 8);
  settings_red.ldr_limit_max = EEPROM.read(110) | (EEPROM.read(111) << 8);

  settings_white.min = EEPROM.read(112);
  settings_white.max = EEPROM.read(113);
  settings_white.ldr_limit_min = EEPROM.read(114) | (EEPROM.read(115) << 8);
  settings_white.ldr_limit_max = EEPROM.read(116) | (EEPROM.read(117) << 8);

  utc_timezone = EEPROM.read(118);
  check_dst = EEPROM.read(119);

  if(mode == 0)
  {

    if(!start_in_ap_mode)
    {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), password.c_str());

      // Wait for connection
      wifi_tries = 0;
      while ((WiFi.status() != WL_CONNECTED) && (++wifi_tries <= max_wifi_tries))
      {
        digitalWrite(LED, 1);
        delay(100);
        digitalWrite(LED, 0);
        delay(400);
      }
    }

    if (start_in_ap_mode || (wifi_tries > max_wifi_tries))
    {
      WiFi.disconnect();
      delay(200);
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ap_ssid, ap_password);
    }

    // connect to wifi and visit http://binaryClock.local
    mdns.begin("binaryClock");

    server.on ( "/", handleRoot);
    server.on ( "/Settings", handleSettings);
    server.on ( "/Test", handleTest);
    server.on ( "/TestBrightness", handleTestBrightness);
    server.on ( "/TestLDR", handleTestLDR);
    server.on ( "/Info", handleInfo);
    server.on ( "/ssid_pw", handleSaveSSID);
    server.on ( "/use_ntp", writeNtp);
    server.on ( "/Time", handleUpdateTime);
    server.on ( "/set_time", handleSetTime);
    server.on ( "/brightness_red", writeRedBrightness);
    server.on ( "/brightness_white", writeWhiteBrightness);
    server.on ( "/NTP", []() 
    {
      digitalWrite(LED, 0);
      server.send(200, "text/plain", "ESP8266 is getting the time and shutting down afterwards..." );
      mode = 1;
      digitalWrite(LED, 1);

      sendSerial(1);

      delay(500);
      setupNTP();
    } );
    server.on ( "/Shutdown", []() 
    {
      digitalWrite(LED, 0);
      server.send(200, "text/plain", "ESP8266 is shutting down..." );
      digitalWrite(LED, 1);
      shutdown(0);
    } );
    server.onNotFound ( handleNotFound );
    server.begin();

    serial_case = 0xA0;
  }
  else if (mode == 1)
  {
    sendSerial(1);
    
    setupNTP();
  }
  else if (mode == 2)
  {

    sendSerial(1);
    sendSerial(2);
    sendSerial(3);
    sendSerial(4);

    if(use_ntp)
    {
      setupNTP();
    }

  }
  digitalWrite(LED, 1);
}
 
void loop()
{
  
  if(mode == 0)
  {
    server.handleClient();
  }
  else if(mode == 1)
  {
    getNTP();
  }
  else
  {
    shutdown(0);
  }

  checkSerial();
}