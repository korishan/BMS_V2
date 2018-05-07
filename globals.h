#ifndef GLOBALS_H
#define GLOBALS_H

#define BMS_ALERT_PIN 2     // attached to interrupt INT0
#define BMS_BOOT_PIN 7      // connected to TS1 input
#define BMS_I2C_ADDRESS 0x18

// A global variable for storing the internal voltage reference of the 1.1V rail of the chip.
// This value is used for internal calibrations of the ADC of internal ADC. Not to be used with external ADC's
extern long internalVCC;

#endif


