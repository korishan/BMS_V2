#ifndef DECLARES_H
#define DECLARES_H

// main
int freeMemory();
void serialPrint(const char* format, ...);

int oledPrint(char *out, const char *format, ...);
static void printchar(char **str, int c);
static int prints(char **out, const char *string, int width, int pad);
static int printi(char **out, int i, int b, int sg, int width, int pad, int letbase);
static int print(char **out, int *varg);


// display
void PrintMenu();
void displayMenu();
void displayStatus();
void displaySysStatus();
void displayVoltage();
void oled_loop();
void printScreen(uint8_t x, uint8_t y, String input);
void setupDisplay();

// serial
void display_help(bool bSerial);
void readSerial();
boolean recvWithEndMarker(char *rcvdChars);
char wordWrap(char *input, int width);

// i2c
void SearchDevices();
void writeTo(uint8_t deviceAddress, uint16_t settingFlag, uint16_t FlagValue);
int16_t readFrom(uint8_t deviceAddress, uint16_t settingFlag, uint16_t numBytes);

// bq769x0
void setBMSDefaults();
int bmsBegin();
void bmsUpdate();
int checkStatus(); // returns 0 if everything is OK
void update(void);
void shutdown(void);
void bmsShutdown(void);
void RebootDevice(void);

// charging control
bool enableCharging(void);
bool disableCharging(void);
bool enableDischarging(void);
bool disableDischarging(void);

// hardware settings
void setShuntResistorValue(int res_mOhm);
void setThermistorBetaValue(int beta_K);

// limit settings (for battery protection)
void setTemperatureLimits(int minDischarge_degC, int maxDischarge_degC, int minCharge_degC, int maxCharge_degC);  // °C
long setShortCircuitProtection(long current_mA, int delay_us );
long setOvercurrentChargeProtection(long current_mA, int delay_ms );
long setOvercurrentDischargeProtection(long current_mA, int delay_ms );
int setCellUndervoltageProtection(int voltage_mV, int delay_s );
int setCellOvervoltageProtection(int voltage_mV, int delay_s );

// balancing settings
void setBalancingThresholds(int idleTime_min, int absVoltage_mV, byte voltageDifference_mV );
void setIdleCurrentThreshold(int current_mA);

// automatic balancing when battery is within balancing thresholds
void enableAutoBalancing(void);
void disableAutoBalancing(void);

// battery status
int getBatteryCurrent(void);
int getBatteryVoltage(void);
int getCellVoltage(byte idCell);  // from 1 to 15
int getMinCellVoltage(void);
int getMaxCellVoltage(void);
int getShuntResistorValue();
int getThermistorBetaValue();
int getTemperatureLimits(int temperatureSetting);
float getTemperatureDegC(byte channel);
float getTemperatureDegF(byte channel);

int getBalancingThresholdTime();
int getBalancingThresholdVoltage();
byte getBalancingThresholdDifference();
long getShortCircuitProtectionCurrent();
int getShortCircuitProtectionDelay();
long getOvercurrentChargeProtectionCurrent();
int getOvercurrentChargeProtectionDelay();
long getOvercurrentDischargeProtectionCurrent();
int getOvercurrentDischargeProtectionDelay();
int getCellUndervoltageProtectionVoltage();
int getCellUndervoltageProtectionDelay();
int getCellOvervoltageProtectionVoltage();
int getCellOvervoltageProtectionDelay();
int getIdleCurrentThreshold();

// interrupt handling (not to be called manually!)
void setAlertInterruptFlag(void);

#endif
