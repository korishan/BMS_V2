#include <Arduino.h>
#include <Wire.h>
#include "declares.h"
#include "settings.h"

short nDevices;

void writeTo(uint8_t deviceAddress, uint16_t settingFlag, uint16_t FlagValue)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(settingFlag);
  Wire.write(FlagValue);
  Wire.endTransmission();

  delay(10);
}

int16_t readFrom(uint8_t deviceAddress, uint16_t settingFlag, uint16_t numBytes)
{
  int numOfBytes;
  Wire.beginTransmission(deviceAddress);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(deviceAddress, numBytes);

  int16_t rData = -1;
  numOfBytes = Wire.available();
//  Serial.print(F("numOfBytes Available: ");
//  Serial.println(numOfBytes);

  if (numOfBytes > 0) {
    for (int i = 0; i < numOfBytes; i++) {
      uint8_t aData = Wire.read();
//      Serial.print(F("Value ");
//      Serial.print(i);
//      Serial.print(F(": ");
//      Serial.print(aData);
      if (numBytes > 1) {
        uint8_t bData = Wire.read();
        rData = (aData << 8) | bData;
      }
      else {
        rData = aData;
      }
    }
    //    rData = Wire.read();
    //    if (numBytes > 1) {
    //      uint8_t b;
    //      b = Wire.read();
    //      rData = (rData << 8) | b;
    //      Serial.println("Second Byte");
  }

  return rData;
}

void SearchDevices()
{
  uint8_t findaddress;
  uint8_t error;

  Serial.println(F("\nScanning Bus..."));

  int nDevices = 0;
  for (findaddress = 1; findaddress < 127; findaddress++ )
  {
    // The i2c_scanner uses the return value of the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(findaddress);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print(F("Found device at address 0x"));
      if (findaddress < 16)
        Serial.print(F("0"));
      Serial.print(findaddress, HEX);
      Serial.println(F("  !"));
//      bms.address = findaddress;
      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print(F("Unknown error at address 0x"));
      if (findaddress < 16)
        Serial.print(F("0"));
      Serial.println(findaddress, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println(F("No I2C devices found\n"));
  else
    Serial.println(F("done\n"));

  String buf = String("Found " + String(nDevices) + " device(s)");
  printScreen(20, 20, buf);
  delay(1000);

  return;
}


