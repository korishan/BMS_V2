#include <Arduino.h>
#include "globals.h"
#include "declares.h"
#include "display.h"
#include "settings.h"

const byte numChars = 32;
boolean newData;
uint8_t oledDisplayStatus;

void readSerial()
{
  char strCmd[32];
  char receivedChars[numChars]; // an array to store the received data
  char strRemaining[32];
  int intValue[5] = {0};
  char strVariable[16];
  char *buf;

  recvWithEndMarker(receivedChars);
  if (newData == true)
  {
    Serial.print(F("Received Command: "));
    Serial.println(receivedChars);
    sscanf(receivedChars, "%s %s", strCmd, strVariable);
    oled.clear();
    oled.set1X();


    switch (strCmd[0])
    {
      case '1':
        oledDisplayStatus = DISPLAYMENU;
        //        get_bms_status();
        break;
      case '2':
        oledDisplayStatus = DISPLAYVOLTAGES;
        //get_bms_voltages(bms.address);
        break;
      case '3':
        Serial.println(F("Not implimented yet."));
        break;
      case 'c':
        if (strcmp(strCmd, "cs") == 0)
          Serial.println(checkStatus());
        break;
      case 'h':
        display_help(true);
        break;
      case 'r':
        if (strcmp(strCmd, "rs") == 0)
          SearchDevices();
        else if (strcmp(strCmd, "rb") == 0)
          RebootDevice();
        break;
      case 'g':
        switch (strCmd[1])
        {
          case 'v':
            if (strcmp(strVariable, "disp") == 0)
              Serial.println(settings.displayStatusRefresh);
            else if (strcmp(strVariable, "bms") == 0)
              Serial.println(settings.bmsRefreshInt);
            else if (receivedChars[2] == 1)
              Serial.println(F("Option not available"));
            break;
        }
        if (strcmp(strCmd, "get") == 0) // Setting Variables
        {
          if (strcmp(strVariable, "dispRefresh") == 0)
          {
            Serial.print(F( "Display Refresh rate "));
            Serial.print(settings.displayStatusRefresh);
          }
          if (strcmp(strVariable, "SRV") == 0) // Shunt Resistor Value
          {
            Serial.print(F("Shunt Resistor Value "));
            Serial.print(getShuntResistorValue());
          }
          if (strcmp(strVariable, "TBV") == 0) // Thermistor Beta Value
          {
            Serial.print(F( "Thermistor Beta Value "));
            Serial.print(getThermistorBetaValue());
          }
          if (strcmp(strVariable, "TL") == 0) // Temp Limits
          {
            Serial.println(F( "Temperature Limits:"));
            for (int i = 0; i < 4; i++) {
              Serial.print(getTemperatureLimits(i));
              if (i < 3) Serial.print(F(", "));
            }
          }
          if (strcmp(strVariable, "SCP") == 0) // Short Circuit Protection
          {
            Serial.print(F( "Short Circuit Protection "));
            Serial.print(intValue[0]);
            Serial.print(F( "', '"));
            Serial.print(intValue[1]);
            //setShortCircuitProtection(intValue[0], intValue[1]);
          }
          if (strcmp(strVariable, "OCP") == 0) // Over Current Charge Protection
          {
            Serial.print(F( "Overcurrent Charge Protection "));
            Serial.print(getOvercurrentChargeProtectionCurrent());
            Serial.print(F( ", "));
            Serial.print(getOvercurrentChargeProtectionDelay());
          }
          if (strcmp(strVariable, "ODP") == 0) // Over Current Discharge Protection
          {
            Serial.print(F( "Overcurrent Discharge Protection "));
            Serial.print(getOvercurrentDischargeProtectionCurrent());
            Serial.print(F( ", "));
            Serial.print(getOvercurrentDischargeProtectionDelay());
          }
          if (strcmp(strVariable, "CUP") == 0) // Cell Undervoltage Protection
          {
            Serial.print(F( "Cell Undervoltage Protection "));
            Serial.print(getCellUndervoltageProtectionVoltage());
            Serial.print(F( ", "));
            Serial.print(getCellUndervoltageProtectionDelay());
          }
          if (strcmp(strVariable, "COP") == 0) // Cell Overvoltage Protection
          {
            Serial.print(F( "Cell Overvoltage Protection "));
            Serial.print(getCellOvervoltageProtectionVoltage());
            Serial.print(F( ", "));
            Serial.print(getCellOvervoltageProtectionDelay());
          }
          if (strcmp(strVariable, "BT") == 0) // Balancing Threshold
          {
            Serial.print(F( "Balancing Threshold "));
            Serial.print(getBalancingThresholdTime());
            Serial.print(F( ", "));
            Serial.print(getBalancingThresholdVoltage());
            Serial.print(F( ", "));
            Serial.print(getBalancingThresholdDifference());
          }
          if (strcmp(strVariable, "ICT") == 0) // Idle Current Threshold
          {
            Serial.print(F( "Idle Current Threshold "));
            Serial.print(getIdleCurrentThreshold());
          }
        }
        break;
      case 's':
        if (strcmp(strCmd, "set") == 0) // Setting Variables
        {
          if (strcmp(strVariable, "dispRefresh") == 0)
          {
            sscanf(receivedChars, "%s %s %d",
                   strCmd, strVariable, intValue[0]);
            Serial.print(F( "Changed Display Refresh rate from '"));
            Serial.print(settings.displayStatusRefresh);
            Serial.print(F( "' to '"));
            settings.displayStatusRefresh = intValue[0];
            Serial.print(settings.displayStatusRefresh);
          }
          if (strcmp(strVariable, "SRV") == 0) // Shunt Resistor Value
          {
            sscanf(receivedChars, "%s %s %d",
                   strCmd, strVariable, intValue[0]);
            Serial.print(F("Changed Shunt Resistor Value from '"));
            Serial.print(getShuntResistorValue());
            Serial.print(F( "' to '"));
            setShuntResistorValue(intValue[0]);
            Serial.print(getShuntResistorValue());
          }
          if (strcmp(strVariable, "TBV") == 0) // Thermistor Beta Value
          {
            sscanf(receivedChars, "%s %s %d",
                   strCmd, strVariable, intValue[0]);
            Serial.print(F( "Changed Thermistor Beta Value from '"));
            Serial.print(getThermistorBetaValue());
            Serial.print(F( "' to '"));
            setThermistorBetaValue(intValue[0]);
            Serial.print(getThermistorBetaValue());
          }
          if (strcmp(strVariable, "TL") == 0) // Temp Limits
          {
            sscanf(receivedChars, "%s %s %d %d %d %d",
                   strCmd, strVariable, intValue[0], intValue[1], intValue[2], intValue[3]);
            Serial.println(F( "Changed Temperature Limits from:"));
            for (int i = 0; i < 4; i++) {
              Serial.print(getTemperatureLimits(i));
              if (i < 3) Serial.print(F(", "));
            }
            setTemperatureLimits(intValue[0], intValue[1], intValue[2], intValue[3]);
            Serial.print(F(" to "));
            for (int i = 0; i < 4; i++) {
              Serial.print(getTemperatureLimits(i));
              if (i < 3) Serial.print(F(", "));
            }
          }
          if (strcmp(strVariable, "SCP") == 0) // Short Circuit Protection
          {
            sscanf(receivedChars, "%s %s %d %d",
                   strCmd, strVariable, intValue[0], intValue[1]);
            Serial.print(F( "Changed Short Circuit Protection from '%d', '%d' to '"));
            Serial.print(intValue[0]);
            Serial.print(F( "', '"));
            Serial.print(intValue[1]);
            setShortCircuitProtection(intValue[0], intValue[1]);
          }
          if (strcmp(strVariable, "OCP") == 0) // Over Current Charge Protection
          {
            sscanf(receivedChars, "%s %s %d %d",
                   strCmd, strVariable, intValue[0], intValue[1]);
            Serial.print(F( "Changed Overcurrent Charge Protection from "));
            Serial.print(getOvercurrentChargeProtectionCurrent());
            Serial.print(F( ", "));
            Serial.print(getOvercurrentChargeProtectionDelay());
            Serial.print(F( " to "));
            setOvercurrentChargeProtection(intValue[0], intValue[1]);
            Serial.print(getOvercurrentChargeProtectionCurrent());
            Serial.print(F( ", "));
            Serial.print(getOvercurrentChargeProtectionDelay());
          }
          if (strcmp(strVariable, "ODP") == 0) // Over Current Discharge Protection
          {
            sscanf(receivedChars, "%s %s %d %d",
                   strCmd, strVariable, intValue[0], intValue[1]);
            Serial.print(F( "Changed Overcurrent Discharge Protection from "));
            Serial.print(getOvercurrentDischargeProtectionCurrent());
            Serial.print(F( ", "));
            Serial.print(getOvercurrentDischargeProtectionDelay());
            Serial.print(F( " to "));
            setOvercurrentDischargeProtection(intValue[0], intValue[1]);
            Serial.print(getOvercurrentDischargeProtectionCurrent());
            Serial.print(F( ", "));
            Serial.print(getOvercurrentDischargeProtectionDelay());
          }
          if (strcmp(strVariable, "CUP") == 0) // Cell Undervoltage Protection
          {
            sscanf(receivedChars, "%s %s %d %d",
                   strCmd, strVariable, intValue[0], intValue[1]);
            Serial.print(F( "Changed Cell Undervoltage Protection from "));
            Serial.print(getCellUndervoltageProtectionVoltage());
            Serial.print(F( ", "));
            Serial.print(getCellUndervoltageProtectionDelay());
            Serial.print(F( " to "));
            setCellUndervoltageProtection(intValue[0], intValue[1]);
            Serial.print(getCellUndervoltageProtectionVoltage());
            Serial.print(F( ", "));
            Serial.print(getCellUndervoltageProtectionDelay());
          }
          if (strcmp(strVariable, "COP") == 0) // Cell Overvoltage Protection
          {
            sscanf(receivedChars, "%s %s %d %d",
                   strCmd, strVariable, intValue[0], intValue[1]);
            Serial.print(F( "Changed Cell Overvoltage Protection from "));
            Serial.print(getCellOvervoltageProtectionVoltage());
            Serial.print(F( ", "));
            Serial.print(getCellOvervoltageProtectionDelay());
            Serial.print(F( " to "));
            setCellOvervoltageProtection(intValue[0], intValue[1]);
            Serial.print(getCellOvervoltageProtectionVoltage());
            Serial.print(F( ", "));
            Serial.print(getCellOvervoltageProtectionDelay());
          }
          if (strcmp(strVariable, "BT") == 0) // Balancing Threshold
          {
            sscanf(receivedChars, "%s %s %d %d %d",
                   strCmd, strVariable, intValue[0], intValue[1], intValue[2]);
            Serial.print(F( "Changed Balancing Threshold from "));
            Serial.print(getBalancingThresholdTime());
            Serial.print(F( ", "));
            Serial.print(getBalancingThresholdVoltage());
            Serial.print(F( ", "));
            Serial.print(getBalancingThresholdDifference());
            Serial.print(F( " to "));
            setBalancingThresholds(intValue[0], intValue[1], intValue[2]);
            Serial.print(getBalancingThresholdTime());
            Serial.print(F( ", "));
            Serial.print(getBalancingThresholdVoltage());
            Serial.print(F( ", "));
            Serial.print(getBalancingThresholdDifference());
          }
          if (strcmp(strVariable, "ICT") == 0) // Idle Current Threshold
          {
            sscanf(receivedChars, "%s %s %d",
                   strCmd, strVariable, intValue[0]);
            Serial.print(F( "Changed Idle Current Threshold Value from "));
            Serial.print(getIdleCurrentThreshold());
            Serial.print(F( " to "));
            setIdleCurrentThreshold(intValue[0]);
            Serial.print(getIdleCurrentThreshold());
          }
        }
        Serial.println();

        switch (strCmd[1])
        {
          case 'v':
            //            if (strcmp(strVariable, "disp") == 0)
            //              settings.displayStatusRefresh = intValue[0];
            break;
          default:
            break;
        }
        break;
      default:
        Serial.println("Invalid option.");
    }
    newData = false;

    Serial.println();
    Serial.println();
    PrintMenu();
  }
}

boolean recvWithEndMarker(char *rcvdChars) {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  while (Serial.available() > 0 && newData == false)
  {
    rc = Serial.read();

    if (rc != endMarker) {
      rcvdChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      rcvdChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void display_help(bool bSerial)
{
  char buf[200];

  if (bSerial)
  { // Code here for displaying the help information to the Serial Monitor
    Serial.print(F("\n-----------------------------------------------------------\n"));
    Serial.print(F("                     Help Information\n"));
    Serial.print(F("-----------------------------------------------------------\n"));
    Serial.print(F("Commands for controlling the bms from the Serial Monitor\n"));
    Serial.print(F("-----------------------------------------------------------\n"));
    Serial.println();
  }
  else
  { // Code here for displaying the help information to the Display

  }
}

