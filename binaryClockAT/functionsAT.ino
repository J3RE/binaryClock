void sqwInterrupt()
{
  second = ++second % 60;
}

void animateBrightness()
{ 
  if((millis() - millis_animate) >= DELAY_BRIGHTNESS)
  {
    if(second % 2)
    {
      if(brightness_red_now < (brightness_red + (DELTA_BRIGHTNESS << 1)))
        brightness_red_now += 2;

      if(brightness_white_now > brightness_white)
        brightness_white_now--;
    }
    else
    {
      if(brightness_red_now > brightness_red)
        brightness_red_now -= 2;
      
      if(brightness_white_now < (brightness_white + DELTA_BRIGHTNESS))
        brightness_white_now++;
    }

    brightness_red_log = log_brightness_9bit[brightness_red_now];
    brightness_white_log = log_brightness[brightness_white_now];
    analogWrite(WHITE_LED, brightness_white_log);

    millis_animate = millis();
  }
}

void displayNumbers(uint8_t left, uint8_t right)
{
  data_leds[0] = (left & 0x0F);
  data_leds[1] = (right & 0x0F);
  data_leds[2] = (((left >> 4) & 0x01) | ((right >> 3) & 0x06));
}

// write new time to the RTC-module
void setRTC()
{  
  RTC.setSeconds(timeinfo.tm_sec);
  RTC.setMinutes(timeinfo.tm_min);
  RTC.setHours(timeinfo.tm_hour);
  RTC.setDayOfWeek(timeinfo.tm_wday);
  RTC.setDate(timeinfo.tm_mday);
  RTC.setMonth(timeinfo.tm_mon);
  RTC.setYear(timeinfo.tm_year - 100);
  
  second = timeinfo.tm_sec;
  RTC.setClock();
}

// read the time from the RTC-module
void getTime()
{
  RTC.readClock();
  timeinfo.tm_hour = RTC.getHours();
  timeinfo.tm_min = RTC.getMinutes();
  second = RTC.getSeconds();
  displayNumbers(timeinfo.tm_hour, timeinfo.tm_min);
}

// power on/off the ESP8266
void changeMode(uint8_t x)
{
  mode = x;

  if(mode == 3)
  {
    digitalWrite(PWR_ESP, HIGH); // turn off
    Serial.end();
    getTime();
  }
  else
  {
    digitalWrite(STATUS_LED, LOW);
    digitalWrite(PWR_ESP, LOW); // turn on
    fails = 0;
    start_reading_serial = 0;
    serial_case = 0x00;
    millis_webserver = millis();
    millis_serial = millis();

    Serial.begin(115200);
  }
}

// read ldr and adjust brightness
void updateBrightness()
{
  uint16_t ldr_temp = 0;
  for(int i = 0; i < 4; i++)
    ldr_temp += analogRead(LDR);

  ldr_now = ldr_temp >> 2;

  ldr_average = ldr_now/5 + ldr_average * 4/5;

  if (!brightness_timeout)
  {
    if(ldr_average > settings_red.ldr_limit_max)
    {
      brightness_red = settings_red.max << 1;
    }
    else if(ldr_average < settings_red.ldr_limit_min)
    {
      brightness_red = settings_red.min << 1;
    }
    else
    {
      brightness_red = map(ldr_average, settings_red.ldr_limit_min, settings_red.ldr_limit_max, settings_red.min, settings_red.max) << 1;
    }

    if(ldr_average > settings_white.ldr_limit_max)
    {
      brightness_white = settings_white.max;
    }
    else if(ldr_average < settings_white.ldr_limit_min)
    {
      brightness_white = settings_white.min;
    }
    else
    {
      brightness_white = map(ldr_average, settings_white.ldr_limit_min, settings_white.ldr_limit_max, settings_white.min, settings_white.max);
    }
  }
  else
  {
    brightness_timeout--;
  }
}

// handle the communication with the ESP8266
void checkSerial()
{
  if( (millis() - millis_webserver) > ESP_TIMEOUT)
  {
    ledFails(1);
  }

  if((serial_case != 0xA0) && ((millis() - millis_serial) > SERIAL_TIMEOUT))
  {
    ledFails(1);
  }

  if(Serial.available() > 0)
  {

    uint8_t byte = Serial.read();

    if (start_reading_serial)
    {

      switch (serial_case)
      {
        case 0xA0:  
          serial_case = byte;
          millis_serial = millis();
          serial_byte_counter = 0;
          break;

        case 0x01:
          if(byte == 0xAA)
          {
            serial_case = 0xA0;
          }
          else
          {
            ledFails(1);
          }
          break;

        case 0xF2:
          if(serial_byte_counter < 1)
          {
            buffer[serial_byte_counter] = byte;
          }
          else
          {
            if(byte == 0xAA)
            {
              use_ntp = buffer[0];
              serial_case = 0xA0;
            }
            else
            {
              ledFails(1);
            }
          }
          serial_byte_counter++;
          break;

        case 0xF3:
          if(serial_byte_counter < 7)
          {
            buffer[serial_byte_counter] = byte;
          }
          else
          {
            if(byte == 0xAA)
            {
              timeinfo.tm_sec = buffer[0];
              timeinfo.tm_min = buffer[1];
              timeinfo.tm_hour = buffer[2];
              timeinfo.tm_mday = buffer[3];
              timeinfo.tm_mon = buffer[4];
              timeinfo.tm_year = buffer[5];
              timeinfo.tm_wday = buffer[6];
              setRTC();
              displayNumbers(timeinfo.tm_hour, timeinfo.tm_min);
              serial_case = 0xA0;
            }
            else
            {
              ledFails(1);
            }
          }
          serial_byte_counter++;
          break;

        case 0xF4:
          if(serial_byte_counter < 6)
          {
            buffer[serial_byte_counter] = byte;
          }
          else
          {
            if(byte == 0xAA)
            {
              settings_red.min = buffer[0];
              settings_red.max = buffer[1];
              settings_red.ldr_limit_min = buffer[2];
              settings_red.ldr_limit_min |= buffer[3] << 8;
              settings_red.ldr_limit_max = buffer[4];
              settings_red.ldr_limit_max |= buffer[5] << 8;
              serial_case = 0xA0;
            }
            else
            {
              ledFails(1);
            }
          }
          serial_byte_counter++;
          break;

        case 0xF5:
          if(serial_byte_counter < 6)
          {
            buffer[serial_byte_counter] = byte;
          }
          else
          {
            if(byte == 0xAA)
            {
              settings_white.min = buffer[0];
              settings_white.max = buffer[1];
              settings_white.ldr_limit_min = buffer[2];
              settings_white.ldr_limit_min |= buffer[3] << 8;
              settings_white.ldr_limit_max = buffer[4];
              settings_white.ldr_limit_max |= buffer[5] << 8;
              serial_case = 0xA0;
            }
            else
            {
              ledFails(1);
            }
          }
          serial_byte_counter++;
          break;

        case 0xF6:
          if(serial_byte_counter < 1)
          {
            buffer[serial_byte_counter] = byte;
          }
          else
          {
            if(byte == 0xAA)
            {
              brightness_red = buffer[0] << 1;
              brightness_timeout = 9;
              serial_case = 0xA0;
            }
            else
            {
              ledFails(1);
            }
          }
          serial_byte_counter++;
          break;
          
        case 0xF7:
          if(serial_byte_counter < 1)
          {
            buffer[serial_byte_counter] = byte;
          }
          else
          {
            if(byte == 0xAA)
            {
              brightness_white = buffer[0];
              brightness_timeout = 9;
              serial_case = 0xA0;
            }
            else
            {  
              ledFails(1);
            }
          }
          serial_byte_counter++;
          break;

        default:
          ledFails(1);
          break;
      }

      switch (serial_case)
      {
        case 0xF0:
          Serial.write(mode);
          serial_case = 0x01;
          break;
          
        case 0xF1:
          Serial.write(0xF1);
          brightness_timeout = 0;
          Serial.write(ldr_now & 0xFF);
          Serial.write((ldr_now >> 8) & 0xFF);
          Serial.write(0xAA);
          serial_case = 0x01;
          break;

        case 0xFA:
          changeMode(3);
          break;
          
        case 0xFB:
          millis_webserver = millis();
          serial_case = 0x01;
          break;
          
        case 0xFC:
          Serial.write(start_in_ap_mode);
          serial_case = 0x01;
          break;

        case 0xA0:
        case 0x01:
        case 0xF2:
        case 0xF3:
        case 0xF4:
        case 0xF5:
        case 0xF6:
        case 0xF7:
          break;

        default:
          ledFails(1);
          break;
      }
    }
    else if (byte == 0x5A)
    {
      serial_byte_counter++;
      if(serial_byte_counter > 4)
      {
        start_reading_serial = 1;
        millis_webserver = millis();
        Serial.write(0xAA);
        serial_case = 0x01;
      }
    }

  }
}

void ledFails(uint8_t error)
{
  // check for fails
  if (error)
  {
    if(++fails >= FAILS_MAX)
    {
      changeMode(3);
    }
    else
    {
      serial_case = 0xA0;
    }
  }

  // blink LED, while the webserver is running and change the duty cycle if an error occurred
  if(mode != 3)
  {
    if(!fails)
    {
      if(led && ((millis() - millis_led) >= 100))
      {
        digitalWrite(STATUS_LED, LOW);
        led = !led;
        millis_led = millis();
      }
      else if(!led && ((millis() - millis_led) >= 500))
      {
        digitalWrite(STATUS_LED, HIGH);
        led = !led;
        millis_led = millis();
      }
    }
    else
    {
      if(led && ((millis() - millis_led) >= 500))
      {
        digitalWrite(STATUS_LED, LOW);
        led = !led;
        millis_led = millis();
      }
      else if(!led && ((millis() - millis_led) >= 100))
      {
        digitalWrite(STATUS_LED, HIGH);
        led = !led;
        millis_led = millis();
      }
    }
  }
  else
  {
    if(fails > 0)
      digitalWrite(STATUS_LED, HIGH);
    else
      digitalWrite(STATUS_LED, LOW);
  }
}

void handleButton()
{
  if( (millis() - millis_button) >= BUTTON_POLL_TIME)
  {
    // clever debouncing stuff
    button_history = button_history << 1;
    button_history |= digitalRead(BUTTON);

    // button pressed
    if ((button_history & 0b11000111) == 0b11000000)
    { 
      millis_button_pressed = millis();
      start_in_ap_mode = 0;
      button_history = 0b00000000;
    }    

    // button pressed for longer than BUTTON_AP_MODE_TIME
    if(button_history == 0b00000000)
    {
      if((millis() - millis_button_pressed) >= BUTTON_AP_MODE_TIME)
      {
        start_in_ap_mode = 1; 
        if(mode == 3)
        {
          changeMode(0);
        }
      }
    }

    // button released
    if ((button_history & 0b11000111) == 0b00000111)
    { 
      if(!start_in_ap_mode)
      {
        if(mode == 3)
        {
          changeMode(0);
        }
        else
        {
          changeMode(3);
        }
      }  
      button_history = 0b11111111;
    }

    millis_button = millis();
  }
}