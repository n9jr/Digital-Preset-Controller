#include <Wire.h>

#define PWM 10

byte RxByte;
byte LastByte;

void I2C_RxHandler(int numbytes) {
  while(Wire.available()) {
    RxByte=Wire.read();
  }
}

void setup() {
  pinMode(PWM,OUTPUT);
  Wire.begin(0x55);
  Wire.onReceive(I2C_RxHandler);
}

void loop() {
  if(RxByte != LastByte) {
    analogWrite(PWM,RxByte);
    LastByte=RxByte;
  }
}
