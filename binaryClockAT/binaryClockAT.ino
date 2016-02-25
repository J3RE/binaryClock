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
#define WHITE_LED 5
#define STATUS_LED 7
#define LDR A3

#define SQW 2
#define SQW_INTERRUPT 0

#define ESP_TIMEOUT           300000UL // 5 min
#define SERIAL_TIMEOUT        2000U    // 2 sec
#define BUTTON_POLL_TIME      8        // 8 ms
#define BUTTON_AP_MODE_TIME   1500U    // 1.5 s
#define DELAY_BRIGHTNESS      10       // 10 ms
#define DELTA_BRIGHTNESS      20       // 20 brightness levels
#define FAILS_MAX             1        // shutdown ESP8266 after the first fail

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
uint16_t brightness_red_now = 0;
uint16_t brightness_red;
uint8_t brightness_white_now = 0;
uint8_t  brightness_white, brightness_white_log, second_old, animate_seconds;
volatile uint8_t second = 0;
volatile uint8_t layer = 0;
volatile uint16_t brightness_counter = 0;
volatile uint16_t brightness_red_log;

uint8_t button_history = 0xFF;
uint8_t button_pressed, led;
uint16_t ldr_now, ldr_average;

uint8_t use_ntp, start_in_ap_mode, serial_case, serial_byte_counter, fails, start_reading_serial;
uint8_t buffer[7];

// timing data
unsigned long millis_serial, millis_webserver, millis_led, millis_button, millis_button_pressed, millis_animate;

const uint8_t log_brightness[256] = {
  0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
  2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
  2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
  4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,
  5,   5,   5,   6,   6,   6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,
  8,   8,   8,   8,   8,   8,   9,   9,   9,   9,  10,  10,  10,  10,  10,  11,
 11,  11,  11,  12,  12,  12,  12,  13,  13,  13,  14,  14,  14,  15,  15,  15,
 16,  16,  16,  17,  17,  17,  18,  18,  19,  19,  20,  20,  20,  21,  21,  22,
 22,  23,  23,  24,  24,  25,  26,  26,  27,  27,  28,  29,  29,  30,  30,  31,
 32,  33,  33,  34,  35,  36,  36,  37,  38,  39,  40,  41,  41,  42,  43,  44,
 45,  46,  47,  48,  49,  51,  52,  53,  54,  55,  56,  58,  59,  60,  62,  63,
 64,  66,  67,  69,  70,  72,  73,  75,  77,  78,  80,  82,  84,  86,  87,  89,
 91,  93,  95,  98, 100, 102, 104, 106, 109, 111, 114, 116, 119, 121, 124, 127,
130, 132, 135, 138, 141, 144, 148, 151, 154, 158, 161, 165, 168, 172, 176, 180,
184, 188, 192, 196, 200, 205, 209, 214, 219, 223, 228, 233, 238, 244, 249, 254};

const uint16_t log_brightness_9bit[512] = {
  0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
  2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
  2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,
  4,   4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,
  5,   5,   5,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,
  7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,   8,
  8,   8,   8,   8,   8,   9,   9,   9,   9,   9,   9,   9,   9,  10,  10,  10,
 10,  10,  10,  10,  10,  11,  11,  11,  11,  11,  11,  11,  12,  12,  12,  12,
 12,  12,  12,  13,  13,  13,  13,  13,  13,  14,  14,  14,  14,  14,  15,  15,
 15,  15,  15,  15,  16,  16,  16,  16,  16,  17,  17,  17,  17,  18,  18,  18,
 18,  18,  19,  19,  19,  19,  20,  20,  20,  20,  21,  21,  21,  21,  22,  22,
 22,  23,  23,  23,  23,  24,  24,  24,  25,  25,  25,  26,  26,  26,  26,  27,
 27,  27,  28,  28,  29,  29,  29,  30,  30,  30,  31,  31,  32,  32,  32,  33,
 33,  34,  34,  34,  35,  35,  36,  36,  37,  37,  37,  38,  38,  39,  39,  40,
 40,  41,  41,  42,  42,  43,  43,  44,  45,  45,  46,  46,  47,  47,  48,  49,
 49,  50,  50,  51,  52,  52,  53,  54,  54,  55,  56,  56,  57,  58,  58,  59,
 60,  61,  61,  62,  63,  64,  64,  65,  66,  67,  68,  69,  69,  70,  71,  72,
 73,  74,  75,  76,  77,  78,  78,  79,  80,  81,  82,  83,  84,  86,  87,  88,
 89,  90,  91,  92,  93,  94,  96,  97,  98,  99, 100, 102, 103, 104, 105, 107,
108, 109, 111, 112, 113, 115, 116, 118, 119, 121, 122, 124, 125, 127, 128, 130,
131, 133, 135, 136, 138, 140, 141, 143, 145, 147, 148, 150, 152, 154, 156, 158,
160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 181, 183, 185, 187, 190, 192,
194, 197, 199, 202, 204, 207, 209, 212, 214, 217, 220, 222, 225, 228, 231, 233,
236, 239, 242, 245, 248, 251, 254, 257, 261, 264, 267, 270, 274, 277, 281, 284,
287, 291, 295, 298, 302, 306, 309, 313, 317, 321, 325, 329, 333, 337, 341, 345,
350, 354, 358, 363, 367, 372, 376, 381, 385, 390, 395, 400, 405, 410, 415, 420,
425, 430, 436, 441, 446, 452, 457, 463, 469, 474, 480, 486, 492, 498, 504, 511};

void setup()
{
  pinMode(PWR_ESP, OUTPUT);
  digitalWrite(PWR_ESP, HIGH); // turn off

  // Timer2 configuration
  // 16 MHz Clock, no prescaler, count to 128, 512 brightness levels, 3 layers
  // 16 MHz    / 128 = 125    kHz ...interruptfrequenzy 
  // 125 kHz   / 512 = 244.14 Hz
  // 244.14 Hz / 3   = 81.4   Hz  ...update rate of red LED's

  // CTC mode
  TCCR2A = (1 << WGM21);
  // set prescaler to 0
  TCCR2B = (1 << CS20);
  // count to 128, than trigger interrupt
  OCR2A = 128;
  TIMSK2 = (1 << OCIE2A);

  // set SQW-pin as input with internal pullup resistor
  pinMode(SQW, INPUT);
  digitalWrite(SQW, HIGH);
  // initialise RTC
  RealTimeClockDS1307();
  
  // set the LED-pins as output
  DDR_LED_PORT_ANODE |= MASK_LED_PORT_ANODE;
  DDR_LED_PORT_CATHODE |= MASK_LED_PORT_CATHODE;

  // set button as input with internal pullup resistor
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH);

  // set status-led as output
  pinMode(STATUS_LED, OUTPUT);

  // enable interrupts
  sei();

  // set settings manually, in case sync with esp fails
  settings_red.min = 50;
  settings_red.max = 255;
  settings_red.ldr_limit_min = 30;
  settings_red.ldr_limit_max = 500;
  settings_white.min = 50;
  settings_white.max = 255;
  settings_white.ldr_limit_min = 30;
  settings_white.ldr_limit_max = 500;


  // initialize ldr
  ldr_average = analogRead(LDR);
  updateBrightness();
  brightness_red_log = log_brightness[brightness_red];
  brightness_white_log = log_brightness[brightness_white];

  displayNumbers(0x1F, 0x3F);
  delay(500);

  changeMode(2);

  // RTC-interrupt every second
  attachInterrupt(SQW_INTERRUPT, sqwInterrupt, FALLING);
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

  if( second != second_old)
  {
    updateBrightness();
    second_old = second;
  }

  animateBrightness();

  handleButton();
 
  ledFails(0);
}

ISR(TIMER2_COMPA_vect)
{
  // cycle through 512 brightness levels, then move to the next layer
  LED_PORT_ANODE &= ~MASK_LED_PORT_ANODE;
  if(++brightness_counter == 512)
  {
    LED_PORT_CATHODE &= ~MASK_LED_PORT_CATHODE;
    (layer == 2) ? layer = 0 : layer++;
    LED_PORT_CATHODE |= 1 << layer;
    brightness_counter = 0;
  }

  if (brightness_red_log > brightness_counter)
  {
    LED_PORT_ANODE |= data_leds[layer];   
  }
}
