#ifndef CONFIG_H
#define CONFIG_H


// Debug messages
//#define DBG_ENABLE

#ifdef DBG_ENABLE
//  #define DBG_TIME_ENABLE
//  #define DBG_WEB_ENABLE
//  #define DBG_PVO_ENABLE
  #define DBG_METER_ENABLE
#endif

//*****************************************************************
// Network variables
static byte EthMAC[]       = { 0x90, 0xA2, 0xDA, 0x10, 0x88, 0xC4 }; // MAC address can be any number, as long as it is unique in your local network
static byte EthIPAddress[] = { 192, 168, 0, 199};                    // IP of arduino
static byte EthDNS[]       = { 192, 168, 0, 1};                      // use the address of your gateway { 192, 168, 1, 1 } if your router supports this
static byte EthGateWay[]   = { 192, 168, 0, 1 };
static byte EthSubnet[]    = { 255, 255, 255, 0 };  


//*****************************************************************
#define NTP_SERVER "nl.pool.ntp.org"                             // If you are having problems with the time synchonisation, try a different NTP server

//*****************************************************************
// You can find your api-key in the PvOutput settings page, under api-settings
#define PVOUTPUT_API_KEY        "4ea0bb4c35d3628dc4708feb20f08f2e3f09aa2b"
#define PVOUTPUT_SYSTEM_ID      48034

//*****************************************************************
// The update interval must match what you have set in the PvOutput settings
// PvOutput->Settings->System->Live settings->Status interval
// Default is 5 minutes
#define UPDATEINTERVAL 5

//*****************************************************************
// The actual time can be shifted to move the time of uploading to pvoutput and exosite
// This is to prevent missing uploads because everyone is uploading at exactly the same time
// Offset is in seconds, positive numbers will upload earlier
// A negative number will delay the upload by the amount of seconds set here
#define TIME_OFFSET 0

//*****************************************************************
// Enable the watchdog only if you know the bootloader can handle this.
#define USE_WD

//*****************************************************************
// Sensor configuration
//*****************************************************************
// NUMSENSORS must match the number of sensors defined.
#define NUMSENSORS 2

#define METER1_PIN    7
#define METER2_PIN    3
#define METER3_PIN    2
#define METER4_PIN    0

// EEPROM
#define EE_WDT_CTR     0
#define EE_WDT_STATE   1

#define EE_METER_DATA_START     (10*4)
#define EE_METER_DATA_SIZE      (10*4)    // reserve room for 10 32-bit values

#define EE_METER_OFFSET(m)    (EE_METER_DATA_START+(m*EE_METER_DATA_SIZE))

//*****************************************************************
// S0 sensors have 5 parameters: 
//   1: The digital pin to which they are connected.
//   2: The number of pulses for 1 kWh
//   3: The System ID of the corresponding pvOutput graph
//   4: The number of the variable to log to (see software manual)
//   5: The x-factor. The actual and total values will be divided by this number before  to pvoutput
PulseMeter  MeterConsumption(METER1_PIN, 100, PVOUTPUT_SYSTEM_ID, _ISR_Meter1_Pulse, EE_METER_OFFSET(0));   // S0 sensor connected to pin 2, logging to variable 1 & 2 (production) of sid 2222
PulseMeter  MeterProduction (METER2_PIN, 100, PVOUTPUT_SYSTEM_ID, _ISR_Meter2_Pulse, EE_METER_OFFSET(1));   // S0 sensor connected to pin 3, logging to variable 1 & 2 (production) of sid 2222. This will be added to S1


//*****************************************************************
// the next list must be in the correct order and have the same length as NUMSENSORS
PulseMeter* Meters[NUMSENSORS] = {&MeterConsumption, &MeterProduction};

#endif

