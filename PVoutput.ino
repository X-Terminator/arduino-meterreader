
/**** Defines ***/
#define PVOUTPUT_API_KEY    "4ea0bb4c35d3628dc4708feb20f08f2e3f09aa2b"
#define PVOUTPUT_SYSTEM_ID  48034

#ifdef DBG_PVO_ENABLE
  #define DBG_PVO(d)  {d}
#else
  #define DBG_PVO(d)  
#endif

/**** Global Variables ***/
char g_PVOutputResponse[80];
time_t g_PVOutputResponseTime;
int g_PVOutputDnsStatus = 0;
IPAddress g_PVOutputIP;

/**** Local Variables ***/
char s_webData[256];

// This function updates all registered sensors to pvoutput
void PVOutputSend()
{
  EthernetClient pvout;
  // create a total for each variable that can be used in pvoutput
  // !! The index in this array starts at 0 while the pvoutput vars start at 1
 /* float v[12]; // data sum
  bool b[12]; // data present flags
  // start with 0
  for(byte n = 0; n < 12; n++)
  { 
    v[n] = 0;
    b[n] = false;
  }
  */
  
  DBG_PVO(Serial.println("PVOutputSend");)
  
  _PVOutputResolveIP(); // update the ipaddress via DNS
  
  if (g_PVOutputDnsStatus == 1)
  {
    unsigned int sid = PVOUTPUT_SYSTEM_ID;
  
    unsigned long lvWattProd, lvWattCons, lvWattHourProd, lvWattHourCons;
    time_t lvTime = now() - 30;  // 30 sec offset

     DBG_PVO(Serial.println("PVOutputSend: DNS resolve success!");)
    
    int res = pvout.connect(g_PVOutputIP, 80);
    if(res == 1) // connection successfull
    {
      DBG_PVO(Serial.println("PVOutputSend: Connection success!");)
      char *lvBuf = s_webData;

      //pvout << F("GET /service/r2/addstatus.jsp");
      lvBuf += sprintf(lvBuf, "GET /service/r2/addstatus.jsp");
      //pvout << F("?key=" PVOUTPUT_API_KEY);
      lvBuf += sprintf(lvBuf, "?key=%s", PVOUTPUT_API_KEY);
//      pvout << F("&sid=") << sid;
      lvBuf += sprintf(lvBuf, "&sid=%u", sid);
//      sprintf(s_webData, "&d=%04d%02d%02d", year(lvTime),month(lvTime),day(lvTime));
      lvBuf += sprintf(lvBuf, "&d=%04d%02d%02d", year(lvTime),month(lvTime),day(lvTime));
//      pvout << s_webData;
//      sprintf(s_webData, "&t=%02d:%02d", hour(lvTime),minute(lvTime));
      lvBuf += sprintf(lvBuf, "&t=%02d:%02d", hour(lvTime),minute(lvTime));
//      pvout << s_webData;
      
//      pvout << "&v1=" << MeterProduction.EnergyToday;      // v1: Energy Generation
      lvBuf += sprintf(lvBuf, "&v1=%ld", MeterProduction.EnergyToday); // v1: Energy Generation
//      pvout << "&v2=" << MeterProduction.PowerPeak;        // v2: Power Generation
      lvBuf += sprintf(lvBuf, "&v2=%ld", MeterProduction.PowerPeak); // v2: Power Generation
//      pvout << "&v3=" << MeterConsumption.EnergyToday;     // v3: Energy Consumption
      lvBuf += sprintf(lvBuf, "&v3=%ld", MeterConsumption.EnergyToday); // v3: Energy Consumption
//      pvout << "&v4=" << MeterConsumption.PowerAverage;    // v4: Power Consumption    
      lvBuf += sprintf(lvBuf, "&v4=%ld", MeterConsumption.PowerAverage); // v4: Power Consumption
      
//      pvout << endl << F("Host: pvoutput.org") << endl << endl;
      lvBuf += sprintf(lvBuf, "\nHost: pvoutput.org\n\n");
      pvout << s_webData;
      
      // give pvoutput some time to process the request
      delay(500);
      
      // read the response code. 200 means ok. 0 means that there is no response yet
      byte lastResponse = pvout.parseInt();
      if(lastResponse == 0)
      {
        DBG_PVO(Serial.println("PVOutputSend: Response timeout!");)
        sprintf(g_PVOutputResponse,"Response timeout\0");
        g_PVOutputResponseTime = now();
      }
      else if(lastResponse != 200)
      { 
        DBG_PVO(Serial.println("PVOutputSend: Response success!");)        
        sprintf(g_PVOutputResponse, "%03d",lastResponse);
        size_t numchars = pvout.readBytes(g_PVOutputResponse+3, 80); 
        g_PVOutputResponse[numchars+3] = 0; // terminate the string
        g_PVOutputResponseTime = now();
      }
      pvout.stop();
    
    }
    else // cannnot connect
    {
      DBG_PVO(Serial.println("PVOutputSend: Connection failed!");)
      sprintf(g_PVOutputResponse, "No connection\0");
      g_PVOutputResponseTime = now();
    }
  }
  else
  {
     DBG_PVO(Serial.println("PVOutputSend: DNS resolve failed!");) 
  }
}

// This function will contact the DNS server and ask for an IP address of PvOutput
// If successfull, this address will be used
// If not, keep using the previous found address
// In this way, we can still update to pvoutput if the dns timeouts.
void _PVOutputResolveIP()
{
  // Look up the host first
  DNSClient dns;
  IPAddress remote_addr;

  dns.begin(Ethernet.dnsServerIP());
  g_PVOutputDnsStatus = dns.getHostByName((char*)"pvoutput.org", remote_addr);
  if (g_PVOutputDnsStatus == 1) 
  {
    g_PVOutputIP = remote_addr; // if success, copy
  }
}

