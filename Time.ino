
/**** Defines  ***/
#define TIME_SYNC_TRIES    10  // try to sync 10 times

#ifdef DBG_TIME_ENABLE
  #define DBG_TIME(d)  {d}
#else
  #define DBG_TIME(d)  
#endif

/**** Constants ***/
// NTP Servers:
const char timeServer[] = NTP_SERVER;  // NTP server

const int  timeZone = 1;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message

/**** Global Variables ***/
time_t  g_TimeOfLastSync = 0;
byte    g_TimeNTPRetries = 0;
int     g_upTimeHours = 0;               // the amount of hours the Arduino is running

/**** Local Variables ***/
static EthernetUDP s_UDP;
static byte s_NTPPacketBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
static bool s_DST = false;

static byte s_PrevMinute;
static byte s_PrevHour;
static byte s_PrevDay;
static byte s_PrevMonth;
static int  s_PrevYear;

static byte s_Minute;
static byte s_Hour;
static byte s_Day;
static byte s_Month;
static int  s_Year;

/**** Public Functions ***/
bool TimeInit()
{
  //setSyncInterval(10);    // fast interval for initial time sync
  //setSyncProvider(_TimeSyncNTPTime);
  TimeUpdate();
  bool lvDummy;
  TimeTick(&lvDummy, &lvDummy, &lvDummy, &lvDummy, &lvDummy, &lvDummy);
  g_upTimeHours = 0;  
}

bool TimeUpdate()
{
  byte i;
  // initialize time server
  // Try to set the time 10 times
  DBG_TIME(Serial.println("Time: TimeUpdate");)
  for(i = 1; i <= TIME_SYNC_TRIES; i++)
  {
    unsigned long newTime = _TimeSyncNTPTime();
    if (newTime > 0) // got a valid time, keep it.
    {
        setTime(newTime);
        g_TimeOfLastSync = newTime;
        break;
    }
  }
  g_TimeNTPRetries = i;
  DBG_TIME(Serial.print("Time: TimeUpdate finished after ");Serial.print(g_TimeNTPRetries);Serial.println(" tries");)  
}

unsigned long TimeTick(bool *outMinuteChanged, bool *outHourChanged, bool *outDayChanged, bool *outWeekChanged, bool *outMonthChanged, bool *outYearChanged)
{ 
  unsigned long lvTime = now();
  
  s_Minute= minute(lvTime);
  s_Hour  = hour(lvTime);
  s_Day   = day(lvTime);
  s_Month = month(lvTime);
  s_Year  = year(lvTime);
  
  if (s_Minute != s_PrevMinute)  
  {  
    s_PrevMinute = s_Minute;  
    *outMinuteChanged = true;  
  }  
  else  
  { 
    *outMinuteChanged = false;  
  }
  if (s_Hour != s_PrevHour)      
  {  
     s_PrevHour = s_Hour;      
     *outHourChanged = true;    
     g_upTimeHours++;
  }  
  else  
  { 
    *outHourChanged = false;  
  }
  if (s_Day != s_PrevDay)
  {  
    s_PrevDay = s_Day;        
    *outDayChanged = true;     
    if (weekday(lvTime) == 2)    // new week starts on monday
    {
      *outWeekChanged = true;
    }
    else
    {
      *outWeekChanged = false;
    }
  }  
  else  
  { 
    *outDayChanged = false;
    *outWeekChanged = false;
  }
  if (s_Month != s_PrevMonth)    
  {  
    s_PrevMonth = s_Month;    
    *outMonthChanged = true;   
  }  
  else  
  { 
    *outMonthChanged = false;  
  }
  if (s_Year != s_PrevYear)      
  {  
    s_PrevYear = s_Year;      
    *outYearChanged = true;    
  }  
  else  
  { 
    *outYearChanged = false;  
  }
 
  if (!TimeIsValid())
  {
     // have not received a valid time yet, retry every minute
     if (*outMinuteChanged)
     {
       TimeUpdate();
     }
  }
  else
  {
     // have received a valid time, update twice a day at 3:00 and 15:00
     if(*outHourChanged)
     {
        // sync the time at fixed interval
        if(s_Hour == 3 || s_Hour == 15)
        {
            TimeUpdate();
        }
     }
  }
  
  return lvTime;
}

bool TimeIsValid()
{
  return ((timeStatus() != timeNotSet) && (year() > 2000));
}

char dt[20];
char* DateTime(time_t t)
{
    //int y = year(t) - 2000;
    //if(y < 0) sprintf(dt,"Never");
    //else
    sprintf(dt, "%02d-%02d-%04d  %02d:%02d:%02d", day(t),month(t),year(t),hour(t),minute(t),second(t));
    return dt;
}

/**** Private Functions ***/
static unsigned long _TimeSyncNTPTime()
{
  DBG_TIME(Serial.println("Time: _TimeSyncNTPTime");)
  unsigned long lvTime = _TimeSendNTPRequest();
  if(lvTime != 0)
  {
    DBG_TIME(Serial.print("Time: recieved NTP time: ");Serial.println(lvTime);)
    // Convert to local time   
    // DST == DaySavingTime == Zomertijd
    boolean dst = false;
    int m = month(lvTime);
    int previousSunday = day(lvTime) - weekday(lvTime) + 1;  // add one since weekday starts at 1
    s_DST = !(m < 3 || m > 10); // between october and march
    if (s_DST) 
    {
        if (m == 3) 
        {
            //  starts last sunday of march
            s_DST = previousSunday >= 25;
        } 
        else if (m == 10) 
        {
            // ends last sunday of october
            s_DST = previousSunday < 25;
        }
    }    
    DBG_TIME(Serial.print("Time: DST is ");Serial.println((s_DST ? "ON" : "OFF"));)
    lvTime += (s_DST ? (timeZone+1) : timeZone) * SECS_PER_HOUR; // CEST or CET
    lvTime += TIME_OFFSET;
  }
  else
  {
     DBG_TIME(Serial.println("Time: NTP failure!");)
  }

  return lvTime;
}

// send an NTP request to the time server
static unsigned long _TimeSendNTPRequest()
{
    unsigned long lvTime = 0;

    DBG_TIME(Serial.println("Time: _TimeSendNTPRequest");)
  
    s_UDP.begin(8888);    // open socket on arbitrary port
    
    while(s_UDP.parsePacket())
    {
      s_UDP.flush();   // make sure udp buffer is empty
    }
    
    // set all bytes in the buffer to 0
    memset(s_NTPPacketBuffer, 0, NTP_PACKET_SIZE); 
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    s_NTPPacketBuffer[0] = 0b11100011;   // LI, Version, Mode
    s_NTPPacketBuffer[1] = 0;     // Stratum, or type of clock
    s_NTPPacketBuffer[2] = 6;     // Polling Interval
    s_NTPPacketBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    s_NTPPacketBuffer[12]  = 49; 
    s_NTPPacketBuffer[13]  = 0x4E;
    s_NTPPacketBuffer[14]  = 49;
    s_NTPPacketBuffer[15]  = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp: 		   
    if (( s_UDP.beginPacket(timeServer, 123) // 123 is the NTP port
	 && s_UDP.write(s_NTPPacketBuffer, NTP_PACKET_SIZE) == NTP_PACKET_SIZE
	 && s_UDP.endPacket()))
    {
      // Wait for response; check every pollIntv ms up to maxPoll times
      byte maxPoll = 10;		// poll up to this many times
      const int pollIntv = 150;		// poll every this many ms
      int pktLen;				// received packet length
      while(maxPoll-- > 0)
      {
        if ((pktLen = s_UDP.parsePacket()) == NTP_PACKET_SIZE)
        {
          break;
        }
        delay(maxPoll);        // poll every this many ms
      }
      
      if (pktLen == NTP_PACKET_SIZE)
      {
        s_UDP.read(s_NTPPacketBuffer, NTP_PACKET_SIZE);  // read the packet into the buffer

        DBG_TIME(
          Serial.print("Time: NTP Response: ");
          for (int i = 0; i < NTP_PACKET_SIZE; i++)
          {
             Serial.print(s_NTPPacketBuffer[i], HEX);
             Serial.print(" ");
          }
          Serial.println("");)
        
        // the timestamp starts at byte 40 of the received packet and is four bytes, or two words
        lvTime = ((unsigned long)word(s_NTPPacketBuffer[40], s_NTPPacketBuffer[41]) << 16) | (word(s_NTPPacketBuffer[42], s_NTPPacketBuffer[43]));
    
        // check for invalid response
        if (lvTime > 2208988800ul)
        {
          // Round to the nearest second if we want accuracy
          // The fractionary part is the next byte divided by 256: if it is
          // greater than 500ms we round to the next second; we also account
          // for an assumed network delay of 50ms, and (0.5-0.05)*256=115;
          // additionally, we account for how much we delayed reading the packet
          // since its arrival, which we assume on average to be pollIntv/2.          
          lvTime += (s_NTPPacketBuffer[44] > 115 - pollIntv/8);
          
          lvTime -= 2208988800ul;  // Convert epoch time (secs since 1900) to unix time (seconds since 1970)
        }
      }
      else
      {
        DBG_TIME(
          Serial.print("Time: invalid NTP response (");
          Serial.print(pktLen);
          Serial.println(" bytes)");
          )
      }
    }
    else
    {
       DBG_TIME(Serial.println("Time: UDP send failure");)
    }
    // Discard any bytes left in the buffer
    s_UDP.flush(); 
    s_UDP.stop();
  
    return lvTime;
}



