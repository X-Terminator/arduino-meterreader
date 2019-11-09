
#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <SPI.h>
#include <Dns.h>
#include <TimeLib.h>
//#include <MsTimer2.h>
//#include <avr/wdt.h>
///#include <utility/w5500.h>

#include "PulseMeter.h"
#include "FlashMini.h"
#include "Config.h"

bool s_ButtonWasPressed = false;

static bool s_PrevTimeWasValid = false;

    
void setup() {
  // put your setup code here, to run once:
   
   #ifdef DBG_ENABLE
    Serial.begin(9600);
    while (!Serial) ; // Needed for Leonardo only
  #endif
  
  DisplayInit();
  Serial1.write("D");
  ButtonInit();
  Serial1.write("B");
  
  delay(300);
  Ethernet.begin(EthMAC, EthIPAddress, EthGateWay, EthDNS, EthSubnet);
  Serial1.write("E");
  
  // set connect timeout parameters
  w5500.setRetransmissionTime(2000); // 2000ms per try
  w5500.setRetransmissionCount(8);
    
  TimeInit();
  Serial1.write("T");
  s_PrevTimeWasValid = TimeIsValid();
  
  WebServerInit();
  Serial1.write("W");
  
  // initialize the meters
  for(byte i = 0; i < NUMSENSORS; i++)
  {
      Serial.println("Meter Init");
      Meters[i]->Init();
      Serial1.print(i);
  }

  //SetupWatchdog();
  // start the timer interrupt
 // MsTimer2::set(5, Every5ms); // 5ms period
  //MsTimer2::start();
  DisplayUpdate();
}

// check and update all WDT every 5ms.
/*void Every5ms()
{
  CheckWatchdog();
}*/

void loop() 
{

  bool lvMinuteChanged, lvHourChanged, lvDayChanged, lvWeekChanged, lvMonthChanged, lvYearChanged;
  unsigned long lvTime = TimeTick(&lvMinuteChanged, &lvHourChanged, &lvDayChanged, &lvWeekChanged, &lvMonthChanged, &lvYearChanged);
  bool lvNewButtonPress = false;
  
//  busy(1);
  for(byte i = 0; i < NUMSENSORS; i++)
  {
    Meters[i]->CalculateActuals(false);
  }
   
  if (TimeIsValid())
  {
    if (s_PrevTimeWasValid)
    {   
//       busy(3);   
      // update every minute
      if (lvMinuteChanged)
      {
        // update every 5 minutes or whatever is set in userdefs
        if((minute(lvTime) % UPDATEINTERVAL) == 0)
        {
            PVOutputSend();
//            busy(31);
            // reset the maximum for pvoutput
            for(byte i = 0; i < NUMSENSORS; i++)
            {
                Meters[i]->ResetPowerPeak();
                Meters[i]->ResetPowerAverage();                
            }
        }
      }  
      // day change
      if (lvDayChanged)
      {
//         busy(32);
         // reset counter(s) 
         for(byte i = 0; i < NUMSENSORS; i++)
         {
           Meters[i]->ResetCounters(lvDayChanged, lvWeekChanged, lvMonthChanged, lvYearChanged, false);
         } 
      }
    
      s_PrevTimeWasValid = true;
    }
  }
  
  // hour has changed   
  if(lvHourChanged && (minute(lvTime) == 0))
  {
//    busy(2);
    
    // save the counter values every hour
    SaveMeterValues();
  }

  if (ButtonIsPressed())
  {
//    busy(4);
    if (!s_ButtonWasPressed)
    {
      s_ButtonWasPressed = true;
      lvNewButtonPress = true;
    }
  }
  else
  {
    s_ButtonWasPressed = false;
  }
  
//  busy(5);
  DisplayTick(lvNewButtonPress);
  
//  busy(6);
  WebServerTick();
  
//  busy(0);
  delay(100);
}


void SaveMeterValues()
{
  for(byte i = 0; i < NUMSENSORS; i++)
  {
     Meters[i]->Save();
  }
}

void ResetDayCounters()
{
  for(byte i = 0; i < NUMSENSORS; i++)
  {
     Meters[i]->ResetCounters(true, false, false, false, false);
  }
}
