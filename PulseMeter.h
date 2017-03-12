#ifndef PULSEMETER_H
#define PULSEMETER_H

#include "Arduino.h"
#include <avr/eeprom.h>
#include <EthernetClient.h>
#include "FlashMini.h"

typedef void (*ISR_Function)(void);
extern void _ISR_Meter1_Pulse();
extern void _ISR_Meter2_Pulse();
extern void _ISR_Meter3_Pulse();
extern void _ISR_Meter4_Pulse();



class PulseMeter
{
  public:
    PulseMeter(byte pn, int p, int sid, ISR_Function ISR, byte ee_offset);// constructor
    void Init();                                 // initialize all variables
    void Loop(int m);                            // Called from main loop				
    void CalculateActuals(bool inForceUpdate = false);                     // Convert all counters according to the pulses per unit
    void Status(Print& client);                  // Dump status to ethernet
    void ResetCounters(bool inResetToday, bool inResetWeek = false, bool inResetMonth = false, bool inResetYear = false, bool inResetTotal = false);
    void Save();                                 // Save all counters
    void UpdateCounters(long inNewToday = -1, long inNewWeek = -1, long inNewMonth = -1, long inNewYear = -1, long inNewTotal = -1);
    
    void ResetPowerPeak();                            // reset peak so new peak measurement can start
    void ResetPowerAverage();

        
    long EnergyToday;
    long EnergyWeek;
    long EnergyMonth;
    long EnergyYear;
    long EnergyTotal;
    
    long PowerActual;                            // Actual power measured value in correct units
    long PowerPeak;                            // Actual power measured value in correct units
    long PowerAverage;
    
    bool MeterUpdated;
    
    //long Midnight;                               // The total counter value at the last midnight
    //long Today;                                  // Total for today in correct units. Reset at midnight
    //long Actual;                                 // Actual measured value in correct units
    //long Peak;                                   // Peak value of the last period
    unsigned int SID;                            // System id where this sensor logs to
    byte Type;                                   // Variable of PvOutput to log to. See userdefs.h for explanation  
  
    //unsigned long TotalPulses;
    //unsigned long WattHour;
    //unsigned long Watt;
    //unsigned long WattPeak;  
    void _HandlePulse();    // ISR    

  private:


    bool m_FirstPulse;
    long m_CntTotal;    // lifetime
    long m_CntYear;
    long m_CntMonth;
    long m_CntWeek;   
    long m_CntToday;
    
    bool m_UpdateActuals;
   
    unsigned long m_LastPulseTime;
    unsigned long m_PulseInterval;
    
    long m_PrevEnergyTotal;
    unsigned long m_PrevPowerAverageTime;
  
    byte m_ee_offset;
    
    byte pin;
     
    int  ppu;                                    // the pulses per unit (kWh or m3) for this counter
    //byte ee;                                     // the address to store the day counter in case of a reset
    //byte ee2;                                    // the address to store the total counter in case of a reset
};

#endif



