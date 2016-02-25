
void shutdown(uint8_t error)
{
  EEPROM.end();
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

  for(;;)
  {
    digitalWrite(LED, 0);
    delay(300);
    digitalWrite(LED, 1);
    delay(300);

    if (error)
    {
      Serial.write(0xFF);
      Serial.write(0xFA);
    }
    else
      Serial.write(0xFA);
  }
}

void handleRoot() 
{
  digitalWrite(LED, 0);
  char temp[250];

  sendSerial(1);

  snprintf( temp, 250,
"<p><a href='/Settings'>Settings</a></p>\
<p><a href='/Test>Test</a></p>\
<p><a href='/%s'>update Time (%s)</a></p>\
<p><a href='/Shutdown'>Shutdown</a></p>\
<p><a href='/Info'>Info</a></p>\
<font size='1'>by J3RE</font><br>",
  (use_ntp) ? "NTP" : "Time", (use_ntp) ? "NTP" : "Manual");
  server.send(200, "text/html", open + temp + close );
  digitalWrite(LED, 1);
}

void handleSettings() 
{
  digitalWrite(LED, 0);

  sendSerial(1);

  char temp1[750];
  char temp2[750];
  char temp3[700];
  char temp4[400];

  snprintf( temp1, 750,
"<form action='use_ntp'><font size='3'><strong>Time - Settings:</strong></font><br>\
<table align='center'><tr><td>use NTP-Server:</td><td>yes<input type='radio' name='ntp' value='1' %s></td><td>no<input type='radio' name='ntp' value='0' %s></td></tr>\
<tr><td>Auto DST:</td><td>yes<input type='radio' name='dst' value='1' %s></td><td>no<input type='radio' name='dst' value='0' %s></td></tr>\
<tr><td>UTC-Offset:</td><td></td><td><input type='number' name='utc_timezone' min='-12' max='12' value='%d'></td></tr>\
<tr><td>Max-NTP-tries:</td><td></td><td><input type='number' name='ntp_tries' min='1' max='25' value='%d'></td></tr>\
<tr><td></td><td><input type='submit' value='save'></td></tr></table></form><br>",
(use_ntp) ? "checked" : "", (!use_ntp) ? "checked" : "", (check_dst) ? "checked" : "", (!check_dst) ? "checked" : "", utc_timezone, max_ntp_tries);

  snprintf( temp2, 750,
"<font size='3'><strong>Brightness - Settings:</strong></font><br>\
<form action='brightness_red'>\
<font size='3'><strong>Red LEDs:</strong></font><br>\
<table align='center'><tr><td>LDR-limit Min:</td><td><input type='number' min='0' max='1023' name='ldr_limit_min_red' value='%d'></td></tr>\
<tr><td>LDR-limit Max:</td><td><input type='number' min='0' max='1023' name='ldr_limit_max_red' value='%d'></td></tr>\
<tr><td>Brightness Min:</td><td><input type='number' min='0' max='235' name='min_red' value='%d'></td></tr>\
<tr><td>Brightness Max:</td><td><input type='number' min='0' max='235' name='max_red' value='%d'></td></tr>\
<tr><td></td><td><input type='submit' value='save'></td></tr></table></form>",
   settings_red.ldr_limit_min, settings_red.ldr_limit_max, settings_red.min, settings_red.max);

  snprintf( temp3, 700,
"<form action='brightness_white'>\
<font size='3'><strong>White LEDs: </strong></font><br>\
<table align='center'><tr><td>LDR-limit Min:</td><td><input type='number' min='0' max='1023' name='ldr_limit_min_white' value='%d'></td></tr>\
<tr><td>LDR-limit Max:</td><td><input type='number' min='0' max='1023' name='ldr_limit_max_white' value='%d'></td></tr>\
<tr><td>Brightness Min:</td><td><input type='number' min='0' max='235' name='min_white' value='%d'></td></tr>\
<tr><td>Brightness Max:</td><td><input type='number' min='0' max='235' name='max_white' value='%d'></td></tr>\
<tr><td></td><td><input type='submit' value='save'></td></tr></table></form><br>",
  settings_white.ldr_limit_min, settings_white.ldr_limit_max, settings_white.min, settings_white.max);

  snprintf( temp4, 400,
"<form action='ssid_pw'>\
<font size='3'><strong>WLAN - Settings:</strong></font><br>\
<table align='center'><tr><td>SSID:</td><td><input type='text' name='input_ssid' size=20></td></tr>\
<tr><td>PW:</td><td><input type='password' name='input_password' size=20></td></tr>\
<tr><td></td><td><input type='submit' value='save'></td></tr></table></form><br>\
<font size='2'><a href='../'>back</a></font>");

  server.send(200, "text/html", open + temp1 + temp2 + temp3 + temp4 + close );
  digitalWrite(LED, 1);
}

void handleTest() 
{
  digitalWrite(LED, 0);
  char temp[650];

  sendSerial(1);

  snprintf( temp, 650,
"<form action='TestBrightness'>\
<table align='center'><tr><td>Brightness red LED:</td><td><input type='number' min='0' max='235' name='now_red' size=3 value='%d'></td></tr>\
<tr><td>Brightness white LED:</td><td><input type='number' min='0' max='235' name='now_white' size=3 value='%d'></td></tr></table>\
<font size='2'>(changes brightness for a few sec)</font><br>\
<input type='submit' value='update Brightness'></form><br>\
<form action='TestLDR'>LDR: %d<br>\
<font size='2'>red: %d,white: %d</font><br>\
<input type='submit' value='update LDR'></form>\
<font size='2'><a href='../'>back</a></font>",  
  brightness_now_red, brightness_now_white, ldr, brightness_ldr_red, brightness_ldr_white);
  server.send(200, "text/html", open + temp + close );
  digitalWrite(LED, 1);
}

void handleTestBrightness() 
{
  digitalWrite(LED, 0);
  
  uint8_t b1 = 0, b2 = 0;

  sendSerial(1);

  b1 = server.arg("now_red").toInt();
  b2 = server.arg("now_white").toInt();

  if(b1 <= 235)
  {
    brightness_now_red = b1;
    sendSerial(5);
  }
  if(b2 <= 235)
  {
    brightness_now_white = b2;
    sendSerial(6);
  }

  handleTest();
}

void handleTestLDR() 
{
  sendSerial(9);

  digitalWrite(LED, 0);

  sendSerial(1);

  millis_old = millis();

  while((millis() - millis_old) < 10)
    checkSerial();

  if(ldr > settings_red.ldr_limit_max)
    brightness_ldr_red = settings_red.max;
  else if(ldr < settings_red.ldr_limit_min)
    brightness_ldr_red = settings_red.min;
  else
    brightness_ldr_red = map(ldr, settings_red.ldr_limit_min, settings_red.ldr_limit_max, settings_red.min, settings_red.max);

  if(ldr > settings_white.ldr_limit_max)
    brightness_ldr_white = settings_white.max;
  else if(ldr < settings_white.ldr_limit_min)
    brightness_ldr_white = settings_white.min;
  else
    brightness_ldr_white = map(ldr, settings_white.ldr_limit_min, settings_white.ldr_limit_max, settings_white.min, settings_white.max);

  handleTest();
}

void handleInfo() 
{
  digitalWrite(LED, 0);
  char temp[400];
  int sec = millis() / 1000;

  sendSerial(1);

  char password_stars[64];

  int i = 0;
  while(password[i] != '\0')
  {
    password_stars[i++] = '*';
  }
  password_stars[i] = '\0';

  snprintf( temp, 400,  
"<p>SSID: %s<br>\
PW: %s</p>\
<p>last Update: %02d:%02d:%02d, %02d.%02d.%d, %d tr%s<br>\
ESP8266 Uptime: %d min. %d sec.</p>\
<font size='2'><a href='../'>back</a></font>",
  ssid.c_str(), password_stars, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, last_successfull_ntp_tries, (last_successfull_ntp_tries != 1) ? "ies" : "y", sec / 60, sec % 60);
  server.send(200, "text/html", open + temp + close );
  digitalWrite(LED, 1);
}

void handleSaveSSID() 
{
  digitalWrite(LED, 0);
  char temp[300];

  sendSerial(1);

  String temp_ssid = server.arg("input_ssid");
  String temp_pw = server.arg("input_password");
  
  if((temp_ssid.length() > 0) && (temp_pw.length() > 0))
  {
    for(int i = 0; i < 96; i++)
      EEPROM.write(i, 0);

    for (int i = 0; i < temp_ssid.length(); ++i)
      EEPROM.write(i, temp_ssid[i]);

    for (int i = 0; i < temp_pw.length(); ++i)
      EEPROM.write(32+i, temp_pw[i]);

    EEPROM.commit();

    ssid = temp_ssid;
    password = temp_pw;
    char password_stars[64];

    int i = 0;
    while(password[i] != '\0')
    {
      password_stars[i++] = '*';
    }
    password_stars[i] = '\0';

    snprintf( temp, 300,  
"<p>SSID: %s<br>\
PW: %s</p>\
<font size='2'><a href='/Settings'>back</a></font>",
    temp_ssid.c_str(), password_stars);
  }
  else
  {
    snprintf( temp, 300,
"<p>invalid input, try again!</p>\
<font size='2'><a href='/Settings'>back</a></font>");
  }

  server.send(200, "text/html", open + temp + close );
  digitalWrite(LED, 1);
}

void writeNtp() 
{
  digitalWrite(LED, 0);
  uint8_t temp_max_ntp_tries, temp_use_ntp, temp_check_dst, temp_utc_timezone;

  temp_use_ntp = server.arg("ntp").toInt();
  temp_check_dst = server.arg("dst").toInt();
  temp_utc_timezone =  server.arg("utc_timezone").toInt();
  temp_max_ntp_tries =  server.arg("ntp_tries").toInt();

  
  if((temp_max_ntp_tries <= 25) && (temp_max_ntp_tries > 0) && (abs(utc_timezone) <= 12))
  {
    use_ntp = temp_use_ntp;
    check_dst = temp_check_dst;
    utc_timezone = temp_utc_timezone;
    max_ntp_tries = temp_max_ntp_tries;

    EEPROM.write(103, max_ntp_tries);
    EEPROM.write(104, use_ntp);
    EEPROM.write(118, utc_timezone);
    EEPROM.write(119, check_dst);
    EEPROM.commit();
    
    sendSerial(2);

    handleSettings();
  }
  else
  {
    char temp[100];
    snprintf( temp, 100,  
"<p>Invalid input, try again!</p>\
<font size='2'><a href='/Settings'>back</a></font>");
    server.send(200, "text/html", open + temp + close );
    digitalWrite(LED, 1);
  }
}

void writeRedBrightness() 
{
  digitalWrite(LED, 0);
  uint8_t error = 0;
  struct settings temp_settings;

  sendSerial(1);

  temp_settings.ldr_limit_min = server.arg("ldr_limit_min_red").toInt();
  temp_settings.ldr_limit_max = server.arg("ldr_limit_max_red").toInt();
  temp_settings.min = server.arg("min_red").toInt();
  temp_settings.max = server.arg("max_red").toInt();

  if((temp_settings.min > temp_settings.max) || (temp_settings.max > 235) || (temp_settings.ldr_limit_min > temp_settings.ldr_limit_max) || (temp_settings.ldr_limit_max > 1023))
  {
    char temp[100];
    snprintf( temp, 100,  
"<p>Invalid input, try again!</p>\
<font size='2'><a href='/Settings'>back</a></font>");
    server.send(200, "text/html", open + temp + close );
    digitalWrite(LED, 1);
  }
  else
  {
    settings_red.min = temp_settings.min;
    settings_red.max = temp_settings.max;
    settings_red.ldr_limit_min = temp_settings.ldr_limit_min;
    settings_red.ldr_limit_max = temp_settings.ldr_limit_max;

    EEPROM.write(106, settings_red.min);
    EEPROM.write(107, settings_red.max);
    EEPROM.write(108, settings_red.ldr_limit_min & 0x00FF);
    EEPROM.write(109, (settings_red.ldr_limit_min >> 8) & 0x00FF);
    EEPROM.write(110, settings_red.ldr_limit_max & 0x00FF);
    EEPROM.write(111, (settings_red.ldr_limit_max >> 8) & 0x00FF);
    EEPROM.commit();

    sendSerial(3);
  
    handleSettings();
  }
}

void writeWhiteBrightness() 
{
  digitalWrite(LED, 0);
  uint8_t error = 0;
  struct settings temp_settings;

  sendSerial(1);

  temp_settings.ldr_limit_min = server.arg("ldr_limit_min_white").toInt();
  temp_settings.ldr_limit_max = server.arg("ldr_limit_max_white").toInt();
  temp_settings.min = server.arg("min_white").toInt();
  temp_settings.max = server.arg("max_white").toInt();

  if((temp_settings.min > temp_settings.max) || (temp_settings.max > 235) || (temp_settings.ldr_limit_min > temp_settings.ldr_limit_max) || (temp_settings.ldr_limit_max > 1023))
  {
    char temp[100];
    snprintf( temp, 100,  
"<p>Invalid input, try again!</p>\
<font size='2'><a href='/Settings'>back</a></font>");
    server.send(200, "text/html", open + temp + close );
    digitalWrite(LED, 1);
  }
  else
  {
    settings_white.min = temp_settings.min;
    settings_white.max = temp_settings.max;
    settings_white.ldr_limit_min = temp_settings.ldr_limit_min;
    settings_white.ldr_limit_max = temp_settings.ldr_limit_max;

    EEPROM.write(112, settings_white.min);
    EEPROM.write(113, settings_white.max);
    EEPROM.write(114, settings_white.ldr_limit_min & 0x00FF);
    EEPROM.write(115, (settings_white.ldr_limit_min >> 8) & 0x00FF);
    EEPROM.write(116, settings_white.ldr_limit_max & 0x00FF);
    EEPROM.write(117, (settings_white.ldr_limit_max >> 8) & 0x00FF);
    EEPROM.commit();

    sendSerial(4);

    handleSettings();
  }
}

void handleUpdateTime() 
{
  digitalWrite(LED, 0);
  char temp[450];

  sendSerial(1);

  snprintf( temp, 450,  
"<form action='set_time'><p>Time: <input type='text' name='hour' size=2 autofocus>:<input type='text' name='minute' size=2><br>\
Date: <input type='text' name='day' size=2>.<input type='text' name='month' size=2>.<input type='text' name='year' size=4><br>\
<font size='2'>(format: hh:mm, dd.mm.yyyy)</font><br>\
<input type='submit' value='save'></p></form>\
<font size='2'><a href='../'>back</a></font>");
  server.send(200, "text/html", open + temp + close );
  digitalWrite(LED, 1);
}

void handleSetTime() 
{
  digitalWrite(LED, 0);
  char temp[200];

  sendSerial(1);

  uint8_t error = 0;
  struct tm temp_time;

  temp_time.tm_hour = server.arg("hour").toInt();
  temp_time.tm_min = server.arg("minute").toInt();
  temp_time.tm_mday = server.arg("day").toInt();
  temp_time.tm_mon = server.arg("month").toInt() - 1;
  temp_time.tm_year = server.arg("year").toInt() - 1900;

  if(temp_time.tm_hour > 23)
    error = 1;
  if(temp_time.tm_min > 59)
    error = 1;
  if(temp_time.tm_mday > 31)
    error = 1;;
  if(temp_time.tm_mon > 11)
    error = 1;
  if(temp_time.tm_year > 300)
    error = 1;

  if(!error)
  {

    // calculate day of week
    unsigned y = temp_time.tm_year + 1900;
    uint8_t m = temp_time.tm_mon;
    uint8_t d = temp_time.tm_mday;

    static char t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    timeinfo.tm_wday = (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;


    timeinfo.tm_hour = temp_time.tm_hour;
    timeinfo.tm_min = temp_time.tm_min;
    timeinfo.tm_sec = 0;
    timeinfo.tm_mday = temp_time.tm_mday;
    timeinfo.tm_mon = temp_time.tm_mon;
    timeinfo.tm_year = temp_time.tm_year;
    
    EEPROM.write(96, 0);
    EEPROM.write(97, timeinfo.tm_min);
    EEPROM.write(98, timeinfo.tm_hour);
    EEPROM.write(99, timeinfo.tm_mday);
    EEPROM.write(100, timeinfo.tm_mon);
    EEPROM.write(101, timeinfo.tm_year);
    EEPROM.write(102, timeinfo.tm_wday);

    EEPROM.commit();

    sendSerial(7);

    snprintf( temp, 200,
"<p>Success!<br>\
Time: %02d:%02d<br>\
Date: %d.%d.%d</p>\
<font size='2'><a href='../'>back</a></font>",
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  }
  else
  {
    snprintf( temp, 200,  
"<p>Invalid input, try again!</p>\
<font size='2'><a href='../'>back</a></font>");
  }

  server.send(200, "text/html", open + temp + close );
  digitalWrite(LED, 1);
}

void handleNotFound()
{
  sendSerial(1);

  digitalWrite(LED, 0);
  String message = "Not Found\n\n";
  message += "The requested URL ";
  message += server.uri();
  message += " was not found on this server.\n";

  server.send (404, "text/plain", message );
  digitalWrite(LED, 1);
}


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void getNTP(void)
{  
  switch (ntp_step)
  { 
    case 0:
      //get a random server from the pool
      WiFi.hostByName(ntpServerName, timeServerIP); 
    
      sendNTPpacket(timeServerIP); // send an NTP packet to a time server
      // wait to see if a reply is available
      millis_old = millis();
      ntp_step++;
      break;

    case 1:
      // wait 1 sec
      if((millis() - millis_old) >= 1000)
        ntp_step++;
      break;

    case 2:
      if ( udp.parsePacket() )
      {
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    
        //the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:
    
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secs_since_1900 = highWord << 16 | lowWord;
        const unsigned long seventy_years = 2208988800UL;        

        // -1s for the waiting
        time_t now = secs_since_1900 - seventy_years + utc_timezone * 3600U - 1;;

        timeinfo = *localtime(&now);

        // Daylight Saving Time, for central europe
        // check out: http://hackaday.com/2012/07/16/automatic-daylight-savings-time-compensation-for-your-clock-projects/
        if(check_dst)
        {
          int y = timeinfo.tm_year + 1900;

          uint8_t start_day = 31;
          uint8_t end_day = 31;
          int8_t dow = (y + y/4 - y/100 + y/400 + 33) % 7;

          while(dow != 0)
          {
            dow = --dow % 7;
            start_day--;
          }

          dow = (y + y/4 - y/100 + y/400 + 37) % 7;

          while(dow != 0)
          {
            dow = --dow % 7;
            end_day--;
          }

          if(((timeinfo.tm_mon > 2) && (timeinfo.tm_mon < 9)) || ((timeinfo.tm_mon == 2) && (timeinfo.tm_mday >= start_day) && (timeinfo.tm_hour >= 2))\
             || ((timeinfo.tm_mon == 9) && (timeinfo.tm_mday <= end_day) && (timeinfo.tm_hour > 2)))
          {
            timeinfo.tm_isdst = 1;
            now += 3600;
            timeinfo = *localtime(&now);
          }
          else
          {
            timeinfo.tm_isdst = 0;
          }
        }

        EEPROM.write(96, timeinfo.tm_sec);
        EEPROM.write(97, timeinfo.tm_min);
        EEPROM.write(98, timeinfo.tm_hour);
        EEPROM.write(99, timeinfo.tm_mday);
        EEPROM.write(100, timeinfo.tm_mon);
        EEPROM.write(101, timeinfo.tm_year);
        
        EEPROM.write(105, ntp_tries);

        EEPROM.commit();

        sendSerial(8);

        shutdown(0);

      }
      else
      {
        if(++ntp_tries >= max_ntp_tries)
        {
          shutdown(1);
        }
        else
        {
          millis_old = millis();
          ntp_step++;      
        }
      }
      break;
    
    case 3:
      // wait 1 sec
      if((millis() - millis_old) > 1000)
        ntp_step = 0;
      break;

    default:
      shutdown(1);
      break;
  }
}

void setupNTP(void)
{    
  if((mode == 1) || (mode == 2) || start_in_ap_mode || (wifi_tries > max_wifi_tries))
  {
    mode = 1;
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
    digitalWrite(LED, 1);

    if (wifi_tries > max_wifi_tries)
    {
      shutdown(1);
    }
  }
  
  udp.begin(localPort);
  ntp_tries = 0;
  ntp_step = 1;
}

void sendSerial(uint8_t serial_case)
{
  switch (serial_case)
  {
    // webserver active
    case 1:
      Serial.write(0xFB);
      Serial.write(0xAA);
      break;

    // send use_ntp
    case 2:
      Serial.write(0xF2);
      Serial.write(use_ntp);
      Serial.write(0xAA);
      break;

    // send brightness settings red
    case 3:
      Serial.write(0xF4);
      Serial.write(settings_red.min);
      Serial.write(settings_red.max);
      Serial.write( settings_red.ldr_limit_min & 0x00FF);
      Serial.write((settings_red.ldr_limit_min >> 8) & 0x00FF);
      Serial.write( settings_red.ldr_limit_max & 0x00FF);
      Serial.write((settings_red.ldr_limit_max >> 8) & 0x00FF);
      Serial.write(0xAA);
      break;

    // send brightness settings white
    case 4:
      Serial.write(0xF5);
      Serial.write(settings_white.min);
      Serial.write(settings_white.max);
      Serial.write( settings_white.ldr_limit_min & 0x00FF);
      Serial.write((settings_white.ldr_limit_min >> 8) & 0x00FF);
      Serial.write( settings_white.ldr_limit_max & 0x00FF);
      Serial.write((settings_white.ldr_limit_max >> 8) & 0x00FF);
      Serial.write(0xAA);
      break;

    // send brightness now red
    case 5:
      Serial.write(0xF6);
      Serial.write(brightness_now_red);
      Serial.write(0xAA);
      break;

    // send brightness now white
    case 6:
      Serial.write(0xF7);
      Serial.write(brightness_now_white);
      Serial.write(0xAA);
      break;

    // send time struct manual
    case 7:
      Serial.write(0xF3);
      Serial.write(0x00);
      Serial.write( timeinfo.tm_min);
      Serial.write( timeinfo.tm_hour);
      Serial.write( timeinfo.tm_mday);
      Serial.write( timeinfo.tm_mon + 1);
      Serial.write( timeinfo.tm_year - 100);
      Serial.write( timeinfo.tm_wday);
      Serial.write(0xAA);
      break;

    // send time struct ntp
    case 8:
      Serial.write(0xF3);
      Serial.write( timeinfo.tm_sec);
      Serial.write( timeinfo.tm_min);
      Serial.write( timeinfo.tm_hour);
      Serial.write( timeinfo.tm_mday);
      Serial.write( timeinfo.tm_mon + 1);
      Serial.write( timeinfo.tm_year - 100);
      Serial.write( timeinfo.tm_wday);
      Serial.write(0xAA);
      break;

    // request ldr data
    case 9:
      Serial.write(0xF1);
      Serial.write(0xAA);
      break;

  }
}

void checkSerial()
{
  if(Serial.available() > 0)
  {
    uint8_t byte = Serial.read();

    switch (serial_case)
    {
      case 0xA0:  
        serial_case = byte;
        serial_byte_counter = 0;
        break;

      case 0xF1:
        if(serial_byte_counter < 2)
        {
          buffer[serial_byte_counter] = byte;
        }
        else
        {
          if(byte == 0xAA)
          {
            ldr = buffer[0] & 0xFF;
            ldr |= (buffer[1] << 8) & 0xFF00;
            serial_case = 0xA0;
          }
          else
          {
            Serial.write(0xFF);
          }
        }
        serial_byte_counter++;
        break;

      default:
        // Serial.write(0xFF);
        serial_case = 0xA0;
        break;
    }
  }

}