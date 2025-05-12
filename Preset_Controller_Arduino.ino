// Digital Preset Controller for Arduino
// Copyright (c) 2024,2025 Joseph Reed, all rights reserved.

#include <Wire.h>
#include "EasyNextionLibrary.h"
#include <EEPROM.h>

EasyNex myNex(Serial); // Create an object of EasyNex class with the name < myNex >

// Arduino Pins (v1.0)
//int Position=A0;      // Rotor controller DIN pin 4
//int Speed=10;         // Rotor controller DIN pin 3
//int CCW=7;            // Rotor controller DIN pin 2
//int CW=6;             // Rotor controller DIN pin 1

// New Arduino Pins (v1.3)
int Position=A4;          // Rotor controller DIN pin 4
int Speed=3;              // Rotor controller DIN pin 3
int CCW=8;                // Rotor controller DIN pin 2
int CW=12;                // Rotor controller DIN pin 1

// Variables
String softwareVersion="1.3";  // Version information
String Build="132";

unsigned long pollTimer;
unsigned long currentMillis;
unsigned long pollInterval=250;
unsigned long genTimer;

int adc=0;
float vpd=.0048828;
int azimuth=0;        // Azimuth in degrees
int last_azimuth=0;   // Previous azimuth
int display_azimuth=0;// Azimuth corrected for rotor stops
// Voltage calibration variables
float v_Position=0.0; // Current rotor position
float v_Target=0.0;   // Target rotor position
float v_Min=0.0439453;      // Voltage at CCW stop (180)
float v_Max=4.482438;      // Voltage at CW stop (450)
float v_Degree=0.0098634;   // Voltage per degree of rotation
float tv_Min;         // Used for rotor calibration of v_Min
float tv_Max;         // Used for rotor calibration of v_Max
float tv_Degree;      // Used for rotor calibration of v_Degree

char Buffer[50];        // Used for converting floats to strings
int loopCount=0;        // Used to throttle bandwidth of GetPosition()
String genString="";    // General purpose string variable
int genInt1=0;          // General purpose integer variable
int genInt2=0;          // General purpose integer variable
float genFloat=0.0;     // General purpose floating point variable
int Zero=0;             // Useful for rotor speed control

// Mode menu and programming actions
int deMenu=0;           // Determines of Direct Entry is a menu only item.
int mcMenu=0;           // Include Manual Control in Mode menu
int ppMenu=0;           // Include Program Presets in Mode menu
int ppProgramAction=1;  // Toggles between remaining in program mode or switch back
int deProgramAction=1;  // Remain in DE or go to Home Page
// Rotor variables
int softTurn=75;        // Slow rotor speed 1-255
int fastTurn=225;       // Fast rotor speed 1-255
int softSpan=5;         // Size of slow start/stop zones
int manualTurn=128;     // Rotor turn rate when manually turning the rotor
int softStartEnabled=0; // Enable or Disable Slow Start
int mcRotorAction=1;    // Track with fast tune

// Preset memory banks
int memoryBank=0;       // Determines which preset bank to use
int defaultMemoryBank=0;

int checkConfig = 1;

// FUNCTIONS

//++++++++++++++++++++++++++++++++++++++++++++++++++++
// Main Operating Pages and Menu
//++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++
// Home Page
//++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
 Home Page Triggers
  Button  Hex Trigger Function
  b0      0   0       Recall preset 1
  b1      1   1       Recall preset 2
  b2      2   2       Recall preset 3
  b3      3   3       Recall preset 4
  b4      4A  74      Change Memory Bank (On Azimuth button)
  b5      4A  74      Change Memory Bank (Memory bank display)
  b8      8   8       Recall preset 5
  b9      9   9       Recall preset 6
  b10     0A  10      Recall preset 7
  b11     0B  11      Recall preset 8
  b16                 Page 3 (Menu)
  b17     56  86      Home page mode button
  h0      1A  26      Slider touch
  ho      12  18      Slider release   
 */
 
void HomePage() { // Home Page
    Serial.print("page page2");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    if (memoryBank == 0) {
      myNex.writeStr("b21.txt", "MEMORY BANK 1");
    } else {
      myNex.writeStr("b21.txt", "MEMORY BANK 2");
    }
    DisplayPresets();
}

//++++++++++++++++++++++++++++++++++++++++++++++
// Triggers called from the Home Page
//++++++++++++++++++++++++++++++++++++++++++++++
void trigger0() {     // Recall Preset 1
  if (memoryBank == 0) {
    EEPROM.get(102,v_Target);
  } else if (memoryBank == 1) {
    EEPROM.get(202,v_Target);
  }
  RecallPreset(v_Target);
}

void trigger1() {   // Preset 2 Recall
  if (memoryBank == 0) {
    EEPROM.get(112,v_Target);
  } else if (memoryBank == 1) {
    EEPROM.get(212,v_Target);
  }  
  RecallPreset(v_Target);
}

void trigger2() {   // Preset 3 Recall
  if (memoryBank == 0) {
    EEPROM.get(122,v_Target);
  } else if (memoryBank == 1) {
    EEPROM.get(222,v_Target);
  }
  RecallPreset(v_Target);
}

void trigger3() {   // Preset 4 Recall
  if (memoryBank == 0) {
    EEPROM.get(132,v_Target);
  } else if (memoryBank == 1) {
    EEPROM.get(232,v_Target);
  }
  RecallPreset(v_Target);
}

void trigger8() {   // Preset 5 Recall
  if (memoryBank == 0) {
    EEPROM.get(142,v_Target);
  } else if (memoryBank == 1) {
    EEPROM.get(242,v_Target);
  }
  RecallPreset(v_Target);
}

void trigger9() {   // Preset 6 Recall
  if (memoryBank == 0) {
    EEPROM.get(152,v_Target);
  } else if (memoryBank == 1) {
    EEPROM.get(252,v_Target);
  }  
  RecallPreset(v_Target);
}

void trigger10() {   // Preset 7 Recall
  if (memoryBank == 0) {
    EEPROM.get(162,v_Target);
  } else if (memoryBank == 1) {
    EEPROM.get(262,v_Target);
  }
  RecallPreset(v_Target);
}

void trigger11() {   // Preset 8 Recall
  if (memoryBank == 0) {
    EEPROM.get(172,v_Target);
  } else if (memoryBank == 1) {
    EEPROM.get(272,v_Target);
  }
  RecallPreset(v_Target);
}

void trigger86() {  // Home Page Mode Button
  if (ppMenu == 0) {
    ProgramPage();
  }
  else if (deMenu == 0) {
    DirectEntryPage();
  }
  else if (mcMenu == 0) {
    ManualControlPage();
  }
  else {
    HomePage();
  }
}

void trigger26() { // Slider touch
  myNex.writeNum("n1.pco",34815);
}

void trigger18() {  // Slider release
    // Read the target azimuth and convert it to a voltage
    int TargetAzimuth = myNex.readNumber("n1.val");
    if (TargetAzimuth <=180) {
      TargetAzimuth=TargetAzimuth+180;
    }
    else {
      if(TargetAzimuth >= 180) {
        TargetAzimuth=TargetAzimuth-180;
      }
    }
    v_Target=(TargetAzimuth*v_Degree)+v_Min;
    RecallPreset(v_Target); // Send it off as a One Time Preset
}

void trigger74() {  // Change memory banks
  if (memoryBank == 0) {
        memoryBank = 1;
        myNex.writeStr("b21.txt", "MEMORY BANK 2");
    } else {
        memoryBank = 0;
        myNex.writeStr("b21.txt", "MEMORY BANK 1");
    }
    DisplayPresets();
}


//++++++++++++++++++++++++++++++++++++++++++++++
// Program Page
//++++++++++++++++++++++++++++++++++++++++++++++
/* Program Page Triggers
 Home Page Triggers
  Button  Hex Dec     Function
  b0      4   4       Program preset 1
  b1      5   5       Program preset 2
  b2      6   6       Program preset 3
  b3      7   7       Program preset 4
  b8      0C  12      Program preset 5
  b9      0D  13      Program preset 6
  b10     0E  14      Program preset 7
  b11     0F  15      Program preset 8
  b5      4D  77      Change memory bank
  b4      4D  77      Change memory bank
  b16                 Page 3 (Menu)
  b17     56  87      Program page mode button   
 */
void ProgramPage() {
    Serial.print("page page0");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    if (memoryBank == 0) {
      myNex.writeStr("b21.txt", "MEMORY BANK 1");
    } else if (memoryBank == 1) {
      myNex.writeStr("b21.txt", "MEMORY BANK 2");
    }   
    DisplayPresets();
}

//++++++++++++++++++++++++++++++++++++++++++++++
// Triggers called from the Program Page
//++++++++++++++++++++++++++++++++++++++++++++++

void trigger4() {    // Preset 1 Store
  int Address=100;
  if (memoryBank == 1) {
    Address = Address + 100;
  }
  StorePreset(Address);
}

void trigger5(){    // Preset 2 Store
  int Address=110;
  if (memoryBank == 1) {
    Address = Address + 100;
  }
  StorePreset(Address);
}

void trigger6() {    // Preset 3 Store
  int Address=120;
  if (memoryBank == 1) {
    Address = Address + 100;
  }
  StorePreset(Address);  
}

void trigger7() {    // Preset 4 Store
  int Address=130;
  if (memoryBank == 1) {
    Address = Address + 100;
  }
  StorePreset(Address);
}

void trigger12() {    // Preset 5 Store
  int Address=140;
  if (memoryBank == 1) {
    Address = Address + 100;
  }
  StorePreset(Address);
}

void trigger13() {    // Preset 6 Store
  int Address=150;
  if (memoryBank == 1) {
    Address = Address + 100;
  }
  StorePreset(Address);  
}

void trigger14() {    // Preset 7 Store
  int Address=160;
  if (memoryBank == 1) {
    Address = Address + 100;
  }
  StorePreset(Address);
}

void trigger15() {    // Preset 8 Store
  int Address=170;
  if (memoryBank == 1) {
    Address = Address + 100;
  }
  StorePreset(Address);
  }  

void trigger87() {  // Program Page Mode Button
  if (deMenu == 0) {
    DirectEntryPage();
  }
  else if (mcMenu == 0) {
    ManualControlPage();
  }
  else {
    HomePage();
  }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// StorePreset()
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void StorePreset(int Address) {
  EEPROM.put(Address,display_azimuth);
  EEPROM.put(Address+2, v_Position);
  if (ppProgramAction == 0) {
    HomePage();
  }
  else {
    DisplayPresets();
  }
}
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Manual Control Page
//++++++++++++++++++++++++++++++++++++++++++++++++++
/* Manual Control Page Triggers
 Home Page Triggers
  Button  Hex Dec     Function
  h0      31  49      Slider bar release
  CCW     2F  47      Button touch
  CCW     2E  46      Button touch
  CW/CCW  2D  45      Button release
  b16                 Page 3 (Menu)
  b17     52  82      Manual Control Mode Button  (see Menu)
*/ 
void ManualControlPage() {
    if (mcRotorAction == 1) {
      Serial.print("page page10");
    }
    else {
      Serial.print("page page16");
    }
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    if (mcRotorAction == 1) {
    myNex.writeNum("h0.val", int(manualTurn/2.55));
    myNex.writeNum("n0.val", int(manualTurn/2.55));
    }
    else {
      manualTurn=fastTurn;
    }
}

void trigger48() { // Call Manual Turn Screen
  ManualControlPage;
}

//++++++++++++++++++++++++++++++++++++++++++++++
// Triggers called from the Manual Control Page
//++++++++++++++++++++++++++++++++++++++++++++++
void trigger49() { // Slider release
  manualTurn=myNex.readNumber("n0.val");
  manualTurn=int(manualTurn*2.55);
  EEPROM.put(56, manualTurn);
}

void trigger46() { // CCW Button press
  loopCount=0;
  SendRotorSpeed(manualTurn);
  digitalWrite(CW,HIGH);
  while(1) {
    myNex.NextionListen();
    loopCount++;
    if(loopCount == 10000) {
      GetPosition();
      loopCount=0;      
    }
  }
}

void trigger47() { // CCW Button press
  loopCount=0;
  SendRotorSpeed(manualTurn);
  digitalWrite(CCW,HIGH);
  while(1) {
    myNex.NextionListen();
    loopCount++;
    if(loopCount == 10000) {
      GetPosition();
      loopCount=0;      
    }
  }
}

void trigger45() {  // CW/CCW Button Release
  SendRotorSpeed(Zero);
  digitalWrite(CW,LOW);
  digitalWrite(CCW,LOW);
}


//++++++++++++++++++++++++++++++++++++++++++++++
// Direct Entry Page
//++++++++++++++++++++++++++++++++++++++++++++++
/* Manual Control Page Triggers
 Home Page Triggers
  Button  Hex Dec     Function
  b0      24  36      Enter Key
  b11     26  38      Erase Key
  b16                 Page 3 (Menu)
  b17     58  88      Direct Entry Page Mode Button
*/
void DirectEntryPage() {
  Serial.print("page page8");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

//++++++++++++++++++++++++++++++++++++++++++++++
// Triggers called from the Direct Entry Page
//++++++++++++++++++++++++++++++++++++++++++++++
void trigger36() { // Direct Entry Enter Key
  if (genString != "") {
    int azimuth=genString.toInt();
    if (azimuth >= 0 && azimuth <=360) { // Test for a valid value
      if (azimuth <= 180) { // Correct for the rotor stops
        azimuth=azimuth+180;
      }
      else {
        azimuth=azimuth-180;
      }
      float target=azimuth*v_Degree+v_Min;
      RecallPreset(target);
       }
      genString="";
      myNex.writeStr("t1.txt",genString);
      if(deProgramAction == 0) {  // Return to Home Page
        HomePage();
    }
  }
}

void trigger38() { // Erase
  genString="";
  myNex.writeStr("t1.txt",genString);
}

void trigger88() {  // Direct Entry Page Mode Button
  if (mcMenu == 0) {
    ManualControlPage();
  }
  else {
    HomePage();
  }
}
//++++++++++++++++++++++++++++++++++++++++
// Direct Entry Keypad triggers
//++++++++++++++++++++++++++++++++++++++++

void trigger27() { //Number 1
  genString=genString+"1";
  myNex.writeStr("t1.txt",genString);
}

void trigger28() { //Number 2
  genString=genString+"2";
  myNex.writeStr("t1.txt",genString);
}

void trigger29() { //Number 3
  genString=genString+"3";
  myNex.writeStr("t1.txt",genString);
}

void trigger30() { //Number 4
  genString=genString+"4";
  myNex.writeStr("t1.txt",genString);
}

void trigger31() { //Number 5
  genString=genString+"5";
  myNex.writeStr("t1.txt",genString);
}

void trigger32() { //Number 6
  genString=genString+"6";
  myNex.writeStr("t1.txt",genString);
}

void trigger33() { // Number 7
  genString=genString+"7";
  myNex.writeStr("t1.txt",genString);
}

void trigger34() { //Number 8
  genString=genString+"8";
  myNex.writeStr("t1.txt",genString);
}

void trigger35() { //Number 9
  genString=genString+"9";
  myNex.writeStr("t1.txt",genString);
}

void trigger37() { //Number 0
  genString=genString+"0";
  myNex.writeStr("t1.txt",genString);
}


// End of Direct Entry Keypad

//+++++++++++++++++++++++++++++++++++++++++++++++
// Menu
//+++++++++++++++++++++++++++++++++++++++++++++++
/*
 Menu Triggers (Menu is static page 3)
  Home Page Triggers
  Button  Hex Dec     Function
  b0      13  19      Calls Voltage Calibration
  b1      3F  63      Calls Rotor Settings
  b2      54  84      Calls Direct Entry Page
  b3                  Calls static page 11
  b4      52  82      Exit button
  b5      19  25      Calls static page 13
  b6      55  85      Calls Manual Control
  b7      52  82      Calls Home Page
  b8      53  83      Calls Program Page
 */
//++++++++++++++++++++++++++++++++++++++++++++++
// Triggers called from the Menu
//++++++++++++++++++++++++++++++++++++++++++++++
void trigger82() { // Home Page
  HomePage();
}

void trigger83() { // Program Page
  ProgramPage();
}

void trigger84() { // Direct Entry Page
  DirectEntryPage();
}

void trigger85() { // Manual Control Page
  ManualControlPage();
}

void trigger19() {   // Voltage calibration Step 1
  tv_Min=0;
  tv_Max=0;
  tv_Degree=0;
  Serial.print("page page1");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  PrintCurrentEEPROM();
}


//+++++++++++++++++++++++++++++++++++++++++++++++
// Functions Called From Menu
//+++++++++++++++++++++++++++++++++++++++++++++++
// System Preferences
//+++++++++++++++++++++++++++++++++++++++++++++++
/*
  MenuModePreferences Triggers
  Button  Hex Dec     Function
  b0      2B  43      Enable/Disable Direct Entry in Mode menu
  b1      11  17      Enable/Disable Program Page in Mode menu
  b2                  Calls static page 13 (system preferenes)
  b3      28  40      Enable/Disable Manual Control in Mode menu
 */
void ModeMenuPreferences() {
  Serial.print("page page9");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  printModeMenuPreferences();
}

void trigger41() {    // Systems Preference menu
  ModeMenuPreferences();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Triggers Called from Mode Menu Preferences (page 13)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++
void trigger17() {  // Set ppMenu
  if (ppMenu == 0) {  // Disable
    ppMenu = 1;
  }
  else {                      //Enable
    ppMenu = 0;
  }
  EEPROM.put(50,ppMenu);
  printModeMenuPreferences();
}

void trigger40() {  // Enable/disable manualTuneMenu in System Preferences
  if (mcMenu == 0) {  // Disable
    mcMenu = 1;
  }
  else {                      //Enable
    mcMenu = 0;
  }
  EEPROM.put(48,mcMenu);
  printModeMenuPreferences();
}

void trigger43() {    // Direct Entry
  if (deMenu == 0) { // Switch 
    deMenu = 1;
  }
  else {
    deMenu = 0;
  }
  EEPROM.put(46,deMenu);
  printModeMenuPreferences();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Programming Preferences 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
  ProgramActionPreferences Triggers
  Button  Hex Dec     Function
  b0      50  80      Return to Home after Direct Entry
  b1      4B  75      Set default memory bank of start up
  b2                  Calls static page 13 (back button)
  b3      51  81      Return to Home after programming memory
 */
void ProgramActionPreferences() {
  Serial.print("page page14");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  printProgramActionPreferences();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Triggers called by ProgramAction Preferenes
//++++++++++++++++++++++++++++++++++++++++++++++++++++++
void trigger75() { // Set default preset memory bank
  if (defaultMemoryBank == 0) {
    defaultMemoryBank = 1;
  } else if (defaultMemoryBank == 1) {
    defaultMemoryBank = 0;
  }
  EEPROM.put(62,defaultMemoryBank);
  printProgramActionPreferences();
}

void trigger80() {
  if (ppProgramAction == 0) { // Switch 
    ppProgramAction = 1;
  }
  else {
    ppProgramAction = 0;
  }
  EEPROM.put(52,ppProgramAction);
  printProgramActionPreferences();
}

void trigger81() { // Toggle Home after DE entry
    if (deProgramAction == 0) { // Switch 
    deProgramAction = 1;
  }
  else {
    deProgramAction = 0;
  }
  EEPROM.put(54,deProgramAction);
  printProgramActionPreferences();  
}

void trigger53() { // Programming preferences
  ProgramActionPreferences();
}

void trigger54() {  // Rotor preferences
  RotorPreferences();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++
// Rotor Preferences (Called from System Preferences)
//+++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
  RotorPreferences Triggers
  Button  Hex Dec     Function
  b0      4E  78      Enable Slow Start
  b2                  Call to static page 13
  b3      4F  79      Manual speed matches preset speed
 */
void RotorPreferences() {
  Serial.print("page page15");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  printRotorPreferences();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Triggers called by RotorPreferences
//++++++++++++++++++++++++++++++++++++++++++++++++++++++
void trigger78() { // Enable/disable slow start function
  if (softStartEnabled == 0) {  // Disable
    softStartEnabled = 1;
  }
  else {                      //Enable
    softStartEnabled = 0;
  }
  EEPROM.put(58,softStartEnabled);
  printRotorPreferences();
}

void trigger79() {  // Manual control speed = preset speed
  if(mcRotorAction == 0) {
    mcRotorAction=1;
  }
  else {
    mcRotorAction = 0;
  }
  EEPROM.put(60,mcRotorAction);
  printRotorPreferences();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Rotor Settings (Called from Menu)
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
  RotorSettings Triggers
  Button  Hex Dec     Function
  b0                  Call to static page 3
  b1      16  22      Apply rotor speed
*/
void trigger63() {   // Call rotor calibration from Menu
    Serial.print("page page4");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff); 
    int slow= int(softTurn/2.55);
    int fast= int(fastTurn/2.55);
    int manual= int(manualTurn/2.55);
    myNex.writeNum("h0.val", slow);
    myNex.writeNum("h1.val", fast);
    myNex.writeNum("h2.val",softSpan);
    myNex.writeNum("h3.val",manual);
    printRotorSettings();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Triggers called by Rotor Settings
//++++++++++++++++++++++++++++++++++++++++++++++++++++++
void trigger22() { // Set rotor speed
  softTurn = myNex.readNumber("n0.val");
  softTurn = int(softTurn*2.55);
  fastTurn = myNex.readNumber("n1.val");
  fastTurn = int(fastTurn*2.55);
  softSpan = myNex.readNumber("n2.val");
  manualTurn = myNex.readNumber("n3.val");
  manualTurn = int(manualTurn*2.55);
  EEPROM.put(40,softTurn);
  EEPROM.put(42,fastTurn);
  EEPROM.put(44,softSpan);
  EEPROM.put(56,manualTurn);
  Serial.print("page page3");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff); 
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Voltage Calibration Step 2
//++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
  RotorSettings Triggers
  Button  Hex Dec     Function
  b0                  Calls static page 3 (cancel)
  b1      18  24      Calls calibration summary
 */
void trigger20() {  // Voltage calibration Step 2
  Serial.print("page page6");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  //int adc=MeasureVoltage();
  adc=MeasureVoltage();
  float interim=float(adc);
  tv_Min=adc*vpd;
  dtostrf(tv_Min,10,7,Buffer);
  String tstring= String(Buffer);
  myNex.writeStr("t5.txt",tstring);
  PrintCurrentEEPROM();
  }
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Triggers for Voltage calibration Step 2
//++++++++++++++++++++++++++++++++++++++++++++++++++
void trigger24() { // Summary calibration screen
  Serial.print("page page7");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  //int adc=MeasureVoltage();
  adc=MeasureVoltage();
  float interim=float(adc);
  tv_Max=interim*vpd;
  tv_Degree=(tv_Max-tv_Min)/450;
  dtostrf(tv_Min,10,7,Buffer);
  String tstring= String(Buffer);
  myNex.writeStr("t5.txt",tstring);
  dtostrf(tv_Max,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t8.txt",tstring);
  dtostrf(tv_Degree,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t11.txt",tstring);
  PrintCurrentEEPROM();
  }

//++++++++++++++++++++++++++++++++++++++++++++++++
// Voltage Calibration Step 3 (Summary)
//++++++++++++++++++++++++++++++++++++++++++++++++
/*
  RotorSettings Triggers
  Button  Hex Dec     Function
  b0                  Call to static page 3
  b1      15  21      Apply voltage calibration
*/

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Triggers for Voltage calibration Step 3
//++++++++++++++++++++++++++++++++++++++++++++++++++
void trigger21() {  // Submit voltage calibration to EEPROM
  v_Min=tv_Min;
  v_Max=tv_Max;
  v_Degree=tv_Degree;
  EEPROM.put(10,tv_Min);
  EEPROM.put(20,tv_Max);
  EEPROM.put(30,tv_Degree);
  Serial.print("page page3");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff); 
}

//++++++++++++++++++++++++++++++++++++++++++++++++++
// MeasureVoltage
//++++++++++++++++++++++++++++++++++++++++++++++++++
int MeasureVoltage() {  // calculate values for v_Min and v_Max
  int arraySize=50;
  int sample[arraySize];
  int Array[arraySize];
  
  int mfv;
  for (int i=0; i< arraySize; i++) {
    for (int x=0; x<arraySize; x++){
      sample[x] = analogRead(Position);
    }
    Array[i]=ProcessArray(sample, arraySize);
  } 
  mfv=ProcessArray(Array, arraySize);
  return mfv;
}  // End of MeasureVoltage();


//+++++++++++++++++++++++++++++++++++++++++++++++++++++
// Functions that populate labels and buttons with data
//++++++++++++++++++++++++++++++++++++++++++++++++++++++
void DisplayPresets() { // Read and display preset azimuths
  int temp=0;
  int Address = 0;
  String tString1 = "";
  
  if (memoryBank == 0) {
    Address = 90;
  } else {
    Address = 190;
  }

  for (int i=0; i<8; i++) {
    Address = Address + 10;
    tString1 = "b";
    tString1 = tString1 + i;
    tString1 = tString1 + ".txt";
    EEPROM.get(Address, temp);
    myNex.writeStr(tString1, String(temp));
    
  }
} // End of PresetDisplay

void printModeMenuPreferences() {
  if (deMenu == 0) { 
    myNex.writeNum("b0.bco", 2016);
    myNex.writeStr("b0.txt","Enabled");
  }
  else {
    myNex.writeNum("b0.bco", 65504);
    myNex.writeStr("b0.txt","Disabled");
  }
  if (ppMenu == 0) { 
    myNex.writeNum("b1.bco", 2016);
    myNex.writeStr("b1.txt","Enabled");
  }
  else {
    myNex.writeNum("b1.bco", 65504);
    myNex.writeStr("b1.txt","Disabled");
  }
  if (mcMenu == 0) { 
    myNex.writeNum("b3.bco", 2016);
    myNex.writeStr("b3.txt","Enabled");
  }
  else {
    myNex.writeNum("b3.bco", 65504);
    myNex.writeStr("b3.txt","Disabled");
  }
}

void printProgramActionPreferences() {
   if (ppProgramAction == 0) { 
    myNex.writeNum("b4.bco", 2016);
    myNex.writeStr("b4.txt","Enabled");
  }
  else {
    myNex.writeNum("b4.bco", 65504);
    myNex.writeStr("b4.txt","Disabled");
  }
  if (deProgramAction == 0) {
    myNex.writeNum("b5.bco", 2016);
    myNex.writeStr("b5.txt","Enabled");
  }
  else {
    myNex.writeNum("b5.bco", 65504);
    myNex.writeStr("b5.txt","Disabled");
  }
  if (defaultMemoryBank == 0) {
    myNex.writeNum("b6.bco", 2016);
    myNex.writeStr("b6.txt", "Bank 1");
  } else {
    myNex.writeNum("b6.bco", 2016);
    myNex.writeStr("b6.txt", "Bank 2");
  }
}

void printRotorPreferences() {
   if (softStartEnabled == 0) { 
    myNex.writeNum("b7.bco", 2016);
    myNex.writeStr("b7.txt","Enabled");
  }
  else {
    myNex.writeNum("b7.bco", 65504);
    myNex.writeStr("b7.txt","Disabled");
  }
  if (mcRotorAction == 1) {
    myNex.writeNum("b8.bco", 2016);
    myNex.writeStr("b8.txt","Enabled");
  }
  else {
    myNex.writeNum("b8.bco", 65504);
    myNex.writeStr("b8.txt","Disabled");
  }
}

void printRotorSettings() {
  myNex.writeNum("n0.val", int(softTurn/2.55));
  myNex.writeNum("n1.val", int(fastTurn/2.55));
  myNex.writeNum("n2.val",softSpan);
  myNex.writeNum("n3.val",int(manualTurn/2.55));
}

void printMemoryBank() {
  String tString1 = "";
  String tString2 = "";
  float tempf = 0.0;
  int tempi = 0;
  int Address = 0;

  if (genInt1 == 1) {
    genInt1 = 0;
    myNex.writeStr("t30.txt", "MEMORY BANK 1");
    myNex.writeStr("b0.txt", "Bank 2");
  } else {
    genInt1 = 1;
    myNex.writeStr("t30.txt", "MEMORY BANK 2");
    myNex.writeStr("b0.txt", "Bank 1");
  }

  if (genInt1 == 0) {
    Address = 90;
  } else {
    Address = 190;
  }

  for (int i=0; i<8; i++) {
    tString1 = "n";
    tString1 = tString1 + i;
    tString1 = tString1 + ".val";
    tString2 = "t";
    tString2 = tString2 + i;
    tString2 = tString2 + ".txt";
    Address = Address + 10;
    EEPROM.get(Address, tempi);
    EEPROM.get(Address+2, tempf);
    dtostrf(tempf,10,7,Buffer);
    genString = String(Buffer);
    myNex.writeNum(tString1, tempi);
    myNex.writeStr(tString2, genString);
  }
}

void PrintCurrentEEPROM() {   // Used in voltage calibration
  dtostrf(v_Min,10,7,Buffer);
  String tstring= String(Buffer);
  myNex.writeStr("t6.txt",tstring);
  dtostrf(v_Max,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t9.txt",tstring);
  dtostrf(v_Degree,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t12.txt",tstring);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// System Functions
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int ProcessArray(int A[], int n) { // Process the array and return the most common value
  
  for (int i = 0; i < n; i++)    //Sort the array
  {
    int temp;
    for (int j = i + 1; j < n; j++)
    {
      if (A[i] > A[j])
      {
        temp = A[i];
        A[i] = A[j];
        A[j] = temp;
      }
    }
  }
  //finnd the most occuring element
  int max_count = 1, res = A[0], count = 1;
  for (int i = 1; i < n; i++) {
    if (A[i] == A[i - 1])
      count++;
    else {
      if (count > max_count) {
        max_count = count;
        res = A[i - 1];
      }
      count = 1;
    }
  }
  // If last element is most frequent
  if (count > max_count)
  {
    max_count = count;
    res = A[n - 1];
  }
  return res; //return the most repeatinng  element
}  //  

void SendRotorSpeed(int turnSpeed) { // Uncomment for Arduino slave
  //Wire.beginTransmission(0x55);
  //Wire.write((turnSpeed>>2));
  //Wire.endTransmission();
  if (turnSpeed == 0){
    digitalWrite(Speed,LOW);
  }
  else {
    analogWrite(Speed,turnSpeed);   // Comment out if using Arduino Slave
  }
}

void GetPosition() {
  int arraySize=250;
  int sample[arraySize];
  for (int i=0; i< arraySize; i++){
    sample[i]=analogRead(Position);
  }
  adc=ProcessArray(sample, arraySize);
  float interim=float(adc);
  v_Position=adc*vpd;
  azimuth = int((v_Position-v_Min)/v_Degree);
 // if (azimuth != last_azimuth){
      last_azimuth = azimuth;   
     // Update nextion display
     display_azimuth=azimuth;
     if (azimuth <= 179){
      display_azimuth = display_azimuth + 180;
     }
     if (azimuth >= 180){
      display_azimuth = display_azimuth - 180;
     }
     myNex.writeNum("n9.val", display_azimuth);
 // }
}  // End of GetPosition

// RecallPreset 
void RecallPreset(float v_Target){
  if (softStartEnabled == 1) {
    if (v_Target > v_Position) { //clockwise
      SendRotorSpeed(fastTurn);
      digitalWrite(CW,HIGH);
      while(v_Position < v_Target){
      GetPosition();
      } 
      digitalWrite(CW,LOW);
      SendRotorSpeed(Zero);
    }
    else {
      SendRotorSpeed(fastTurn);
      digitalWrite(CCW,HIGH);
      while(v_Position > v_Target){
      GetPosition();
      }
      digitalWrite(CCW,LOW);
      SendRotorSpeed(Zero);
    }
  } 
  else {
  float slowStart=0.0;
  float slowStop=0.0;
  if (v_Target > v_Position){  // Clockwise
    slowStart=v_Position+(softSpan*v_Degree);
    slowStop=v_Target-(softSpan*v_Degree);
    if (v_Target < slowStart) {
      SendRotorSpeed(softTurn);
      digitalWrite(CW,HIGH);
      while(v_Position < v_Target){
      GetPosition();
      }
    } else {
    SendRotorSpeed(softTurn);
    digitalWrite(CW,HIGH);
    while(v_Position < slowStart){
      GetPosition();
    }
    SendRotorSpeed(fastTurn);
    while(v_Position < slowStop) {
      GetPosition();  
    }
    SendRotorSpeed(softTurn);
      while(v_Position < v_Target) {
        GetPosition();
      }
    }
    digitalWrite(CW,LOW);
    int tempi=0;
    SendRotorSpeed(tempi);
    }
    else {  // Counter clockwise
     slowStart = v_Position-(softSpan*v_Degree);
     slowStop = v_Target+(softSpan*v_Degree); 
     if (v_Target >= slowStart) {
      SendRotorSpeed(softTurn);
      digitalWrite(CW,HIGH);
      while(v_Position > v_Target){
      GetPosition();
      }
    } else {
     SendRotorSpeed(softTurn);
     digitalWrite(CCW,HIGH);
      while(v_Position > slowStart){
        GetPosition();
      }
      SendRotorSpeed(fastTurn);
      while(v_Position > slowStop) {
        GetPosition();
      }
      SendRotorSpeed(softTurn);
      while(v_Position > v_Target) {
        GetPosition();
      }
    }
    digitalWrite(CCW,LOW);
    int tempi=0;
    SendRotorSpeed(tempi);
     }
  }
    myNex.writeNum("n1.pco",0);  // If using the slider bar
    myNex.writeNum("h0.val",0);  // turn it off and return
    myNex.writeNum("n1.val",0);  // to the home position.
   
}  // End of RecallPreset()


//++++++++++++++++++++++++++++++++++++++++++++++
// System Information
//++++++++++++++++++++++++++++++++++++++++++++++

void trigger90() {  // About page
  Serial.print("page page17");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  myNex.writeStr("t4.txt",softwareVersion);
  myNex.writeStr("t6.txt",Build);
}

void trigger65() {    // Memory Bank 1
  genInt1=1;
  Serial.print("page page5");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  printMemoryBank();
}

void trigger66() {    // Memory Bank 2
  printMemoryBank();
}

void trigger50() { // System info - System Preferences
  Serial.print("page page12");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  printModeMenuPreferences();
  printProgramActionPreferences();
  printRotorPreferences();
}

//****************************************************
// Voltage Calibration Data
//++++++++++++++++++++++++++++++++++++++++++++++++++++
void trigger91()  {
  Serial.print("page page20");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  PrintCurrentEEPROM();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++
// Rotor Settings (System Information)
//++++++++++++++++++++++++++++++++++++++++++++++++++++
void trigger92() {
  Serial.print("page page19");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  printRotorSettings();  
}

//+++++++++++++++++++++++++++++++++++++++++++++
// Main Code Loop
//+++++++++++++++++++++++++++++++++++++++++++++

void setup() {
  Wire.begin();
  Serial.begin(9600);
  myNex.begin(9600);
  delay(500);
  
  pinMode(Speed,OUTPUT);
  pinMode(CCW,OUTPUT);
  pinMode(CW,OUTPUT);
  digitalWrite(CCW,LOW);
  digitalWrite(CW,LOW);
  digitalWrite(Speed,LOW);
  //SendRotorSpeed(Zero);

  // Handle uninitialized EEPROM
  EEPROM.get(8,checkConfig);
  
  if (checkConfig != 0) {
    //EEPROM.put(10,v_Min);
    //EEPROM.put(20,v_Max);
    //EEPROM.put(30,v_Degree);
    EEPROM.put(40,softTurn);
    EEPROM.put(42,fastTurn);
    EEPROM.put(44,softSpan);
    EEPROM.put(46,deMenu);
    EEPROM.put(48,mcMenu);
    EEPROM.put(50,ppMenu);
    EEPROM.put(52,ppProgramAction);
    EEPROM.put(54,deProgramAction);
    EEPROM.put(56,manualTurn);
    EEPROM.put(58,softStartEnabled);
    EEPROM.put(60,mcRotorAction);
    EEPROM.put(62,defaultMemoryBank);
    EEPROM.put(8,Zero);
  }
 
  
  // Read EEPROM to populate system values
  EEPROM.get(10,v_Min);
  EEPROM.get(20,v_Max);
  EEPROM.get(30,v_Degree);
  EEPROM.get(40,softTurn);
  EEPROM.get(42,fastTurn);
  EEPROM.get(44,softSpan);
  EEPROM.get(46,deMenu);
  EEPROM.get(48,mcMenu);
  EEPROM.get(50,ppMenu);
  EEPROM.get(52,ppProgramAction);
  EEPROM.get(54,deProgramAction);
  EEPROM.get(56,manualTurn);
  EEPROM.get(58,softStartEnabled);
  EEPROM.get(60,mcRotorAction);
  EEPROM.get(62,defaultMemoryBank);
  
  memoryBank = defaultMemoryBank;

  // Set initial display
  HomePage(); 
  pollTimer=millis();
}  // end of setup()

void loop() {
  myNex.NextionListen();
  currentMillis=millis();
  if (currentMillis - pollTimer >= pollInterval) {
    GetPosition();
    pollTimer=millis();
  }
}
