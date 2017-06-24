


/**** Defines  ***/

#define WEB_SERVER_PORT    80

#ifdef DBG_WEB_ENABLE
  #define DBG_WEB(d)  {d}
#else
  #define DBG_WEB(d)  
#endif



/**** Local Variables  ***/
EthernetServer s_WebServer(WEB_SERVER_PORT);  // port changed from 80 to 555

extern char s_webData[256];

void WebServerInit()
{
  // start listening
  s_WebServer.begin();
  DBG_WEB(Serial.print("Web: Server listening on port ");Serial.println(WEB_SERVER_PORT);)
}



void WebServerTick()
{
  _WebServerServeClients();
  
}



//String inString = String((char*)"");
//String inString2 = String((char*)"");
static char s_ClientString[128];

void _WebServerServeClients()
{
    EthernetClient client = s_WebServer.available();
    if (client)    
    {
      DBG_WEB(Serial.println("Web: Client connected to server!");)
  //      inString = client.readStringUntil('\n');        
      client.readBytesUntil( '\n', s_ClientString, 128);
      
      client << F("HTTP/1.1 200 OK") << endl;
      client << F("Content-Type: text/html") << endl;
      client << F("Connection: close") << endl << endl;
      if (strstr(s_ClientString, "save")) SaveMeterValues();
      if (strstr(s_ClientString, "reset")) ResetDayCounters();
      if (strstr(s_ClientString, "restart")) while(1); // stay here until the watchdog barks
      if (strstr(s_ClientString, "ntp")) TimeUpdate(); // reload the ntp time
      unsigned char *lvTempStr = (unsigned char *)strstr(s_ClientString, "?");
      if (lvTempStr)
      {
         _WebServerParseUrl(lvTempStr);
      }
      _WebServerShowStatus(client);
      client.stop();
    }
}


void _WebServerParseUrl(unsigned char *inString)
{
   DBG_WEB(Serial.print("Web: Parsing url:");Serial.println((char *)inString);)
   
    // string format is "?0=12345"
    //                  "?3=-12345"
    //                  "?G=12345"
    char lvValueTypes[5]             =  {'D', 'W', 'M' , 'Y', 'T'};
    long lvNewValues[NUMSENSORS][5]  =  {{-1,  -1, -1,   -1,  -1},
                                        {-1,  -1,  -1,   -1,  -1}};

    int  lvValueType = -1;
    int  lvSensorNum = -1;
    long lvTempValue = 0;
    bool lvValueIsSet = false;
    bool lvNegative = false;
        
    while (*inString != '\0')
    {
      if (lvValueType == -1)
      {
        for (int i = 0; i < sizeof(lvValueTypes); i++)
        {
          if (*inString == lvValueTypes[i])
          {
            // found value type
            lvValueType = i;
            DBG_WEB(Serial.print("lvValueType: ");Serial.println(lvValueType);)
            break;
          }
        }
      }
      else
      {
        // value type was found
        if (lvSensorNum == -1)
        {
          if ((*inString - '0') < NUMSENSORS)
          {
              // found sensor number
              lvSensorNum = *inString - '0';
              DBG_WEB(Serial.print("lvSensorNum: ");Serial.println(lvSensorNum);)
          }
          else
          {
             // invalid sensor number, start looking for value type again
             lvValueType = -1;
          }
        }
        else
        {          
          // value type and sensor num were found, start pasing value
          if ((*inString == '&') || (*(inString) == '\0') || (*(inString) == '\n') || (*(inString) == ' '))
          {
             // end of value
             if (lvValueIsSet)
             {
                lvNewValues[lvSensorNum][lvValueType] = (lvNegative ? -1 : 1) * lvTempValue;
                DBG_WEB(Serial.print("Value set: ");Serial.println(lvTempValue);)
             }
             lvValueType = -1;
             lvSensorNum = -1;
             lvTempValue = 0;
             lvValueIsSet = false;
             if (*inString != '&')
             {
               // stop on whitespace
                break; 
             }
          }
          else if ((*inString == '-'))
          {
            lvNegative = true;
          }
          else if ((*inString - '0') < 10)
          {
            lvTempValue *= 10;
            lvTempValue += (*inString - '0');
            lvValueIsSet = true;
            DBG_WEB(Serial.print("lvTempValue: ");Serial.println(lvTempValue);)
          }
        }
      }      
      inString++;
    }
    
    for(int i=0; i<NUMSENSORS; i++)
    {  
//        DBG_WEB(char lvTemp[64];sprintf(lvTemp, "Web: Meters[%d]->UpdateCounters(%u,%u,%u,%u,%u)", i, lvNewValues[i][0], lvNewValues[i][1], lvNewValues[i][2], lvNewValues[i][3], lvNewValues[i][4]);Serial.println(lvTemp);)
        Meters[i]->UpdateCounters(lvNewValues[i][0], lvNewValues[i][1], lvNewValues[i][2], lvNewValues[i][3], lvNewValues[i][4]);
    }
    
} 

void _WebServerShowStatus(EthernetClient client)
{
    const char* br = "<br>";
    client << F("<html><style>td,th {padding:8;text-align:center;}</style>");
    //client << F(VERSION) << br;
    client << F("Current Time: ") << DateTime(now()) <<  br;
    client << F("Uptime: ") << g_upTimeHours/24 << "d+" << g_upTimeHours%24 << "h" << br;  
    client << F("Last NTP update: ") << DateTime(g_TimeOfLastSync) << " (in " << g_TimeNTPRetries << "x)" <<  br;
//    client << F("WD ctr: ") << eeprom_read_byte ((uint8_t*)EE_WDT_CTR) << br;
//    client << F("WD val: ") << eeprom_read_byte ((uint8_t*)EE_WDT_STATE) << br;
//    client << F("Reset Day=") << eeprom_read_byte ((uint8_t*)EE_RESETDAY) << br;
    client << F("millis(): ") << millis() << br;
    client << F("Free RAM: ") << freeRam() << br;

    client << F("<table border=\"1\" cellspacing=\"0\">");
    client << F("<tr><th>Nr<th>pin<th>SID<th>ppu<th>PowerActual<th>PowerPeak<th>PowerAvg<th>m_LastPulseTime<th>m_PulseInterval");
    client << F("<th>EnergyToday<th>EnergyWeek<th>EnergyMonth<th>EnergyYear<th>EnergyTotal");
    client << F("<th>m_CntToday<th>m_CntWeek<th>m_CntMonth<th>m_CntYear<th>m_CntTotal<th>");
    client << F("m_ee_offset<th>EE_CNT_TOTAL<th>EE_CNT_YEAR<th>EE_CNT_MONTH<th>EE_CNT_WEEK<th>EE_CNT_TODAY<th>EE_CNT_CHKSUM</tr>");
    
    
    for(int i=0; i<NUMSENSORS; i++)
    {  
        Meters[i]->CalculateActuals(true);
        client << F("<tr><td>") << i;
        Meters[i]->Status(client);
        client << F("</tr>");
    }
    client << F("</table>PvOutput error: ") << g_PVOutputResponse << " @ " << DateTime(g_PVOutputResponseTime) << br;
    client << F("PvOutput DNS status: ") << g_PVOutputDnsStatus << br;
    client << F("PvOutput Request: ") << br << s_webData << br;

}

int freeRam() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

