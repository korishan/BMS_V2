/*
    bq769x0.cpp - Battery management system based on bq769x0 for Arduino
    Copyright (C) 2015  Martin Jäger (m.jaeger@posteo.de)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program. If not, see
    <http://www.gnu.org/licenses/>.
*/

/*
  TODO:
  - Balancing algorithm
  - SOC calculation + coulomb counting

*/

#include <Arduino.h>
#include <Wire.h>     // I2C/TWI (for Battery Management IC)
#include <math.h>     // log for thermistor calculation

#include "bq769x0.h"
#include "registers.h"

// for the ISR to know the bq769x0 instance
bq769x0* bq769x0::instancePointer = 0;

#if BQ769X0_DEBUG

const char *byte2char(int x)
{
  static char b[9];
  b[0] = '\0';

  int z;
  for (z = 128; z > 0; z >>= 1)
  {
    strcat(b, ((x & z) == z) ? "1" : "0");
  }

  return b;
}

#endif

uint8_t _crc8_ccitt_update (uint8_t inCrc, uint8_t inData)
{
  uint8_t   i;
  uint8_t   data;

  data = inCrc ^ inData;

  for ( i = 0; i < 8; i++ )
  {
    if (( data & 0x80 ) != 0 )
    {
      data <<= 1;
      data ^= 0x07;
    }
    else
    {
      data <<= 1;
    }
  }
  return data;
}

//----------------------------------------------------------------------------
//bq769x0::bq769x0(byte bqType, int bqI2CAddress)
bq769x0::bq769x0(I2C& bqI2C, byte bqType, int bqI2CAddress):_i2c(bqI2C)
{
  _timer.start();

  // set some safe default values
  autoBalancingEnabled = false;
  balancingMinIdleTime_s = 1800;    // default: 30 minutes
  idleCurrentThreshold = 30; // mA

  thermistorBetaValue = 3435;  // typical value for Semitec 103AT-5 thermistor

  alertInterruptFlag = true;   // init with true to check and clear errors at start-up
  type = bqType;
  I2CAddress = bqI2CAddress;

  if (type == bq76920) {
    numberOfCells = 5;
  }
  else if (type == bq76930) {
    numberOfCells = 10;
  }
  else {
    numberOfCells = 15;
  }

  // prevent errors if someone reduced MAX_NUMBER_OF_CELLS accidentally
  if (numberOfCells > MAX_NUMBER_OF_CELLS) {
    numberOfCells = MAX_NUMBER_OF_CELLS;
  }
}

//bq769x0::bq769x0(I2C& bqI2C, byte alertPin, int bqType, int bqI2CAddress, bool crc):
//    _i2c(bqI2C), _alertInterrupt(alertPin)
//{
//    _timer.start();
//    _alertInterrupt.rise(callback(this, &bq769x0::setAlertInterruptFlag));
//
//    // set some safe default values
//    autoBalancingEnabled = false;
//    balancingMinIdleTime_s = 1800;    // default: 30 minutes
//    idleCurrentThreshold = 30; // mA
//
//    thermistorBetaValue = 3435;  // typical value for Semitec 103AT-5 thermistor
//
//    alertInterruptFlag = true;   // init with true to check and clear errors at start-up
//
//    type = bqType;
//    if (type == bq76920) {
//        numberOfCells = 5;
//    } else if (type == bq76930) {
//        numberOfCells = 10;
//    } else {
//        numberOfCells = 15;
//    }
//
//    // initialize variables
//    for (int i = 0; i < numberOfCells - 1; i++) {
//        cellVoltages[i] = 0;
//    }
//
//    //crcEnabled = crc;
//    //I2CAddress = bqI2CAddress;
//
//    if (determineAddressAndCrc())
//    {
//        // initial settings for bq769x0
//        writeRegister(SYS_CTRL1, 0b00011000);  // switch external thermistor and ADC on
//        writeRegister(SYS_CTRL2, 0b01000000);  // switch CC_EN on
//
//        // get ADC offset and gain
//        adcOffset = (signed int) readRegister(ADCOFFSET);  // convert from 2's complement
//        adcGain = 365 + (((readRegister(ADCGAIN1) & 0b00001100) << 1) |
//            ((readRegister(ADCGAIN2) & 0b11100000) >> 5)); // uV/LSB
//    }
//    else {
//        // TODO: do something else... e.g. set error flag
//#if BQ769X0_DEBUG
//        printf("BMS communication error\n");
//#endif
//    }
//}

//-----------------------------------------------------------------------------
int bq769x0::begin(byte alertPin, byte bootPin)
{
  Wire.begin();        // join I2C bus

  // initialize variables
  for (byte i = 0; i < 4; i++) {
    cellVoltages[i] = 0;
  }

  // Boot IC if pin is defined (else: manual boot via push button has to be
  // done before calling this method)
  if (bootPin >= 0)
  {
#if BQ769X0_DEBUG
    Serial.println("BMS Boot Pin " + String(bootPin));
#endif
    pinMode(bootPin, OUTPUT);
    digitalWrite(bootPin, HIGH);
    delay(5);   // wait 5 ms for device to receive boot signal (datasheet: max. 2 ms)
    pinMode(bootPin, INPUT);     // don't disturb temperature measurement
    delay(10);  // wait for device to boot up completely (datasheet: max. 10 ms)
  }

  // test communication
#if BQ769X0_DEBUG
  Serial.println("Attempt writeRegister(CC_CFG) = 0x19");
#endif
  writeRegister(CC_CFG, 0x19);       // should be set to 0x19 according to datasheet

  if (readRegister(CC_CFG) == 0x19)
  {
#if BQ769X0_DEBUG
    Serial.println("Attempt writeRegister 0x19 Succeeded");
#endif
    // initial settings for bq769x0
    writeRegister(SYS_CTRL1, B00011000);  // switch external thermistor and ADC on
    writeRegister(SYS_CTRL2, B01000000);  // switch CC_EN on

    // attach ALERT interrupt to this instance
    instancePointer = this;
    attachInterrupt(digitalPinToInterrupt(alertPin), bq769x0::alertISR, RISING);

    // get ADC offset and gain
    adcOffset = (signed int) readRegister(ADCOFFSET);  // convert from 2's complement
    adcGain = 365 + (((readRegister(ADCGAIN1) & B00001100) << 1) |
                     ((readRegister(ADCGAIN2) & B11100000) >> 5)); // uV/LSB
    return 0;
  }
  else
  {
#if BQ769X0_DEBUG
    Serial.println("BMS communication error");
    Serial.print("FAILED: readRegister(CC_CFG) = ");
    Serial.println(readRegister(CC_CFG), HEX);
#endif
    return 1;
  }
}

//----------------------------------------------------------------------------
// The bq769x0 drives the ALERT pin high if the SYS_STAT register contains
// a new value (either new CC reading or an error)
void bq769x0::alertISR()
{
  if (instancePointer != 0)
  {
    instancePointer->setAlertInterruptFlag();
  }
}

//----------------------------------------------------------------------------
// Fast function to check whether BMS has an error
// (returns 0 if everything is OK)
int bq769x0::checkStatus()
{
  //  Serial.print("errorStatus: ");
  //  Serial.println(errorStatus);
  if (alertInterruptFlag == false && errorStatus == 0) {
    return 0;
  }
  else {

    regSYS_STAT_t sys_stat;
    sys_stat.regByte = readRegister(SYS_STAT);

    if (sys_stat.bits.CC_READY == 1) {
      //Serial.println("Interrupt: CC ready");
      updateCurrent(true);  // automatically clears CC ready flag
    }

    // Serious error occured
    if (sys_stat.regByte & B00111111)
    {
      if (alertInterruptFlag == true) {
        secSinceErrorCounter = 0;
      }
      errorStatus = sys_stat.regByte;

      int secSinceInterrupt = (millis() - interruptTimestamp) / 1000;

      // check for overrun of millis() or very slow running program
      if (abs(secSinceInterrupt - secSinceErrorCounter) > 2) {
        secSinceErrorCounter = secSinceInterrupt;
      }

      // called only once per second
      if (secSinceInterrupt >= secSinceErrorCounter)
      {
        if (sys_stat.regByte & B00100000) { // XR error
          // datasheet recommendation: try to clear after waiting a few seconds
          if (secSinceErrorCounter % 3 == 0) {
#if BQ769X0_DEBUG
            Serial.println(F("Attempting to clear XR error"));
#endif
            writeRegister(SYS_STAT, B00100000);
          }
        }
        if (sys_stat.regByte & B00010000) { // Alert error
          if (secSinceErrorCounter % 10 == 0) {
#if BQ769X0_DEBUG
            Serial.println(F("Attempting to clear Alert error"));
#endif
            writeRegister(SYS_STAT, B00010000);
          }
        }
        if (sys_stat.regByte & B00001000) { // UV error
          updateVoltages();
          if (cellVoltages[idCellMinVoltage] > minCellVoltage) {
#if BQ769X0_DEBUG
            Serial.println(F("Attempting to clear UV error"));
#endif
            writeRegister(SYS_STAT, B00001000);
          }
        }
        if (sys_stat.regByte & B00000100) { // OV error
          updateVoltages();
          if (cellVoltages[idCellMaxVoltage] < maxCellVoltage) {
#if BQ769X0_DEBUG
            Serial.println(F("Attempting to clear OV error"));
#endif
            writeRegister(SYS_STAT, B00000100);
          }
        }
        if (sys_stat.regByte & B00000010) { // SCD
          if (secSinceErrorCounter % 60 == 0) {
#if BQ769X0_DEBUG
            Serial.println(F("Attempting to clear SCD error"));
#endif
            writeRegister(SYS_STAT, B00000010);
          }
        }
        if (sys_stat.regByte & B00000001) { // OCD
          if (secSinceErrorCounter % 60 == 0) {
#if BQ769X0_DEBUG
            Serial.println(F("Attempting to clear OCD error"));
#endif
            writeRegister(SYS_STAT, B00000001);
          }
        }

        secSinceErrorCounter++;
      }
    }
    else {
      errorStatus = 0;
    }

    return errorStatus;

  }

}

//----------------------------------------------------------------------------
// should be called at least once every 250 ms to get correct coulomb counting
void bq769x0::update()
{
  updateCurrent();  // will only read new current value if alert was triggered
  updateVoltages();
  updateTemperatures();
  updateBalancingSwitches();
}

//----------------------------------------------------------------------------
// puts BMS IC into SHIP mode (i.e. switched off)
void bq769x0::shutdown()
{
  writeRegister(SYS_CTRL1, 0x0);
  writeRegister(SYS_CTRL1, 0x1);
  writeRegister(SYS_CTRL1, 0x2);
}

//----------------------------------------------------------------------------
bool bq769x0::enableCharging()
{
  if (checkStatus() == 0 &&
      cellVoltages[idCellMaxVoltage] < maxCellVoltage &&
      temperatures[0] < maxCellTempCharge &&
      temperatures[0] > minCellTempCharge)
  {
    byte sys_ctrl2;
    sys_ctrl2 = readRegister(SYS_CTRL2);
    writeRegister(SYS_CTRL2, sys_ctrl2 | B00000001);  // switch CHG on
#if BQ769X0_DEBUG
    Serial.println("Enabling CHG FET");
#endif
    return true;
  }
  else {
    return false;
  }
}

//----------------------------------------------------------------------------
bool bq769x0::enableDischarging()
{
  if (checkStatus() == 0 &&
      cellVoltages[idCellMinVoltage] > minCellVoltage &&
      temperatures[0] < maxCellTempDischarge &&
      temperatures[0] > minCellTempDischarge)
  {
    byte sys_ctrl2;
    sys_ctrl2 = readRegister(SYS_CTRL2);
    writeRegister(SYS_CTRL2, sys_ctrl2 | B00000010);  // switch DSG on
    return true;
  }
  else {
    return false;
  }
}

//----------------------------------------------------------------------------
void bq769x0::enableAutoBalancing(void)
{
  autoBalancingEnabled = true;
}

void bq769x0::disableAutoBalancing(void)
{
  autoBalancingEnabled = false;
}

//----------------------------------------------------------------------------
bool bq769x0::disableCharging()
{
  if (checkStatus() == 0 )
  {
    byte sys_ctrl2;
    sys_ctrl2 = readRegister(SYS_CTRL2);
    writeRegister(SYS_CTRL2, sys_ctrl2 | B00000000);  // switch CHG off
#if BQ769X0_DEBUG
    Serial.println("Disabling CHG FET");
#endif

    return true;
  }
  else
  {
    return false;
  }
}

//----------------------------------------------------------------------------
bool bq769x0::disableDischarging()
{
  if (checkStatus() == 0 )
  {
    byte sys_ctrl2;
    sys_ctrl2 = readRegister(SYS_CTRL2);
    writeRegister(SYS_CTRL2, sys_ctrl2 | B00000000);  // switch DSG off
    return true;
  }
  else {
    return false;
  }
}

//----------------------------------------------------------------------------
float bq769x0::getSOC(void)
{
  return (double) coulombCounter / nominalCapacity * 100;
}

//----------------------------------------------------------------------------
int bq769x0::getBalancingStatus()
{
  return balancingStatus;
}

//----------------------------------------------------------------------------
int bq769x0::getBatteryCurrent()
{
  return batCurrent;
}

//----------------------------------------------------------------------------
int bq769x0::getBatteryVoltage()
{
  return batVoltage;
}

//----------------------------------------------------------------------------
int bq769x0::getBalancingThresholdTime()
{
  return balancingMinIdleTime_s / 60;
}

//----------------------------------------------------------------------------
int bq769x0::getBalancingThresholdVoltage()
{
  return balancingMinCellVoltage_mV;
}

//----------------------------------------------------------------------------
byte bq769x0::getBalancingThresholdDifference()
{
  return balancingMaxVoltageDifference_mV;
}

//----------------------------------------------------------------------------
int bq769x0::getCellUndervoltageProtectionVoltage()
{
  return 0;
}

//----------------------------------------------------------------------------
int bq769x0::getCellUndervoltageProtectionDelay()
{
  return 0;
}

//----------------------------------------------------------------------------
int bq769x0::getCellOvervoltageProtectionVoltage()
{
  return 0;
}

//----------------------------------------------------------------------------
int bq769x0::getCellOvervoltageProtectionDelay()
{
  return 0;
}

//----------------------------------------------------------------------------
int bq769x0::getCellVoltage(byte idCell)
{
  return cellVoltages[idCell - 1];
}

//----------------------------------------------------------------------------
int bq769x0::getIdleCurrentThreshold()
{
  return idleCurrentThreshold;
}

//----------------------------------------------------------------------------
int bq769x0::getMaxCellVoltage()
{
  return cellVoltages[idCellMaxVoltage];
}

//----------------------------------------------------------------------------
int bq769x0::getMinCellVoltage()
{
  return cellVoltages[idCellMinVoltage];
}

//----------------------------------------------------------------------------
int bq769x0::getNumberOfConnectedCells(void)
{
  return connectedCells;
}

//----------------------------------------------------------------------------
long bq769x0::getOvercurrentChargeProtectionCurrent()
{
  return 0;
}

//----------------------------------------------------------------------------
int bq769x0::getOvercurrentChargeProtectionDelay()
{
  return 0;
}

//----------------------------------------------------------------------------
long bq769x0::getOvercurrentDischargeProtectionCurrent()
{
  return 0;
}

//----------------------------------------------------------------------------
int bq769x0::getOvercurrentDischargeProtectionDelay()
{
  return 0;
}

//----------------------------------------------------------------------------
long bq769x0::getShortCircuitProtectionCurrent()
{
  return 0;
}

//----------------------------------------------------------------------------
int bq769x0::getShortCircuitProtectionDelay()
{
  return 0;
}

//----------------------------------------------------------------------------
int bq769x0::getShuntResistorValue()
{
  return shuntResistorValue_mOhm;
}

//----------------------------------------------------------------------------
int bq769x0::getTemperatureLimits(int temperatureSetting)
{
  // Temperature limits (°C/10)
  switch (temperatureSetting)
  {
    case 0: return minCellTempDischarge;
    case 1: return maxCellTempDischarge;
    case 2: return minCellTempCharge;
    case 3: return maxCellTempCharge;
    default: return -1;
  }
}

//----------------------------------------------------------------------------
float bq769x0::getTemperatureDegC(byte channel)
{
  if (channel >= 1 && channel <= 3) {
    return (float)temperatures[channel - 1] / 10.0;
  }
  else
    return -273.15;   // Error: Return absolute minimum temperature
}

//----------------------------------------------------------------------------
float bq769x0::getTemperatureDegF(byte channel)
{
  return getTemperatureDegC(channel) * 1.8 + 32;
}

int bq769x0::getThermistorBetaValue()
{
  return thermistorBetaValue;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// the actual ISR, called by static function alertISR()
void bq769x0::setAlertInterruptFlag()
{
  interruptTimestamp = millis();
  alertInterruptFlag = true;
}

//----------------------------------------------------------------------------
void bq769x0::setBalancingThresholds(int idleTime_min, int absVoltage_mV, byte voltageDifference_mV)
{
  balancingMinIdleTime_s = idleTime_min * 60;
  balancingMinCellVoltage_mV = absVoltage_mV;
  balancingMaxVoltageDifference_mV = voltageDifference_mV;
}

//----------------------------------------------------------------------------
void bq769x0::setBatteryCapacity(long capacity_mAh)
{
  nominalCapacity = capacity_mAh * 3600;
}

//----------------------------------------------------------------------------
int bq769x0::setCellUndervoltageProtection(int voltage_mV, int delay_s)
{
  regPROTECT3_t protect3;
  byte uv_trip = 0;

  minCellVoltage = voltage_mV;

  protect3.regByte = readRegister(PROTECT3);

  uv_trip = ((((long)voltage_mV - adcOffset) * 1000 / adcGain) >> 4) & 0x00FF;
  uv_trip += 1;   // always round up for lower cell voltage
  writeRegister(UV_TRIP, uv_trip);

  protect3.bits.UV_DELAY = 0;
  for (int i = sizeof(UV_delay_setting) - 1; i > 0; i--) {
    if (delay_s >= UV_delay_setting[i]) {
      protect3.bits.UV_DELAY = i;
      break;
    }
  }

  writeRegister(PROTECT3, protect3.regByte);

  // returns the actual current threshold value
  return ((long)1 << 12 | uv_trip << 4) * adcGain / 1000 + adcOffset;
}

//----------------------------------------------------------------------------
int bq769x0::setCellOvervoltageProtection(int voltage_mV, int delay_s)
{
  regPROTECT3_t protect3;
  byte ov_trip = 0;

  maxCellVoltage = voltage_mV;

  protect3.regByte = readRegister(PROTECT3);

  ov_trip = ((((long)voltage_mV - adcOffset) * 1000 / adcGain) >> 4) & 0x00FF;
  writeRegister(OV_TRIP, ov_trip);

  protect3.bits.OV_DELAY = 0;
  for (int i = sizeof(OV_delay_setting) - 1; i > 0; i--) {
    if (delay_s >= OV_delay_setting[i]) {
      protect3.bits.OV_DELAY = i;
      break;
    }
  }

  writeRegister(PROTECT3, protect3.regByte);

  // returns the actual current threshold value
  return ((long)1 << 13 | ov_trip << 4) * adcGain / 1000 + adcOffset;
}

void bq769x0::setIdleCurrentThreshold(int current_mA)
{
  idleCurrentThreshold = current_mA;
}

//----------------------------------------------------------------------------
long bq769x0::setOvercurrentChargeProtection(long current_mA, int delay_ms)
{
  // ToDo: Software protection for charge overcurrent
}

//----------------------------------------------------------------------------
long bq769x0::setOvercurrentDischargeProtection(long current_mA, int delay_ms)
{
  regPROTECT2_t protect2;

  // Remark: RSNS must be set to 1 in PROTECT1 register

  protect2.bits.OCD_THRESH = 0;
  for (int i = sizeof(OCD_threshold_setting) - 1; i > 0; i--) {
    if (current_mA * shuntResistorValue_mOhm / 1000 >= OCD_threshold_setting[i]) {
      protect2.bits.OCD_THRESH = i;
      break;
    }
  }

  protect2.bits.OCD_DELAY = 0;
  for (int i = sizeof(OCD_delay_setting) - 1; i > 0; i--) {
    if (delay_ms >= OCD_delay_setting[i]) {
      protect2.bits.OCD_DELAY = i;
      break;
    }
  }

  writeRegister(PROTECT2, protect2.regByte);

  // returns the actual current threshold value
  return (long)OCD_threshold_setting[protect2.bits.OCD_THRESH] * 1000 /
         shuntResistorValue_mOhm;
}

//----------------------------------------------------------------------------
long bq769x0::setShortCircuitProtection(long current_mA, int delay_us)
{
  regPROTECT1_t protect1;

  // only RSNS = 1 considered
  protect1.bits.RSNS = 1;

  protect1.bits.SCD_THRESH = 0;
  for (int i = sizeof(SCD_threshold_setting) - 1; i > 0; i--) {
    if (current_mA * shuntResistorValue_mOhm / 1000 >= SCD_threshold_setting[i]) {
      protect1.bits.SCD_THRESH = i;
      break;
    }
  }

  protect1.bits.SCD_DELAY = 0;
  for (int i = sizeof(SCD_delay_setting) - 1; i > 0; i--) {
    if (delay_us >= SCD_delay_setting[i]) {
      protect1.bits.SCD_DELAY = i;
      break;
    }
  }

  writeRegister(PROTECT1, protect1.regByte);

  // returns the actual current threshold value
  return (long)SCD_threshold_setting[protect1.bits.SCD_THRESH] * 1000 / shuntResistorValue_mOhm;
}

//----------------------------------------------------------------------------
void bq769x0::setShuntResistorValue(int res_mOhm)
{
  shuntResistorValue_mOhm = res_mOhm;
}

//----------------------------------------------------------------------------
void bq769x0::setTemperatureLimits(int minDischarge_degC, int maxDischarge_degC, int minCharge_degC, int maxCharge_degC)
{
  // Temperature limits (°C/10)
  minCellTempDischarge = minDischarge_degC * 10;
  maxCellTempDischarge = maxDischarge_degC * 10;
  minCellTempCharge = minCharge_degC * 10;
  maxCellTempCharge = maxCharge_degC * 10;
}

//----------------------------------------------------------------------------
void bq769x0::setThermistorBetaValue(int beta_K)
{
  thermistorBetaValue = beta_K;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// sets balancing registers if balancing is allowed
// (sufficient idle time + voltage)
byte bq769x0::updateBalancingSwitches(void)
{
  long idleSeconds = (millis() - idleTimestamp) / 1000;
  byte numberOfSections = numberOfCells / 5;

  // check for millis() overflow
  if (idleSeconds < 0) {
    idleTimestamp = 0;
    idleSeconds = millis() / 1000;
  }

  // check if balancing allowed
  if (checkStatus() == 0 &&
      idleSeconds >= balancingMinIdleTime_s &&
      cellVoltages[idCellMaxVoltage] > balancingMinCellVoltage_mV &&
      (cellVoltages[idCellMaxVoltage] - cellVoltages[idCellMinVoltage]) > balancingMaxVoltageDifference_mV)
  {
    balancingActive = true;
    //Serial.println("Balancing enabled!");

    regCELLBAL_t cellbal;
    byte balancingFlags;
    byte balancingFlagsTarget;

    for (int section = 0; section < numberOfSections; section++)
    {
      balancingFlags = 0;
      for (int i = 0; i < 5; i++)
      {
        if ((cellVoltages[section * 5 + i] - cellVoltages[idCellMinVoltage]) > balancingMaxVoltageDifference_mV) {

          // try to enable balancing of current cell
          balancingFlagsTarget = balancingFlags | (1 << i);

          // check if attempting to balance adjacent cells
          bool adjacentCellCollision =
            ((balancingFlagsTarget << 1) & balancingFlags) ||
            ((balancingFlags << 1) & balancingFlagsTarget);

          if (adjacentCellCollision == false) {
            balancingFlags = balancingFlagsTarget;
          }
        }
      }
      Serial.print("Setting CELLBAL");
      Serial.print(section + 1);
      Serial.print(" register to: ");
      Serial.println(byte2char(balancingFlags));

      // set balancing register for this section
      writeRegister(CELLBAL1 + section, balancingFlags);
    }
  }
  else if (balancingActive == true)
  {
    // clear all CELLBAL registers
    for (int section = 0; section < numberOfSections; section++)
    {
      Serial.print("Clearing Register CELLBAL");
      Serial.println(section + 1);
      writeRegister(CELLBAL1 + section, 0x0);
    }

    balancingActive = false;
  }
}

//----------------------------------------------------------------------------
// If ignoreCCReadFlag == true, the current is read independent of an interrupt
// indicating the availability of a new CC reading
void bq769x0::updateCurrent(bool ignoreCCReadyFlag)
{
  int adcVal = 0;
  regSYS_STAT_t sys_stat;
  sys_stat.regByte = readRegister(SYS_STAT);

  if (ignoreCCReadyFlag == true || sys_stat.bits.CC_READY == 1)
  {
    adcVal = (readRegister(0x32) << 8) | readRegister(0x33);
    batCurrent = adcVal * 8.44 / 5.0;  // mA

    if (batCurrent > -10 && batCurrent < 10)
    {
      batCurrent = 0;
    }

    // reset idleTimestamp
    if (abs(batCurrent) > idleCurrentThreshold) {
      idleTimestamp = millis();
    }

    // no error occured which caused alert
    if (!(sys_stat.regByte & B00111111)) {
      alertInterruptFlag = false;
    }

    writeRegister(SYS_STAT, B10000000);  // Clear CC ready flag
  }
}

//----------------------------------------------------------------------------
void bq769x0::updateTemperatures()
{
  float tmp = 0;
  int adcVal = 0;
  int vtsx = 0;
  unsigned long rts = 0;
  /*
    Wire.beginTransmission(I2CAddress);
    Wire.write(0x2C);
    Wire.endTransmission();

    if (Wire.requestFrom(I2CAddress, 2) == 2)
    {
    // calculate R_thermistor according to bq769x0 datasheet
    adcVal = ((Wire.read() & B00111111) << 8) | Wire.read();
    vtsx = adcVal * 0.382; // mV
    rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm

    // Temperature calculation using Beta equation
    // - According to bq769x0 datasheet, only 10k thermistors should be used
    // - 25°C reference temperature for Beta equation assumed
    tmp = 1.0/(1.0/(273.15+25) + 1.0/thermistorBetaValue*log(rts/10000.0)); // K

    temperatures[0] = (tmp - 273.15) * 10.0;
    }*/
  // calculate R_thermistor according to bq769x0 datasheet
  adcVal = (readRegister(TS1_HI_BYTE) & 0b00111111) << 8 | readRegister(TS1_LO_BYTE);
  vtsx = adcVal * 0.382; // mV
  rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm

  // Temperature calculation using Beta equation
  // - According to bq769x0 datasheet, only 10k thermistors should be used
  // - 25°C reference temperature for Beta equation assumed
  tmp = 1.0 / (1.0 / (273.15 + 25) + 1.0 / thermistorBetaValue * log(rts / 10000.0)); // K
  temperatures[0] = (tmp - 273.15) * 10.0;

  if (type == bq76930 || type == bq76940) {
    adcVal = (readRegister(TS2_HI_BYTE) & 0b00111111) << 8 | readRegister(TS2_LO_BYTE);
    vtsx = adcVal * 0.382; // mV
    rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm
    tmp = 1.0 / (1.0 / (273.15 + 25) + 1.0 / thermistorBetaValue * log(rts / 10000.0)); // K
    temperatures[1] = (tmp - 273.15) * 10.0;
  }

  if (type == bq76940) {
    adcVal = (readRegister(TS3_HI_BYTE) & 0b00111111) << 8 | readRegister(TS3_LO_BYTE);
    vtsx = adcVal * 0.382; // mV
    rts = 10000.0 * vtsx / (3300.0 - vtsx); // Ohm
    tmp = 1.0 / (1.0 / (273.15 + 25) + 1.0 / thermistorBetaValue * log(rts / 10000.0)); // K
    temperatures[2] = (tmp - 273.15) * 10.0;
  }
}

//----------------------------------------------------------------------------
// reads all cell voltages to array cellVoltages[4] and updates batVoltage
/*void bq769x0::updateVoltages()
  {
  long adcVal = 0;

  // read battery pack voltage
  adcVal = (readRegister(BAT_HI_BYTE) << 8) | readRegister(BAT_LO_BYTE);
  batVoltage = 4.0 * adcGain * adcVal / 1000.0 + 4 * adcOffset;

  // read cell voltages
  Wire.beginTransmission(I2CAddress);
  Wire.write(VC1_HI_BYTE);
  Wire.endTransmission();

  if (Wire.requestFrom(I2CAddress, 2 * numberOfCells) == 2 * numberOfCells)
  {
    idCellMaxVoltage = 0;
    idCellMinVoltage = 0;
    for (int i = 0; i < numberOfCells; i++)
    {
      adcVal = ((Wire.read() & B00111111) << 8) | Wire.read();
      cellVoltages[i] = adcVal * adcGain / 1000 + adcOffset;

      if (cellVoltages[i] > cellVoltages[idCellMaxVoltage]) {
        idCellMaxVoltage = i;
      }
      if (cellVoltages[i] < cellVoltages[idCellMinVoltage] && cellVoltages[i] > 500) {
        idCellMinVoltage = i;
      }
    }
  }
  }*/
//void bq769x0::updateVoltages()
//{
//  long adcVal = 0;
//  char buf[4];
//  int connectedCellsTemp = 0;
//
//  uint8_t crc;
//
//  // read battery pack voltage
//  adcVal = (readRegister(BAT_HI_BYTE) << 8) | readRegister(BAT_LO_BYTE);
//  batVoltage = 4.0 * adcGain * adcVal / 1000.0 + 4 * adcOffset;
//
//  // read cell voltages
//  buf[0] = (char) VC1_HI_BYTE;
//  _i2c.write(I2CAddress << 1, buf, 1);;
//
//  idCellMaxVoltage = 0;
//  idCellMinVoltage = 0;
//  for (int i = 0; i < numberOfCells; i++)
//  {
//    if (crcEnabled == true) {
//      _i2c.read(I2CAddress << 1, buf, 4);
//      adcVal = (buf[0] & 0b00111111) << 8 | buf[2];
//
//      // CRC of first bytes includes slave address (including R/W bit) and data
//      crc = _crc8_ccitt_update(0, (I2CAddress << 1) | 1);
//      crc = _crc8_ccitt_update(crc, buf[0]);
//      if (crc != buf[1]) return; // don't save corrupted value
//
//      // CRC of subsequent bytes contain only data
//      crc = _crc8_ccitt_update(0, buf[2]);
//      if (crc != buf[3]) return; // don't save corrupted value
//    }
//    else {
//      _i2c.read(I2CAddress << 1, buf, 2);
//      adcVal = (buf[0] & 0b00111111) << 8 | buf[1];
//    }
//
//    cellVoltages[i] = adcVal * adcGain / 1000 + adcOffset;
//
//    if (cellVoltages[i] > 500) {
//      connectedCellsTemp++;
//    }
//
//    if (cellVoltages[i] > cellVoltages[idCellMaxVoltage]) {
//      idCellMaxVoltage = i;
//    }
//    if (cellVoltages[i] < cellVoltages[idCellMinVoltage] && cellVoltages[i] > 500) {
//      idCellMinVoltage = i;
//    }
//  }
//  connectedCells = connectedCellsTemp;
//}

void bq769x0::updateVoltages()
{
  long adcVal = 0;
  char buf[4];
  int connectedCellsTemp = 0;

  uint8_t crc;

  // read cell voltages
  buf[0] = (char) VC1_HI_BYTE;
  _i2c.write(I2CAddress << 1, buf, 1);;

  idCellMaxVoltage = 0;
  idCellMinVoltage = 0;
  for (int i = 0; i < numberOfCells; i++)
  {
    if (crcEnabled == true) {
      _i2c.read(I2CAddress << 1, buf, 4);
      adcVal = (buf[0] & 0b00111111) << 8 | buf[2];

      // CRC of first bytes includes slave address (including R/W bit) and data
      crc = _crc8_ccitt_update(0, (I2CAddress << 1) | 1);
      crc = _crc8_ccitt_update(crc, buf[0]);
      if (crc != buf[1]) return; // don't save corrupted value

      // CRC of subsequent bytes contain only data
      crc = _crc8_ccitt_update(0, buf[2]);
      if (crc != buf[3]) return; // don't save corrupted value
    }
    else {
      _i2c.read(I2CAddress << 1, buf, 2);
      adcVal = (buf[0] & 0b00111111) << 8 | buf[1];
    }

    cellVoltages[i] = adcVal * adcGain / 1000 + adcOffset;

    if (cellVoltages[i] > 500) {
      connectedCellsTemp++;
    }

    if (cellVoltages[i] > cellVoltages[idCellMaxVoltage]) {
      idCellMaxVoltage = i;
    }
    if (cellVoltages[i] < cellVoltages[idCellMinVoltage] && cellVoltages[i] > 500) {
      idCellMinVoltage = i;
    }
  }
  connectedCells = connectedCellsTemp;

  // read battery pack voltage
  adcVal = (readRegister(BAT_HI_BYTE) << 8) | readRegister(BAT_LO_BYTE);
  batVoltage = 4.0 * adcGain * adcVal / 1000.0 + connectedCells * adcOffset;
}

// SOC calculation based on average cell open circuit voltage
void bq769x0::resetSOC(int percent)
{
  if (percent <= 100 && percent >= 0)
  {
    coulombCounter = nominalCapacity * percent / 100.0;
  }
  else  // reset based on OCV
  {
    printf("NumCells: %d, voltage: %d V\n", getNumberOfConnectedCells(), getBatteryVoltage());
    int voltage = getBatteryVoltage() / getNumberOfConnectedCells();

    coulombCounter = 0;  // initialize with totally depleted battery (0% SOC)

    for (int i = 0; i < NUM_OCV_POINTS; i++)
    {
      if (OCV[i] <= voltage) {
        if (i == 0) {
          coulombCounter = nominalCapacity;  // 100% full
        }
        else {
          // interpolate between OCV[i] and OCV[i-1]
          coulombCounter = (double) nominalCapacity / (NUM_OCV_POINTS - 1.0) *
                           (NUM_OCV_POINTS - 1.0 - i + ((float)voltage - OCV[i]) / (OCV[i - 1] - OCV[i]));
        }
        return;
      }
    }
  }
}
//----------------------------------------------------------------------------
/*void bq769x0::writeRegister(byte address, int data)
  {
  Wire.beginTransmission(I2CAddress);
  Wire.write(address);
  Wire.write(data);
  Wire.endTransmission();
  }

  //----------------------------------------------------------------------------
  int bq769x0::readRegister(byte address)
  {
  Wire.beginTransmission(I2CAddress);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom(I2CAddress, 1);
  return Wire.read();
  }*/

void bq769x0::writeRegister(int address, int data)
{
  uint8_t crc = 0;
  char buf[3];

  buf[0] = (char) address;
  buf[1] = data;

  if (crcEnabled == true) {
    // CRC is calculated over the slave address (including R/W bit), register address, and data.
    crc = _crc8_ccitt_update(crc, (I2CAddress << 1) | 0);
    crc = _crc8_ccitt_update(crc, buf[0]);
    crc = _crc8_ccitt_update(crc, buf[1]);
    buf[2] = crc;
    _i2c.write(I2CAddress << 1, buf, 3);
  }
  else {
    _i2c.write(I2CAddress << 1, buf, 2);
  }
}

//----------------------------------------------------------------------------
int bq769x0::readRegister(int address)
{
  uint8_t crc = 0;
  char buf[2];

#if BQ769X0_DEBUG
  //printf("Read register: 0x%x \n", address);
#endif

  buf[0] = (char)address;
  _i2c.write(I2CAddress << 1, buf, 1);;

  if (crcEnabled == true) {
    do {
      _i2c.read(I2CAddress << 1, buf, 2);
      // CRC is calculated over the slave address (including R/W bit) and data.
      crc = _crc8_ccitt_update(crc, (I2CAddress << 1) | 1);
      crc = _crc8_ccitt_update(crc, buf[0]);
    } while (crc != buf[1]);
    return buf[0];
  }
  else {
    _i2c.read(I2CAddress << 1, buf, 1);
    return buf[0];
  }
}


#if BQ769X0_DEBUG

//----------------------------------------------------------------------------
// for debug purposes

void bq769x0::printRegisters()
{
  Serial.print(F("0x00 SYS_STAT:  "));
  Serial.println(byte2char(readRegister(SYS_STAT)));

  Serial.print(F("0x01 CELLBAL1:  "));
  Serial.println(byte2char(readRegister(CELLBAL1)));

  Serial.print(F("0x04 SYS_CTRL1: "));
  Serial.println(byte2char(readRegister(SYS_CTRL1)));

  Serial.print(F("0x05 SYS_CTRL2: "));
  Serial.println(byte2char(readRegister(SYS_CTRL2)));

  Serial.print(F("0x06 PROTECT1:  "));
  Serial.println(byte2char(readRegister(PROTECT1)));

  Serial.print(F("0x07 PROTECT2:  "));
  Serial.println(byte2char(readRegister(PROTECT2)));

  Serial.print(F("0x08 PROTECT3   "));
  Serial.println(byte2char(readRegister(PROTECT3)));

  Serial.print(F("0x09 OV_TRIP:   "));
  Serial.println(byte2char(readRegister(OV_TRIP)));

  Serial.print(F("0x0A UV_TRIP:   "));
  Serial.println(byte2char(readRegister(UV_TRIP)));

  Serial.print(F("0x0B CC_CFG:    "));
  Serial.println(byte2char(readRegister(CC_CFG)));

  Serial.print(F("0x32 CC_HI:     "));
  Serial.println(byte2char(readRegister(CC_HI_BYTE)));

  Serial.print(F("0x33 CC_LO:     "));
  Serial.println(byte2char(readRegister(CC_LO_BYTE)));
  /*
    Serial.print(F("0x50 ADCGAIN1:  "));
    Serial.println(byte2char(readRegister(ADCGAIN1)));

    Serial.print(F("0x51 ADCOFFSET: "));
    Serial.println(byte2char(readRegister(ADCOFFSET)));

    Serial.print(F("0x59 ADCGAIN2:  "));
    Serial.println(byte2char(readRegister(ADCGAIN2)));
  */
}

#endif
