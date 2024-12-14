// Digital Preset Controller
// Copyright (c) 2024 Joseph Reed, all rights reserved.

#include <Wire.h>
#include "EasyNextionLibrary.h"
#include <EEPROM.h>

#define EEPROM_SIZE 512 // Used with ESP32 EEPROM emulator

EasyNex myNex(Serial); // Create an object of EasyNex class with the name < myNex >

// Arduino Pins
//int Position=A0;      // Rotor controller DIN pin 4
//int Speed=10;         // Rotor controller DIN pin 3
//int CCW=7;            // Rotor controller DIN pin 2
//int CW=6;             // Rotor controller DIN pin 1

// ESP32 Pins         // Speed (DIN pin 3) is pin D10 on the Arduino slave
int Position=34;      // Rotor controller DIN pin 4 
int CW=32;            // Rotor controller DIN pin 1
int CCW=33;           // Rotor controller DIN pin 2

// Variables
int adc=0;
//float vpd=.00488288;  // Volts per division 5/1024; 
float vpd=.0032226;   // Volts per division 3.3/1024; 
int azimuth=0;        // Azimuth in degrees
int last_azimuth=0;   // Previous azimuth
int display_azimuth=0;// Azimuth corrected for rotor stops
float v_Position=0.0; // Current rotor position
float v_Target=0.0;   // Target rotor position
float v_Min=0.0;      // Voltage at CCW stop (180)
float v_Max=0.0;      // Voltage at CW stop (450)
float v_Degree=0.0;   // Voltage per degree of rotation
float tv_Min;         // Used for rotor calibration of v_Min
float tv_Max;         // Used for rotor calibration of v_Max
float tv_Degree;      // Used for rotor calibration of v_Degree
int slowTurn=0;       // Slow rotor speed 1-255
int fastTurn=0;       // Fast rotor speed 1-255
int slowSpan=5;       // Size of slow start/stop zones
char Buffer[50];      // Used for converting floats to strings
int loopCount=0;      // Used to throttle bandwidth of GetPosition()
String directEntry=""; // Used to display direct entry
int deMenu=0;         // Determines of Direct Entry is a menu only item.
int setProgramAction=0; // Toggles between remaining in program mode or switch back
int manualTurn=0;       // Rotor turn rate when manually turning the rotor
int manualTurnMenu=0;   // Determines if manual tuning is in the mode menu


void setup() {
  // Uncomment for ESP32
  EEPROM.begin(EEPROM_SIZE);
  Wire.begin();
  Serial.begin(9600);
  myNex.begin(9600);
  delay(500);
  // Comment out Speed for Slave 
//  pinMode(Speed,OUTPUT);
  pinMode(CCW,OUTPUT);
  pinMode(CW,OUTPUT);


  // Read EEPROM to populate system values
  EEPROM.get(10,v_Min);
  EEPROM.get(20,v_Max);
  EEPROM.get(30,v_Degree);
  EEPROM.get(40,slowTurn);
  EEPROM.get(42,fastTurn);
  EEPROM.get(44,slowSpan);
  EEPROM.get(46,deMenu);
  EEPROM.get(50,setProgramAction);
  EEPROM.get(52,manualTurn);
  EEPROM.get(54,manualTurnMenu);

  // Set initial display
  HomeScreen();
}  // end of setup()

void loop() {
  myNex.NextionListen();
  loopCount++;
  if (loopCount == 20000) {
    loopCount=0;
    GetPosition();
  }
}

void SendRotorSpeed(int turnSpeed) { // Uncomment for Arduino slave
  Wire.beginTransmission(0x55);
  Wire.write((turnSpeed>>2));
  Wire.endTransmission();
  //analogWrite(Speed,turnSpeed);   // Comment out if using Arduino Slave
}

void GetPosition(){
  int arraySize=250;
  int sample[arraySize];
  for (int i=0; i< arraySize; i++){
    sample[i]=analogRead(Position);
  }
  adc=ProcessArray(sample, arraySize);
  float interim=float(adc);
  v_Position=adc*vpd;
  azimuth = int((v_Position-v_Min)/v_Degree);
  if (azimuth != last_azimuth){
      last_azimuth = azimuth;   
     // Update nextion display
     display_azimuth=azimuth;
     if (azimuth <= 180){
      display_azimuth = display_azimuth + 180;
     }
     if (azimuth >= 180){
      display_azimuth = display_azimuth - 180;
     }
     myNex.writeNum("n9.val", display_azimuth);
  }
}  // End of GetPosition

// RecallPreset - Well it does what it does.
void RecallPreset(float v_Target){
  float slowStart=0.0;
  float slowStop=0.0;
  if (v_Target > v_Position){  // Clockwise
    slowStart=v_Position+(slowSpan*v_Degree);
    slowStop=v_Target-(slowSpan*v_Degree);
    if (v_Target < slowStart) {
      SendRotorSpeed(slowTurn);
      digitalWrite(CW,HIGH);
      while(v_Position < v_Target){
      GetPosition();
      }
    } else {
    SendRotorSpeed(slowTurn);
    digitalWrite(CW,HIGH);
    while(v_Position < slowStart){
      GetPosition();
    }
    SendRotorSpeed(fastTurn);
    while(v_Position < slowStop) {
      GetPosition();  
    }
    SendRotorSpeed(slowTurn);
      while(v_Position < v_Target) {
        GetPosition();
      }
    }
    digitalWrite(CW,LOW);
    int tempi=0;
    SendRotorSpeed(tempi);
    }
    else {  // Counter clockwise
     slowStart = v_Position-(slowSpan*v_Degree);
     slowStop = v_Target+(slowSpan*v_Degree); 
     if (v_Target >= slowStart) {
      SendRotorSpeed(slowTurn);
      digitalWrite(CW,HIGH);
      while(v_Position > v_Target){
      GetPosition();
      }
    } else {
     SendRotorSpeed(slowTurn);
     digitalWrite(CCW,HIGH);
      while(v_Position > slowStart){
        GetPosition();
      }
      SendRotorSpeed(fastTurn);
      while(v_Position > slowStop) {
        GetPosition();
      }
      SendRotorSpeed(slowTurn);
      while(v_Position > v_Target) {
        GetPosition();
      }
    }
    digitalWrite(CCW,LOW);
    int tempi=0;
    SendRotorSpeed(tempi);
     }
    myNex.writeNum("n1.pco",0);  // If using the slider bar
    myNex.writeNum("h0.val",0);  // turn it off and return
    myNex.writeNum("n1.val",0);  // to the home position.
   
}  // End of RecallPreset()

void HomeScreen() { // Normal operating screen
    Serial.print("page page2");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    PresetDisplay();
}

void ProgramScreen() { // Program mode
    Serial.print("page page0");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff); 
    PresetDisplay();
}

void SystemPreferences() { // System Preferences screen
  Serial.print("page page9");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if (setProgramAction == 0) {
    myNex.writeNum("b1.bco", 2016);
    myNex.writeStr("b1.txt","Enabled");
  }
  else {
    myNex.writeNum("b1.bco", 65504);
    myNex.writeStr("b1.txt","Disabled");
  }
  if (deMenu == 0) {
    myNex.writeNum("b0.bco", 2016);
    myNex.writeStr("b0.txt","Enabled");
  }
  else {
    myNex.writeNum("b0.bco", 65504);
    myNex.writeStr("b0.txt","Disabled");
  }
  if (manualTurnMenu == 0) {
    myNex.writeNum("b3.bco", 2016);
    myNex.writeStr("b3.txt","Enabled");
  }
  else {
    myNex.writeNum("b3.bco", 65504);
    myNex.writeStr("b3.txt","Disabled");
    
  }
}

void ManualTurnScreen() {
    Serial.print("page page10");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    myNex.writeNum("h0.val", int(manualTurn/2.55));
    myNex.writeNum("n0.val", int(manualTurn/2.55));
}

void StorePreset(int Address) {
  EEPROM.put(Address, display_azimuth);
  EEPROM.put(Address+2, v_Position);
  if (setProgramAction == 0){
    HomeScreen();
  }
  else {
     PresetDisplay();
  }
}

void PresetDisplay() { // Read and display preset azimuths
  int temp=0;
  EEPROM.get(60, temp);
  myNex.writeStr("b0.txt",String(temp));    
  EEPROM.get(70, temp);
  myNex.writeStr("b1.txt", String(temp));
  EEPROM.get(80, temp);
  myNex.writeStr("b2.txt",String(temp));
  EEPROM.get(90,temp);
  myNex.writeStr("b3.txt", String(temp));
  EEPROM.get(100,temp);
  myNex.writeStr("b8.txt", String(temp));
  EEPROM.get(110,temp);
  myNex.writeStr("b9.txt", String(temp));
  EEPROM.get(120,temp);
  myNex.writeStr("b10.txt", String(temp));
  EEPROM.get(130,temp);
  myNex.writeStr("b11.txt", String(temp));
} // End of PresetDisplay

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

// Nextion triggers

void trigger0() {     // Recall Preset 1
  EEPROM.get(62,v_Target);
  RecallPreset(v_Target);
}
void trigger4() {    // Preset 1 Store
  int Address=60;
  StorePreset(Address);
}

void trigger1() {   // Preset 2 Recall
  EEPROM.get(72,v_Target);
  RecallPreset(v_Target);
}
void trigger5(){    // Preset 2 Store
  int Address=70;
  StorePreset(Address);
}

void trigger2() {   // Preset 3 Recall
  EEPROM.get(82,v_Target);
  RecallPreset(v_Target);
}
void trigger6() {    // Preset 3 Store
  int Address=80;
  StorePreset(Address);  
}

void trigger3() {   // Preset 4 Recall
  EEPROM.get(92,v_Target);
  RecallPreset(v_Target);
}
void trigger7() {    // Preset 4 Store
  int Address=90;
  StorePreset(Address);
}

void trigger8() {   // Preset 5 Recall
  EEPROM.get(102,v_Target);
  RecallPreset(v_Target);
}
void trigger12() {    // Preset 5 Store
  int Address=100;
  StorePreset(Address);
}

void trigger9() {   // Preset 6 Recall
  EEPROM.get(112,v_Target);
  RecallPreset(v_Target);
}
void trigger13() {    // Preset 6 Store
  int Address=110;
  StorePreset(Address);  
}

void trigger10() {   // Preset 7 Recall
  EEPROM.get(122,v_Target);
  RecallPreset(v_Target);
}
void trigger14() {    // Preset 7 Store
  int Address=120;
  StorePreset(Address);
}

void trigger11() {   // Preset 8 Recall
  EEPROM.get(132,v_Target);
  RecallPreset(v_Target);
}
void trigger15() {    // Preset 8 Store
  int Address=130;
  StorePreset(Address);
}


void trigger16() {  // Switch to operating mode
  HomeScreen();
}

void trigger17() {  // Switch to programming mode
   ProgramScreen(); 
}

void trigger18() {  // Slide and go
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

void trigger22() { // Set rotor speed
  slowTurn = myNex.readNumber("n0.val");
  slowTurn = int(slowTurn*2.55);
  fastTurn = myNex.readNumber("n1.val");
  fastTurn = int(fastTurn*2.55);
  slowSpan = myNex.readNumber("n2.val");
  manualTurn = myNex.readNumber("n3.val");
  manualTurn = int(manualTurn*2.55);
  EEPROM.put(40,slowTurn);
  EEPROM.put(42,fastTurn);
  EEPROM.put(44,slowSpan);
  EEPROM.put(52,manualTurn);
  Serial.print("page page3");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff); 
}

void trigger23() {   // Call rotor calibration from Menu
  Serial.print("page page4");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff); 
  int slow= int(slowTurn/2.55);
  int fast= int(fastTurn/2.55);
  int manual= int(manualTurn/2.55);
  myNex.writeNum("h0.val", slow);
  myNex.writeNum("n0.val", slow);
  myNex.writeNum("h1.val", fast);
  myNex.writeNum("n1.val", fast);
  myNex.writeNum("h2.val",slowSpan);
  myNex.writeNum("n2.val",slowSpan);
  myNex.writeNum("h3.val",manual);
  myNex.writeNum("n3.val",manual);
}

void trigger25() {    // Info screen for preset memories
  Serial.print("page page5");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  int tempi=0;
  float tempf=0.0;
  String tstring="";
  // Presets
  EEPROM.get(60,tempi);
  EEPROM.get(62,tempf);
  myNex.writeNum("n0.val",tempi);
  dtostrf(tempf,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t16.txt",Buffer);
  EEPROM.get(70,tempi);
  EEPROM.get(72,tempf);
  myNex.writeNum("n1.val",tempi);
  dtostrf(tempf,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t17.txt",Buffer);
  EEPROM.get(80,tempi);
  EEPROM.get(82,tempf);
  myNex.writeNum("n2.val",tempi);
  dtostrf(tempf,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t18.txt",Buffer);
  EEPROM.get(90,tempi);
  EEPROM.get(92,tempf);
  myNex.writeNum("n3.val",tempi);
  dtostrf(tempf,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t19.txt",Buffer);
  EEPROM.get(100,tempi);
  EEPROM.get(102,tempf);
  myNex.writeNum("n4.val",tempi);
  dtostrf(tempf,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t20.txt",Buffer);
  EEPROM.get(110,tempi);
  EEPROM.get(112,tempf);
  myNex.writeNum("n5.val",tempi);
  dtostrf(tempf,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t21.txt",Buffer);
  EEPROM.get(120,tempi);
  EEPROM.get(122,tempf);
  myNex.writeNum("n6.val",tempi);
  dtostrf(tempf,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t22.txt",Buffer);
  EEPROM.get(130,tempi);
  EEPROM.get(132,tempf);
  myNex.writeNum("n7.val",tempi);
  dtostrf(tempf,10,7,Buffer);
  tstring=String(Buffer);
  myNex.writeStr("t23.txt",Buffer);
  myNex.writeNum("n8.val",slowSpan);
}

void trigger26() { // Slider touch open target azimuth window
  myNex.writeNum("n1.pco",34815);
  return;
}

// Direct entry

void trigger27() { //Number 1
  directEntry=directEntry+"1";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger28() { //Number 2
  directEntry=directEntry+"2";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger29() { //Number 3
  directEntry=directEntry+"3";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger30() { //Number 4
  directEntry=directEntry+"4";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger31() { //Number 5
  directEntry=directEntry+"5";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger32() { //Number 6
  directEntry=directEntry+"6";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger33() { // Number 7
  directEntry=directEntry+"7";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger34() { //Number 8
  directEntry=directEntry+"8";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger35() { //Number 9
  directEntry=directEntry+"9";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger36() { // Enter Key
  int azimuth=directEntry.toInt();
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
    directEntry="";
    myNex.writeStr("t1.txt",directEntry);
 
}

void trigger37() { //Number 0
  directEntry=directEntry+"0";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger38() { // Erase
  directEntry="";
  myNex.writeStr("t1.txt",directEntry);
}

void trigger39() { // Mode behavior in program mode
  if (deMenu == 0) {
  Serial.print("page page8");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  }
  else if (manualTurnMenu == 0){
    Serial.print("page page10");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);    
  } else {
    HomeScreen();
  }
}

void trigger42(){ // Mode button behavior in Direct Entry Mode
  if(manualTurnMenu == 0) {
    ManualTurnScreen();
  } else {
    HomeScreen();
  }
}

void trigger40() {  // Enable/disable manualTuneMenu in System Preferences
  if (manualTurnMenu == 0) {  // Disable
    manualTurnMenu = 1;
  }
  else {                      //Enable
    manualTurnMenu = 0;
  }
  EEPROM.put(54,manualTurnMenu);
  SystemPreferences();
}

void trigger41() {    // Systems Preference menu
  SystemPreferences();
}

void trigger43() {
  if (deMenu == 0) { // Switch 
    deMenu = 1;
  }
  else {
    deMenu = 0;
  }
  EEPROM.put(46,deMenu);
  SystemPreferences();
}

void trigger44() {
  if (setProgramAction == 0) { // Switch 
    setProgramAction = 1;
  }
  else {
    setProgramAction = 0;
  }
  EEPROM.put(50,setProgramAction);
  SystemPreferences();
}

// Experimental Manual Tuning

void trigger45() {  // On Button Release
  int tempi=0;
  SendRotorSpeed(tempi);
  digitalWrite(CW,LOW);
  digitalWrite(CCW,LOW);
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

void trigger48() { // Call Manual Turn Screen
  ManualTurnScreen();
}

void trigger49() { // update manualTurn speed
  manualTurn=myNex.readNumber("n0.val");
  manualTurn=int(manualTurn*2.55);
  EEPROM.put(52, manualTurn);
}

void trigger50() { // System info - System Parameters
  Serial.print("page page12");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  String tstring="";
  // Rotor Info
  int tempi=int(slowTurn/2.55);
  myNex.writeNum("n1.val",tempi);
  tempi=int(fastTurn/2.55);
  myNex.writeNum("n0.val",tempi);
  tempi=int(manualTurn/2.55);
  myNex.writeNum("n2.val", tempi);
  myNex.writeNum("n3.val", slowSpan);
  // Positional voltage info
  dtostrf(v_Min,10,7,Buffer);
  tstring= String(Buffer);
  myNex.writeStr("t17.txt",tstring);
  dtostrf(v_Max,10,7,Buffer);
  tstring= String(Buffer);
  myNex.writeStr("t18.txt",tstring);
  dtostrf(v_Degree,10,7,Buffer);
  tstring= String(Buffer);
  myNex.writeStr("t19.txt",Buffer);
  // System oreferences
  if(deMenu == 0) {
    myNex.writeStr("t14.txt", "Enabled");
  }
  if(manualTurnMenu == 0){
    myNex.writeStr("t15.txt", "Enabled");
  }
  if(setProgramAction == 0){
    myNex.writeStr("t16.txt", "Enabled");
  }
}
