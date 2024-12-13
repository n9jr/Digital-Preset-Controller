// Arduino Nano as a slave Pulse Width Modulator (PWM)
// Joe Reed N9JR December 2024


#include <Wire.h>

#define PWM 10

byte Rx;
byte LastByte = 0;

void PWM_Handler(int pwm) {
  while(Wire.available()) {
    Rx=Wire.read();
  }
}

void setup() {
  pinMode(PWM,OUTPUT);
  Wire.begin(0x55);
  Wire.onReceive(PWM_Handler);
}

void loop() {
  if(Rx != LastByte) {
    analogWrite(PWM,Rx);
    LastByte=Rx;
  }
}
