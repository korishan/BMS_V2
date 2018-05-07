#include "Arduino.h"
#include "declares.h"
#include "globals.h"
#include "bq769x0.h"    // Library for Texas Instruments bq76920 battery management IC

bq769x0 BMS(bq76920, BMS_I2C_ADDRESS);    // battery management system object

void bmsUpdate()
{
  BMS.update();  // should be called at least every 250 ms
}

int bmsBegin()
{
  Serial.println("bmsBegin Start");
  BMS.begin(BMS_ALERT_PIN, BMS_BOOT_PIN);
  Serial.println("bmsBegin End");
}

void RebootDevice(void)
{
  bmsShutdown();
  delay(50);
  bmsBegin();
}

void bmsShutdown(void)
{
  BMS.shutdown();
}

void setBMSDefaults()
{
  Serial.println("BMS Defaults Start");
  BMS.setTemperatureLimits(-20, 45, 0, 45);
  BMS.setShuntResistorValue(5);
  BMS.setShortCircuitProtection(14000, 200);  // delay in us
  BMS.setOvercurrentChargeProtection(8000, 200);  // delay in ms
  BMS.setOvercurrentDischargeProtection(8000, 320); // delay in ms
  BMS.setCellUndervoltageProtection(2600, 2); // delay in s
  BMS.setCellOvervoltageProtection(3650, 2);  // delay in s

  BMS.setBalancingThresholds(0, 3300, 20);  // minIdleTime_min, minCellV_mV, maxVoltageDiff_mV
  BMS.setIdleCurrentThreshold(100);
  BMS.enableAutoBalancing();
  BMS.enableDischarging();
  Serial.println("BMS Defaults End");
}

int checkStatus() // returns 0 if everything is OK
{
  return BMS.checkStatus();
}

// charging control
bool enableCharging(void) {
  return BMS.enableCharging();
}
bool disableCharging(void) {
  return BMS.disableCharging();
}
bool enableDischarging(void) {
  return BMS.enableDischarging();
}
bool disableDischarging(void) {
  return BMS.disableDischarging();
}

// hardware settings
void setShuntResistorValue(int res_mOhm) {
  BMS.setShuntResistorValue(res_mOhm);
}
void setThermistorBetaValue(int beta_K) {
  BMS.setThermistorBetaValue(beta_K);
}

// limit settings (for battery protection)
void setTemperatureLimits(int minDischarge_degC, int maxDischarge_degC, int minCharge_degC, int maxCharge_degC)  // °C
{
  BMS.setTemperatureLimits(minDischarge_degC, maxDischarge_degC, minCharge_degC, maxCharge_degC);
}
long setShortCircuitProtection(long current_mA, int delay_us )
{
  BMS.setShortCircuitProtection(current_mA, delay_us );
}
long setOvercurrentChargeProtection(long current_mA, int delay_ms )
{
  BMS.setOvercurrentChargeProtection(current_mA, delay_ms );
}
long setOvercurrentDischargeProtection(long current_mA, int delay_ms )
{
  BMS.setOvercurrentDischargeProtection(current_mA, delay_ms );
}
int setCellUndervoltageProtection(int voltage_mV, int delay_s )
{
  BMS.setCellUndervoltageProtection(voltage_mV, delay_s );
}
int setCellOvervoltageProtection(int voltage_mV, int delay_s )
{
  BMS.setCellOvervoltageProtection(voltage_mV, delay_s );
}

// balancing settings
void setBalancingThresholds(int idleTime_min, int absVoltage_mV, byte voltageDifference_mV )
{
  BMS.setBalancingThresholds(idleTime_min, absVoltage_mV, voltageDifference_mV );
}
void setIdleCurrentThreshold(int current_mA)
{
  BMS.setIdleCurrentThreshold(current_mA);
}

// automatic balancing when battery is within balancing thresholds
void enableAutoBalancing(void)
{
  BMS.enableAutoBalancing();
}
void disableAutoBalancing(void)
{
  BMS.disableAutoBalancing();
}

// battery status
int getBatteryCurrent(void)
{
  return BMS.getBatteryCurrent();
}
int getBatteryVoltage(void)
{
  return BMS.getBatteryVoltage();
}
int getCellVoltage(byte idCell)  // from 1 to 15
{
  return BMS.getCellVoltage(idCell);
}
int getMinCellVoltage(void)
{
  return BMS.getMinCellVoltage();
}
int getMaxCellVoltage(void)
{
  return BMS.getMaxCellVoltage();
}
int getShuntResistorValue()
{
  return BMS.getShuntResistorValue();
}
int getThermistorBetaValue()
{
  return BMS.getThermistorBetaValue();
}
int getTemperatureLimits(int temperatureSetting)
{
  return BMS.getTemperatureLimits(temperatureSetting);
}
float getTemperatureDegC(byte channel)
{
  return BMS.getTemperatureDegC(channel);
}
float getTemperatureDegF(byte channel)
{
  return BMS.getTemperatureDegF(channel);
}

long getShortCircuitProtectionCurrent()
{
  return BMS.getShortCircuitProtectionCurrent();
}

int getShortCircuitProtectionDelay()
{
  return BMS.getShortCircuitProtectionDelay();
}

long getOvercurrentChargeProtectionCurrent()
{
  return BMS.getOvercurrentChargeProtectionCurrent();
}

int getOvercurrentChargeProtectionDelay()
{
  return BMS.getOvercurrentChargeProtectionDelay();
}

long getOvercurrentDischargeProtectionCurrent()
{
  return BMS.getOvercurrentDischargeProtectionCurrent();
}

int getOvercurrentDischargeProtectionDelay()
{
  return BMS.getOvercurrentDischargeProtectionDelay();
}

int getCellUndervoltageProtectionVoltage()
{
  return BMS.getCellUndervoltageProtectionVoltage();
}

int getCellUndervoltageProtectionDelay()
{
  return BMS.getCellUndervoltageProtectionDelay();
}

int getCellOvervoltageProtectionVoltage()
{
  return BMS.getCellOvervoltageProtectionVoltage();
}

int getCellOvervoltageProtectionDelay()
{
  return BMS.getCellOvervoltageProtectionDelay();
}

int getBalancingThresholdTime()
{
  return BMS.getBalancingThresholdTime();
}

int getBalancingThresholdVoltage()
{
  return BMS.getBalancingThresholdVoltage();
}

byte getBalancingThresholdDifference()
{
  return BMS.getBalancingThresholdDifference();
}

int getIdleCurrentThreshold()
{
  return BMS.getIdleCurrentThreshold();
}

// interrupt handling (not to be called manually!)
void setAlertInterruptFlag(void)
{
  BMS.setAlertInterruptFlag();
}

//void resetSOC(int percent)
//{
//  BMS.resetSOC(percent);
//}

//void updateVoltages()
//{
//  BMS.updateVoltages();
//}

//void updateTemperatures()
//{
//  BMS.updateTemperatures();
//}

// If ignoreCCReadFlag == true, the current is read independent of an interrupt
// indicating the availability of a new CC reading
//void updateCurrent(bool ignoreCCReadyFlag)
//{
//  BMS.updateCurrent(ignoreCCReadyFlag);
//}

// sets balancing registers if balancing is allowed (sufficient idle time + voltage)
//byte updateBalancingSwitches(void)
//{
//  BMS.updateBalancingSwitches();
//}


