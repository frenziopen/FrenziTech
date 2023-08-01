#include "Arduino.h"
#include <Wire.h>  
#include "HT_SH1107Wire.h"

SH1107Wire  display(0x3c, 500000, SDA, SCL ,GEOMETRY_128_64,GPIO10); // addr, freq, sda, scl, resolution, rst

#define power GPIO8 
#define sol_F GPIO11
#define sol_R GPIO12
#define temp_IN ADC
#define sense_POW GPIO9
#define GPIO16 VBAT_ADC_CTL

void setup() {
  Serial.begin(9600);
  pinMode(power, OUTPUT);
  pinMode(sol_F, OUTPUT);
  pinMode(sol_R, OUTPUT);
  pinMode(sense_POW, OUTPUT);
  pinMode(GPIO16, OUTPUT);
  
  display.init();
  display.clear();
  display.display();
  display.setContrast(255);

  digitalWrite(sol_F, LOW);
  digitalWrite(sol_R, LOW);
  digitalWrite(power, HIGH);
  digitalWrite(sense_POW, HIGH);
  delay(1000);

}

void loop() {
  digitalWrite(GPIO16, HIGH);

  int temp = analogRead(temp_IN);
  Serial.println(temp);
  temp *= (5.0/4096.0); //volts
  temp = (temp - 0.5) * 100.0; //
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.clear();
  display.display();
  display.screenRotate(ANGLE_0_DEGREE);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 32-16/2, ("Temp in C: " + String(temp)));
  display.display();
  delay(1000);

  display.clear();
  display.display();
  display.screenRotate(ANGLE_0_DEGREE);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 32-16/2, (String(analogReadmV(temp_IN)*2/1000) + " V"));
  display.display();
  delay(1000);
 
}
