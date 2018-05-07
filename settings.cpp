#include <Arduino.h>
#include "bq769x0.h"    // Library for Texas Instruments bq76920 battery management IC
#include "globals.h"
#include "declares.h"
#include "display.h"
#include "settings.h"

//  TODO:
//  - Balancing algorithm
//  - SOC calculation + coulomb counting
/*
int checkStatus();  // returns 0 if everything is OK
void update(void);
void shutdown(void);

// charging control
bool enableCharging(void);
bool disableCharging(void);
bool enableDischarging(void);
bool disableDischarging(void);

// hardware settings
void setShuntResistorValue(int res_mOhm);
void setThermistorBetaValue(int beta_K);

// limit settings (for battery protection)
void setTemperatureLimits(int minDischarge_degC, int maxDischarge_degC, int minCharge_degC, int maxCharge_degC);    // °C
long setShortCircuitProtection(long current_mA, int delay_us = 70);
long setOvercurrentChargeProtection(long current_mA, int delay_ms = 8);
long setOvercurrentDischargeProtection(long current_mA, int delay_ms = 8);
int setCellUndervoltageProtection(int voltage_mV, int delay_s = 1);
int setCellOvervoltageProtection(int voltage_mV, int delay_s = 1);

// balancing settings
void setBalancingThresholds(int idleTime_min = 30, int absVoltage_mV = 3400, byte voltageDifference_mV = 20);
void setIdleCurrentThreshold(int current_mA);

// automatic balancing when battery is within balancing thresholds
void enableAutoBalancing(void);
void disableAutoBalancing(void);

// battery status
int  getBatteryCurrent(void);
int  getBatteryVoltage(void);
int  getCellVoltage(byte idCell);    // from 1 to 15
int  getMinCellVoltage(void);
int  getMaxCellVoltage(void);
float getTemperatureDegC(byte channel = 1);
float getTemperatureDegF(byte channel = 1);

// interrupt handling (not to be called manually!)
void setAlertInterruptFlag(void);
*/

void loadSettings()
{
  
}

void saveSettings()
{
  
}

void readSettings()
{
  
}

void revertSettings()
{
  
}

