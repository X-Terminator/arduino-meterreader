#include <Time.h>

#include "PulseMeter.h"
//#include "config.h"

#define EE_CNT_TOTAL  0
#define EE_CNT_YEAR   4
#define EE_CNT_MONTH  8
#define EE_CNT_WEEK   12
#define EE_CNT_TODAY  16
#define EE_CNT_CHKSUM 20

//#ifdef DBG_METER_ENABLE
//  #define DBG_METER(d)  {d}
//#else
  #define DBG_METER(d)  
//#endif


extern PulseMeter* Meters[];


/*
PulseMeter::PulseMeter(byte pn, int p, int sid, ISR_Function ISR, byte ee_offset) {}
void PulseMeter::Init() {}
void PulseMeter::Loop(int m) {}
void PulseMeter::CalculateActuals(bool inForceUpdate) {}
void PulseMeter::Status(Print& client) {}
void PulseMeter::ResetCounters(bool inResetToday, bool inResetWeek , bool inResetMonth , bool inResetYear, bool inResetTotal) {}
void PulseMeter::Save() {}
void PulseMeter::UpdateCounters(long inNewToday, long inNewWeek, long inNewMonth, long inNewYear, long inNewTotal) {}
void PulseMeter::ResetPowerPeak() {}
void PulseMeter::ResetPowerAverage() {}
void PulseMeter::_HandlePulse() {}
*/


PulseMeter::PulseMeter(byte pn, int p, unsigned long sid, ISR_Function ISR, byte ee_offset)
{
  ppu = p;
  SID = sid;
  pin = pn;
  m_ee_offset = ee_offset;

  pinMode(pin, INPUT_PULLUP);
  //delay(200);
  attachInterrupt(digitalPinToInterrupt(pin), ISR, FALLING);
}

void PulseMeter::Init()
{
  PowerActual = 0;
  PowerPeak = 0;
  EnergyToday = 0;

  //ee = (i+20) * 4; // the eeprom address of this sensor where the last value is saved
  //todayCnt = eeprom_read_dword((uint32_t*) ee);
  //ee2 = (i+40) * 4; // the eeprom address for the total counters
  //Midnight = eeprom_read_dword((uint32_t*) ee2);
  m_PulseInterval = 0;
  m_LastPulseTime = 0;
  m_FirstPulse = true;


  // restore counters from EEPROM
  m_CntTotal = eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_TOTAL));
  m_CntYear  = eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_YEAR));
  m_CntMonth = eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_MONTH));
  m_CntWeek  = eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_WEEK));
  m_CntToday = eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_TODAY));

  DBG_METER(Serial.println("Init: Load counters:");Serial.println(m_CntTotal);Serial.println(m_CntYear);Serial.println(m_CntWeek);Serial.println(m_CntToday);)
  // validate counter checksum
  unsigned long lvChecksum = 0;
  lvChecksum = (m_CntTotal + m_CntYear + m_CntMonth + m_CntWeek + m_CntToday);
  if (lvChecksum != eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_CHKSUM)))
  {
    DBG_METER(Serial.println("Init: checksum invalid!");Serial.println(lvChecksum);Serial.println(eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_CHKSUM)));)
    // EEPROM checksum invalid! reset counters to 0
    ResetCounters(true, true, true, true, true);
    Save();
  }
  
  CalculateActuals(true);
  ResetPowerAverage();
//  m_UpdateActuals = true;
}

void PulseMeter::Loop(int m)
{
  // Derived sensors can execute non time critical actions here
}

void PulseMeter::Save()
{
  DBG_METER(Serial.println("PulseMeter: Save");)
  //eeprom_write_dword((uint32_t*) ee, todayCnt);
  // save counters to EEPROM
  eeprom_write_dword((uint32_t*) (m_ee_offset + EE_CNT_TOTAL), m_CntTotal);
  eeprom_write_dword((uint32_t*) (m_ee_offset + EE_CNT_YEAR),  m_CntYear);
  eeprom_write_dword((uint32_t*) (m_ee_offset + EE_CNT_MONTH), m_CntMonth);
  eeprom_write_dword((uint32_t*) (m_ee_offset + EE_CNT_WEEK),  m_CntWeek);
  eeprom_write_dword((uint32_t*) (m_ee_offset + EE_CNT_TODAY), m_CntToday);

  // write counter checksum
  unsigned long lvChecksum = 0;
  lvChecksum = (m_CntTotal + m_CntYear + m_CntMonth + m_CntWeek + m_CntToday);
  eeprom_write_dword((uint32_t*) (m_ee_offset + EE_CNT_CHKSUM), lvChecksum);
}

void PulseMeter::UpdateCounters(long inNewToday, long inNewWeek, long inNewMonth, long inNewYear, long inNewTotal)
{
  DBG_METER(Serial.println("PulseMeter: UpdateCounters");)
  if (inNewTotal != -1)
  {
    m_CntTotal = inNewTotal;
  }
  if (inNewYear != -1)
  {
    m_CntYear = inNewYear;
  }
  if (inNewMonth != -1)
  {
    m_CntMonth = inNewMonth;
  }
  if (inNewWeek != -1)
  {
    m_CntWeek = inNewWeek;
  }
  if (inNewToday != -1)
  {
    m_CntToday = inNewToday;
  }
  Save();
//  m_UpdateActuals = true;
}

void PulseMeter::ResetCounters(bool inResetToday, bool inResetWeek, bool inResetMonth, bool inResetYear, bool inResetTotal)
{
   DBG_METER(Serial.println("PulseMeter: ResetCounters");)
 
  if (inResetTotal)
  {
    m_CntTotal = 0;
  }
  if (inResetYear)
  {
    m_CntYear = 0;
  }
  if (inResetMonth)
  {
    m_CntMonth = 0;
  }
  if (inResetWeek)
  {
    m_CntWeek = 0;
  }
  if (inResetToday)
  {
    m_CntToday = 0;
  }
//  m_UpdateActuals = true;
}

void PulseMeter::ResetPowerPeak()
{
  PowerPeak = 0;
}

void PulseMeter::ResetPowerAverage()
{
  PowerAverage = 0;
  m_PrevEnergyTotal = EnergyTotal;
  m_PrevPowerAverageTime = now();
}


void PulseMeter::CalculateActuals(bool inForceUpdate)
{

    unsigned long lvTimeSinceLastPulse = millis() - m_LastPulseTime;
    // Was the last PulseMeter pulse more than 50 minutes ago?
    if (lvTimeSinceLastPulse > (3000000)) // no pulse for 50+ minutes (< 12 Watt)
    {
      PowerActual = 0;  // then we have no output
      m_PulseInterval = 0;
      m_FirstPulse = true;  // next pulse will be first pulse again
      MeterUpdated = true;
    }
    else if ((PowerActual > 0) && (lvTimeSinceLastPulse > m_PulseInterval))
    {
      m_PulseInterval = lvTimeSinceLastPulse;  // shift interval
//      m_UpdateActuals = true;
    }

//  if (m_UpdateActuals || inForceUpdate)  // has anything changed?
//  {
//    m_UpdateActuals = false;
    if (m_PulseInterval != 0) // prevent division by zero
    {
      // convert to W
      PowerActual = 3600000000 / m_PulseInterval;
      PowerActual /= ppu;
      if (PowerActual > 9999)
      {
        //clip
        PowerActual = 9999;
      }
      else
      {
        if (PowerPeak < abs(PowerActual))
        {
          PowerPeak = PowerActual;
        }
      }
    }
    // convert to Wh
    EnergyToday = m_CntToday * 1000 / ppu;
    EnergyWeek  = m_CntWeek * 1000 / ppu;
    EnergyMonth = m_CntMonth * 1000 / ppu;
    EnergyYear  = m_CntYear * 1000 / ppu;
    EnergyTotal = m_CntTotal * 1000 / ppu;
    
    // calculate power average
    if (now() > m_PrevPowerAverageTime)
    {
      if (EnergyTotal < m_PrevEnergyTotal)
      {
        // counter must have been changed: reset average power
        ResetPowerAverage();
      }
      else
      {
        PowerAverage = ((EnergyTotal - m_PrevEnergyTotal) * SECS_PER_HOUR) / (now() - m_PrevPowerAverageTime);
      }
    }
    
//    MeterUpdated = true;
//  }
}

void PulseMeter::Status(Print& client)
{
  const char* td = "<td>";
  client << td << pin;  
  client << td << SID;
  client << td << ppu;

  client << td << PowerActual;
  client << td << PowerPeak;
  client << td << PowerAverage;

  client << td << m_LastPulseTime;
  client << td << m_PulseInterval;  

  client << td << EnergyToday;
  client << td << EnergyWeek;
  client << td << EnergyMonth;
  client << td << EnergyYear;
  client << td << EnergyTotal;

  client << td << m_CntToday;
  client << td << m_CntWeek;
  client << td << m_CntMonth;
  client << td << m_CntYear;
  client << td << m_CntTotal;
    
  client << td << m_ee_offset;
  
  client << td << (eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_TOTAL)));
  client << td << (eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_YEAR)));
  client << td << (eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_MONTH)));
  client << td << (eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_WEEK)));
  client << td << (eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_TODAY)));
  client << td << (eeprom_read_dword((uint32_t*) (m_ee_offset + EE_CNT_CHKSUM)));
}

void PulseMeter::_HandlePulse()
{
  unsigned long lvMilliTime = millis();

  if (m_FirstPulse)
  {
    m_FirstPulse = false;
  }
  else
  {
    m_PulseInterval = lvMilliTime - m_LastPulseTime;
  }
  m_LastPulseTime = lvMilliTime;

  // increment all counters
  m_CntToday++;
  m_CntWeek++;
  m_CntMonth++;
  m_CntYear++;
  m_CntTotal++;

//  m_UpdateActuals = true;
}

//*** ISR routines **/
void _ISR_Meter1_Pulse()
{
  Meters[0]->_HandlePulse();
}
void _ISR_Meter2_Pulse()
{
  Meters[1]->_HandlePulse();
}
void _ISR_Meter3_Pulse()
{
  Meters[2]->_HandlePulse();
}
void _ISR_Meter4_Pulse()
{
  Meters[3]->_HandlePulse();
}
