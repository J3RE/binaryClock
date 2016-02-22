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

#include <avr/io.h> 
#include <avr/interrupt.h>
#include <Wire.h>
#include <RealTimeClockDS1307_modified.h>

#define LED_PORT_ANODE PORTB  // 0 - 3 anode of LEDs
#define LED_PORT_CATHODE PORTC  // 0 - 2 cathode of LEDs
#define DDR_LED_PORT_ANODE DDRB
#define DDR_LED_PORT_CATHODE DDRC
#define MASK_LED_PORT_ANODE 0x0F
#define MASK_LED_PORT_CATHODE 0x07

#define BUTTON 3
#define PWR_ESP 4
#define BLUE_LED 5
#define STATUS_LED 7
#define LDR A3

#define STATUS_LED_PIN  PIND
#define STATUS_LED_MASK 0x80;

#define SQW 2
#define SQW_INTERRUPT 0

#define ESP_TIMEOUT 300000UL // 5 min
#define SERIAL_TIMEOUT  2000U // 2 sec
#define BUTTON_POLL_TIME  8 // 8 ms
#define BUTTON_AP_MODE_TIME  1500U // 1.5 s
#define FAILS_MAX 1

struct tm
{
  uint8_t tm_sec;   // seconds after the minute  0-59
  uint8_t tm_min;   // minutes after the hour  0-59
  uint8_t tm_hour;  // hours since midnight  0-23
  uint8_t tm_mday;  // day of the month  1-31
  uint8_t tm_mon;   // months since January  0-11
  uint8_t tm_year;  // years since 1900  
  uint8_t tm_wday;  // days since Sunday 0-6
} timeinfo;

struct settings
{
  uint8_t min;
  uint8_t max;
  uint16_t ldr_limit_min;
  uint16_t ldr_limit_max;
} settings_red, settings_white;

// mode 0 esp webserver
// mode 1 esp ntp
// mode 2 esp get eeprom stuff
// mode 3 esp off
uint8_t mode;

// data_leds[0] first layer (1 - 8 hour)
// data_leds[1] third layer (1 - 8 minute)
// data_leds[2] second layer (16 hour, 16 - 32 minute)
uint8_t data_leds[3];

uint8_t update = 1;
uint8_t brightness_timeout = 0;
uint8_t second_old, brightness_white;
volatile uint8_t second = 0;
volatile uint8_t layer = 0;
volatile uint8_t brightness_red, brightness_counter;

uint8_t button_history = 0xFF;
uint8_t button_pressed, led;
uint16_t ldr_now, ldr_average;

uint8_t use_ntp, start_in_ap_mode, serial_case, serial_byte_counter, fails, start_reading_serial;
uint8_t buffer[7];

// timing data
unsigned long millis_serial, millis_webserver, millis_led, millis_button, millis_button_pressed;


void setup()
{
  pinMode(PWR_ESP, OUTPUT);
  digitalWrite(PWR_ESP, HIGH); // turn off

  // Timer2 configuration
  TCCR2A = 0x00;
  // set prescaler to 0
  TCCR2B = (1 << CS20);
  // count to 127, than trigger interrupt
  OCR2A = 127;
  TIMSK2 = (1 << OCIE2A);
  
  // set SQW-pin as input with internal pullup resistor
  pinMode(SQW, INPUT);
  digitalWrite(SQW, HIGH);
  // initialise RTC
  RealTimeClockDS1307();
  
  // RTC-interrupt every second
  attachInterrupt(SQW_INTERRUPT, sqwInterrupt, FALLING);
  
  // set the LED-pins as output
  DDR_LED_PORT_ANODE |= MASK_LED_PORT_ANODE;
  DDR_LED_PORT_CATHODE |= MASK_LED_PORT_CATHODE;

  // set button as input with internal pullup resistor
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH);

  // set status-led as output
  pinMode(STATUS_LED, OUTPUT);

  // adjust brightness
  ldr_average = analogRead(LDR);
  updateBrightness();

  // enable interrupts
  sei();

  displayNumbers(0x1F, 0x3F);
  delay(500);

  changeMode(2);
}

void loop() 
{
  if(!second && update)
  {
    update = 0;
    getTime();
  }
  else if(second == 59)
  {
    update = 1;
  }

  if(mode != 3)
  {
    checkSerial();
  }

  if(use_ntp && (mode == 3) && (timeinfo.tm_hour == 3) && (timeinfo.tm_min == 0) && (second == 10))
  {
    changeMode(1);
  }

  if( ((second % 3) == 0) && (second != second_old))
  {
    updateBrightness();
    second_old = second;
  }

  handleButton();
 
  ledFails(0);
}

ISR(TIMER2_COMPA_vect)
{
  // cycle through 256 brightness levels, then move to the next layer
  brightness_counter++;
  if(!brightness_counter)
    (layer == 2) ? layer = 0 : layer++;

  LED_PORT_ANODE &= ~MASK_LED_PORT_ANODE;
  if (brightness_red > brightness_counter)
  {
    LED_PORT_CATHODE &= ~MASK_LED_PORT_CATHODE;
    LED_PORT_ANODE |= data_leds[layer];   
    LED_PORT_CATHODE |= 1 << layer;
  }
}
