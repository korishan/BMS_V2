#ifndef DISPLAY_H
#define DISPLAY_H
#include "SSD1306AsciiWire.h"

// Display Constants
const uint8_t DISPLAYMENU = 1;
const uint8_t DISPLAYSTATUS = 2;
const uint8_t DISPLAYVOLTAGES = 3;

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

extern uint8_t oledDisplay;

extern SSD1306AsciiWire oled;

// Adafruit
//#include <SPI.h>
//#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
//
//#define OLED_RESET 4
//extern Adafruit_SSD1306 display(OLED_RESET);

#endif

