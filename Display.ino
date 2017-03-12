/**** Defines  ***/
#define BACKLIGHT_OFF  1
#define BACKLIGHT_ON   4

#define CUR_LINE0    0x00
#define CUR_LINE1    0x40
#define CUR_LINE2    0x14
#define CUR_LINE3    0x54

#define DISPLAY_TIMEOUT    60UL    // seconds

#define STATE_UNKNOWN     0
#define STATE_DISP_NORMAL 1
#define STATE_DISP_WEEK   2
#define STATE_DISP_MONTH  3 
#define STATE_DISP_YEAR   4
#define STATE_DISP_TOTAL  5
#define STATE_DISP_DEBUG  6

/**** Local Variables ***/
static byte s_DisplayState = STATE_DISP_NORMAL;
static byte s_CursorLineOffset[4] = {CUR_LINE0,CUR_LINE1,CUR_LINE2,CUR_LINE3};
static unsigned long s_DisplayOnTime;
static bool s_DisplayIsOn = false;

/**** Public Functions ***/
void DisplayInit()
{
  Serial1.begin(9600);

  DisplayClear();
  DisplayOn();
  DisplayBacklight(BACKLIGHT_ON);
  DisplayContrast(45);  
  DisplayCursor(CUR_LINE0);
  Serial1.write("Loading...");
}


void DisplayTick(bool inNewButtonPress)
{
  // handle button press
  if (inNewButtonPress)
  {
    if(s_DisplayIsOn)
    {
      s_DisplayState++;
     if (s_DisplayState > STATE_DISP_DEBUG)
     {
        s_DisplayState = STATE_DISP_NORMAL;
     }
    }
    else
    {
      // display was off
      s_DisplayState = STATE_DISP_NORMAL;
    }
    DisplayOn();
  }
  
  if (s_DisplayIsOn)
  {
    if (((millis() - s_DisplayOnTime) >= (DISPLAY_TIMEOUT*1000)))
    {
      // display timeout, turn off
      DisplayOff();
    }
    else
    {
      DisplayUpdate();
    }
  }
}

void DisplayOn()
{
  // turn display on
  Serial1.write(0xFE);
  Serial1.write(0x41);  
  
  // turn backlight on
  DisplayBacklight(BACKLIGHT_ON);
  
  s_DisplayOnTime = millis();
  s_DisplayIsOn = true;
}
void DisplayOff()
{
  // turn backlight off
  DisplayBacklight(BACKLIGHT_OFF);
  
  // turn display off
  Serial1.write(0xFE);
  Serial1.write(0x42); 
  
  s_DisplayIsOn = false;  
}

/**** Private Functions ***/
void DisplayWritePaddedValue(byte inTotalLen, long inValue, byte inPadChar, byte inSign = 0)
{
  unsigned long temp = 1;  
  if (inSign)
  {
    inTotalLen--;
  }
  while (inTotalLen > 1)
  {
    temp *= 10;
    inTotalLen--;
  }
  while ((temp > 1) && (abs(inValue) < temp))
  {
    Serial1.write(inPadChar);
    temp /= 10;
  }
  if (inSign)
  {
    if (inSign == 1) // auto sign
    {
      Serial1.write((inValue < 0) ? "-" : "+");
    }
    else
    {
      // force sign
      Serial1.write(inSign);
    }  
  }
  Serial1.print(abs(inValue));
}

void DisplayMeterValues(long inPower, long inEnergy, bool inAddSign = false)
{
  DisplayWritePaddedValue(5, inPower, ' ', inAddSign);    // Watt
  inEnergy = (inEnergy + (inEnergy < 0 ? -5 : 5)) / 10;
  Serial1.write("W");
  DisplayWritePaddedValue(3, (inEnergy / 100), ' ', (inAddSign ? (inEnergy < 0 ? '-' : '+') : 0));    // kWh    
  Serial1.write(".");
  inEnergy = abs(inEnergy) % 100;
  DisplayWritePaddedValue(2, inEnergy, '0');    // Wh*100
  Serial1.write("kWh");
}

void DisplayMeterTotalValue(long inEnergy, bool inAddSign = false)
{
  DisplayWritePaddedValue(5, (inEnergy / 1000), ' ', (inAddSign ? (inEnergy < 0 ? '-' : '+') : 0));    // kWh    
  Serial1.write(".");
  DisplayWritePaddedValue(3, (inEnergy%1000), '0');    // Wh
  Serial1.write("kWh");
}

void DisplayUpdate()
{
  static byte s_PrevState = STATE_UNKNOWN;
  static time_t  s_PrevTime;

  bool lvDoRefresh = false;
  if (s_PrevState != s_DisplayState)
  {
    DisplayClear();
    lvDoRefresh = true;
    s_PrevState = s_DisplayState;
  }
  time_t lvTime = now();
  if (lvTime != s_PrevTime)
  {
    s_PrevTime = lvTime;
    lvDoRefresh = true;
  }
  if (lvDoRefresh)
  {
    if (s_DisplayState == STATE_DISP_NORMAL)
    {
        unsigned long lvWattProd, lvWattCons, lvWattHourProd, lvWattHourCons, lvTemp;
        signed long lvWattNet, lvWattHourNet;
  
        
        lvWattProd = MeterProduction.PowerActual;
        lvWattHourProd = MeterProduction.EnergyToday;
  
        lvWattCons = MeterConsumption.PowerActual;
        lvWattHourCons = MeterConsumption.EnergyToday;
        
        lvWattNet = (signed long)lvWattCons - lvWattProd;                  // Import/Export
        lvWattHourNet = (signed long)lvWattHourCons - lvWattHourProd;      // Import/Export
        
        MeterProduction.MeterUpdated = false;
        DisplayCursor(CUR_LINE0);
        // "Prod. 9999W  99.9kWh"
        Serial1.write("Prod:");
        DisplayMeterValues(lvWattProd, lvWattHourProd, false);
        
        // Consumption
        DisplayCursor(CUR_LINE1);
        // "Verb: 9999W  99.9kWh"
        Serial1.write("Verb:");
        DisplayMeterValues(lvWattCons, lvWattHourCons, false);
        
        // Netto
        DisplayCursor(CUR_LINE2);      
        // "Tot.:+9999W +99.9kWh"
        Serial1.write("Tot.:");
        DisplayMeterValues(lvWattNet, lvWattHourNet, true);
        
        // Date & Time
        DisplayCursor(CUR_LINE3);
        // "DD-MM-YYYY  HH:mm:ss"
        Serial1.write(DateTime(lvTime)); //"-");
    }
    else if ((s_DisplayState >= STATE_DISP_WEEK) && (s_DisplayState <= STATE_DISP_TOTAL))
    {   
      long lvEnergyNet, lvEnergyProd, lvEnergyCons;
  
      DisplayCursor(CUR_LINE3);    
      switch(s_DisplayState)
      {
        case STATE_DISP_WEEK:
          lvEnergyProd = MeterProduction.EnergyWeek;
          lvEnergyCons = MeterConsumption.EnergyWeek;
          Serial1.write("WEEK");     
          break;
        case STATE_DISP_MONTH:
          lvEnergyProd = MeterProduction.EnergyMonth;
          lvEnergyCons = MeterConsumption.EnergyMonth;
          Serial1.write("MAAND ");
          Serial1.print(month());
          break;
        case STATE_DISP_YEAR:
          lvEnergyProd = MeterProduction.EnergyYear;
          lvEnergyCons = MeterConsumption.EnergyYear;
          Serial1.write("JAAR ");     
          Serial1.print(year());        
          break;
        case STATE_DISP_TOTAL:
          lvEnergyProd = MeterProduction.EnergyTotal;
          lvEnergyCons = MeterConsumption.EnergyTotal;
          Serial1.write("TOTAAL");     
          break;        
      }
      lvEnergyNet = (lvEnergyCons-lvEnergyProd);
      
      // Production    
      // "Prod:   99999.999kWh"
      DisplayCursor(CUR_LINE0);
      Serial1.write("Prod:   ");
      DisplayMeterTotalValue(lvEnergyProd);
      
      // Consumption
      DisplayCursor(CUR_LINE1);
      Serial1.write("Verb:   ");
      DisplayMeterTotalValue(lvEnergyCons);    
      
      // Netto
      DisplayCursor(CUR_LINE2);
      Serial1.write("Tot.:   ");
      DisplayMeterTotalValue(lvEnergyNet, true);
    }
    else if (s_DisplayState == STATE_DISP_DEBUG)
    {
      extern time_t g_TimeOfLastSync;
      extern time_t g_PVOutputResponseTime;    
      extern int    g_upTimeHours;
      DisplayCursor(CUR_LINE0);
      Serial1.write(DateTime(g_TimeOfLastSync));
      DisplayCursor(CUR_LINE1);      
      Serial1.write(DateTime(g_PVOutputResponseTime));
      DisplayCursor(CUR_LINE3);      
      Serial1.write("Uptime: ");
      Serial1.print(g_upTimeHours);
      Serial1.write(" hr");      
    }
  }
  
  
  
  
  //DisplayCursor(CUR_LINE1);
  //Serial1.write("Verb. 9999W  99.9kWh");
  //DisplayCursor(CUR_LINE2);  
  //Serial1.write("Net. +9999W +99.9kWh");  
}


void DisplaySelectLine(byte inLine)
{
  DisplayCursor(s_CursorLineOffset[inLine]);
}



void DisplayCursor(byte inPos)
{
  if (inPos > 0x67)
  {
    inPos = 0x67;
  }
  Serial1.write(0xFE);
  Serial1.write(0x45);
  Serial1.write(inPos);
}

void DisplayBacklight(byte inValue)
{
  if (inValue < 1)
  {
    inValue = 1;
  }
  if (inValue > 8)
  {
    inValue = 8;
  }
  Serial1.write(0xFE);
  Serial1.write(0x53);
  Serial1.write(inValue);
}
void DisplayContrast(byte inValue)
{
  if (inValue < 1)
  {
    inValue = 1;
  }
  if (inValue > 50)
  {
    inValue = 50;
  }
  Serial1.write(0xFE);
  Serial1.write(0x52);
  Serial1.write(inValue);
}

void DisplayClear() 
{
  Serial1.write(0xFE);
  Serial1.write(0x51); 
  delay(100);
}

