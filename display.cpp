#include <Wire.h>
#include "Arduino.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "bq769x0.h"    // Library for Texas Instruments bq76920 battery management IC

#include "globals.h"
#include "declares.h"
#include "display.h"
#include "settings.h"

void PrintMenu()
{
  Serial.println("1: Device Status");
  Serial.println("2: Voltage Reading");
  Serial.println("3: ...");
  Serial.println("4: ...");
  Serial.println("cs: Check Status");
  Serial.println("rs: ReScan for Devices");
  Serial.println("rb: ReBoot BMS");
}

void setupDisplay()
{
#if RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
#endif // RST_PIN >= 0

  oled.setFont(Adafruit5x7);
}

void oled_loop()
{
  // Control what is currently being displayed
  //
  oledDisplay = DISPLAYSTATUS;
  switch (oledDisplay)
  {
    case DISPLAYMENU:
      displayMenu(); // Displays the menu on screen for user to select option
      break;
    case DISPLAYSTATUS:
      displayStatus(); // Displays the status in intervals of 'displayStatusRefresh'
      break;
    case DISPLAYVOLTAGES:
      break;
    default:
      oledDisplay = DISPLAYSTATUS;
      break;
  }
}

void displayMenu()
{
  oled.println("1: Device Status");
  oled.println("2: Voltage Reading");
  oled.println("3: ...");
  oled.println("4: ...");
  oled.println("r: ReScan for Devices");
}

void displayStatus()
{
  static long previousTimer;
  unsigned long currentTimer = millis();
  static uint8_t currentDisplay;

  if (currentTimer - previousTimer > settings.displayStatusRefresh) // rebuild the picture after some delay
  {
    previousTimer = currentTimer;
    oled.clear();
    oled.set1X();
    oled.println("     System Status");
    switch (currentDisplay)
    {
      case 1:
        displaySysStatus();
        break;
      case 2:
        displayVoltage();
        break;
      case 3:
        //        displayCondition();
        break;
      case 4:
        break;
      default:
        currentDisplay = 0;
        break;
    }
    currentDisplay += 1;
  }
}

void printScreen(uint8_t x, uint8_t y, String input)
{
  char *buf;
  input.toCharArray(buf, input.length());
  //  oled.drawStr( x, y, buf);
}

void displaySysStatus()
{
  //  oled.println("Hello world!");
  //  oled.println("A long line may be truncated");
  //  oled.println();
  //  oled.set2X();
  //  oled.println("2X demo");
  //  oled.set1X();

  oled.println( "SS not available");
}

void displayVoltage()
{
  //  oled.println( "Cell Voltages:");
  //  for (int i = 0; i < 4; i++) {
  //    bms.vc_hi[i] = readFrom(deviceAddress, VC_HI[i], 1);
  //    bms.vc_lo[i] = readFrom(deviceAddress, VC_LO[i], 1);
  //  }

  oled.println( "1.10 | 2.20 | 3.30");
  oled.println( "4.40");
}

void output_oled()    // OLED SSD1306
{
  int balancingStatus = 0; //getBalancingStatus();

  //    i2c.frequency(400000);
  oled.clear();
  /*
      oled.cursor(0, 0);
      oled.printf("%.2f V", getBatteryVoltage()/1000.0);
      oled.cursor(64, 0);
      oled.printf("%.2f A", getBatteryCurrent()/1000.0);

      oled.cursor(0, 8);
      oled.printf("T:%.1f C", getTemperatureDegC(1));
      oled.cursor(64, 8);
      oled.printf("SOC:%.2f", getSOC());

      oled.cursor(0, 16);
      oled.printf("Load: %.2fV", load_voltage/1000.0);

      for (int i = 0; i < getNumberOfCells(); i++) {
          if (blinkOn || !(balancingStatus & (1 << i))) {
              oled.cursor((i % 2 == 0) ? 0 : 64, 24 + (i / 2) * 8);
              oled.printf("%d:%.3f V", i+1, getCellVoltage(i+1)/1000.0);
          }
      }

      oled.display();
  */
  /*
    oled.print(getBatteryVoltage() / 1000.0);
    oled.println(" V");
    oled.print(getBatteryCurrent() / 1000.0);
    oled.println(" A");
    oled.print("T: ");
    oled.print(getTemperatureDegC(1));
    oled.println(" C");
    oled.print("SOC: ");
    oled.println(getSOC());
    oled.print("Load: ");
    oled.print(load_voltage / 1000.0);
    oled.println(" V");

    for (int i = 0; i < getNumberOfCells(); i++) {
    if (blinkOn || !(balancingStatus & (1 << i))) {
      //oled.cursor((i % 2 == 0) ? 0 : 64, 24 + (i / 2) * 8);
      if ( i % 2 == 0 )
      { // Left column
        oled.print(i + 1);
        oled.print(": ");
        oled.print(getCellVoltage(i + 1) / 1000.0);
        oled.print(" V");
      }
      else // Right column
      {
        oled.print("   ");
        oled.print(i + 1);
        oled.print(": ");
        oled.print(getCellVoltage(i + 1) / 1000.0);
        oled.print(" V");
      }
    }
    oled.println();
    }
    oled.println();
  */
}
