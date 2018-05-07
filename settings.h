#ifndef SETTINGS_H
#define SETTINGS_H

#define TEMPLIMIT_minDISCHARGE 0
#define TEMPLIMIT_maxDISCHARGE 1
#define TEMPLIMIT_minCHARGE 2
#define TEMPLIMIT_maxCHARGE 3


struct userSettings
{
  unsigned long displayStatusRefresh;
  unsigned long bmsRefreshInt;
  uint8_t oledDisplay;

  int TemperatureLimits[4]; //-20, 45, 0, 45
  int ShuntResistorValue_Ohm; // 5 Resistanc in Ohm

  // Short Circuit Protection
  long SCProtection_mA; //14000
  int SCProtectionDelay; //200 delay in us

  // Over Current Charge Protection
  long OCCProtection_mA; //8000
  int OCCProtectionDelay; // 200 delay in ms

  // Over Current Discharge Protection
  long OCDProtection_mA; // 8000
  int OCDProtectionDelay; // 320 delay in ms

  // Cell Under Voltage Protection
  int CUVProtection_mV; // 2600
  int CUVProtectionDelay; // 2 delay in s

  // Cell Over Voltage Protection
  int COVProtection_mV; //3650
  int COVProtectionDelay; // 2 delay in s

  int BalancingThresholds_idleTime; // 0, 3300, 20 minIdleTime_min, minCellV_mV, maxVoltageDiff_mV
  int BalancingThresholds_minCellV_mV; //0, 3300, 20 minIdleTime_min, minCellV_mV, maxVoltageDiff_mV
  int BalancingThresholds_maxVoltDiff_mV; // 0, 3300, 20 minIdleTime_min, minCellV_mV, maxVoltageDiff_mV
};

extern userSettings settings;

#endif
