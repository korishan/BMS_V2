#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

#include "globals.h"
#include "declares.h"
#include "display.h"
#include "settings.h"

#include "bq769x0.h"    // Library for Texas Instruments bq76920 battery management IC

SSD1306AsciiWire oled;
userSettings settings;

long internalVCC; // A global variable for storing the internal voltage reference of the 1.1V rail of the chip.

void setup()
{
//  internalVCC = readVcc(); // A global variable for storing the internal voltage reference of the 1.1V rail of the chip.
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000L);

  int err = bmsBegin();

  setupDisplay();
  setDefaults();
  PrintMenu();
}

void loop() {
  unsigned long currentTimer = millis();
  static unsigned long bmsUpdateLast;
  static unsigned long serialLastUpdate;
  static long previousBlink;
  static bool blinkOn;

  if (currentTimer - bmsUpdateLast > settings.bmsRefreshInt)
  {
    bmsUpdate();  // should be called at least every 250 ms
    bmsUpdateLast = currentTimer;
  }
    if (currentTimer - previousBlink > 1000)
  {
    if (blinkOn == false) 
    {
      digitalWrite(D2, HIGH);
    }
    else if (blinkOn == true)
    {
      digitalWrite(D2, LOW);
    }
    blinkOn = !blinkOn;
  }
  if (currentTimer - serialLastUpdate > settings.displayStatusRefresh)
  {
    //BMS.printRegisters();
    Serial.print(F("Cell Voltages: "));
    for (int i = 1; i <= 5; i++)
    {
      Serial.print(getCellVoltage(i));
      Serial.print(F(" "));
    }
    Serial.println();
    serialLastUpdate = currentTimer;
  }
  readSerial(); // Check serial input for commands
}

void setDefaults()
{
  setBMSDefaults();

  settings.displayStatusRefresh = 5000;
  settings.bmsRefreshInt = 250; // Later we will pull this from a user stored variable
  settings.oledDisplay = DISPLAYSTATUS;
}

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

//long readVcc() {
//  // Read 1.1V reference against AVcc
//  // set the reference to Vcc and the measurement to the internal 1.1V reference
//#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
//  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
//#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
//  ADMUX = _BV(MUX5) | _BV(MUX0);
//#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
//  ADMUX = _BV(MUX3) | _BV(MUX2);
//#else
//  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
//#endif
//
//  delay(2); // Wait for Vref to settle
//  ADCSRA |= _BV(ADSC); // Start conversion
//  while (bit_is_set(ADCSRA, ADSC)); // measuring
//
//  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
//  uint8_t high = ADCH; // unlocks both
//
//  long result = (high << 8) | low;
//
//  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
//  return result; // Vcc in millivolts
//}
