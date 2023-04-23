#include <Wire.h>
#include "heltec.h"
#define GPI38      38
#define GPIO23     23
#define GPIO25     25

/*
 * https://www.circuitschools.com/
 * Interfacing Soil NPK Sensor with Arduino for measuring 
 * Nitrogen, Phosphorus, and Potassium nutrients
 */
 
// RO to pin RX & DI to pin TX
// Address | Function_Code | Start Address (Hi) | Start Address (Lo) | Number of Points (Hi) | Number of Points (Lo) | Error Check (Lo) | Error Check (Hi)
// 0x01    | 0x03          | 0x00               | 0x00               | 0x00                  | 0x03
const byte nitro[]  = {0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x05, 0xcb}; // TX:01 03 00 00 00 01 84 0a 
byte values[11];
float soilMoisture;
float soilTemperature;
float soilConductivity;

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup() {

  pinMode(GPIO23, OUTPUT);
  pinMode(GPIO25, OUTPUT);

  pinMode(GPI38,  INPUT);
  digitalWrite(GPIO23,HIGH);
  digitalWrite(GPIO25,HIGH);
//  delay(300);
//  digitalWrite(GPIO23,LOW);
  int aVin = analogRead(GPI38);
  float dVin = floatMap(aVin, 0, 4095, 0, 3.3);

  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, false /*Serial Enable*/);
  Heltec.display -> clear();
  Heltec.display -> setFont(ArialMT_Plain_24); // ArialMT_Plain_10, ArialMT_Plain_16, ArialMT_Plain_24
  Heltec.display -> drawString(0, 10, "BISMILLAH");
  String batteryVin = ("BAT = " + String(dVin, 1) + " V");
  Heltec.display -> drawString(0, 30, String(batteryVin));

  Heltec.display -> display();
  delay(5000);
  Serial.begin(4800);
  Serial.println("Initialize...");
  Serial2.begin(4800);
  
}
 int firstItt = 0;
void loop() {
  nitrogen();

  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, false /*Serial Enable*/);
  Heltec.display -> clear();
  Heltec.display -> setFont(ArialMT_Plain_24); // ArialMT_Plain_10, ArialMT_Plain_16, ArialMT_Plain_24
  String temperature = ("T = " + String(soilTemperature/10, 1) + " C"); 
  String humidity = ("H = " + String(soilMoisture/10, 1) + " %");
  String conductivity = ("C = " + String(soilConductivity/10, 1) + " u");
  Heltec.display -> drawString(0, 0, String(temperature));
  Heltec.display -> drawString(0, 20, String(humidity));
  Heltec.display -> drawString(0, 40, String(conductivity));
  
  Heltec.display -> display();
  delay(10000); 
  Serial2.begin(4800);
  delay(500);
  if (firstItt == 0)
  {
    digitalWrite(GPIO23, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);
    firstItt = 1;
    digitalWrite(GPIO23, HIGH);    // turn the LED off by making the voltage LOW
    delay(50000);
  }
  }
 
byte nitrogen() {
  // clear the receive buffer
  Serial2.flush();
 
  // write out the message
  for (uint8_t i = 0; i < sizeof(nitro); i++ ) Serial2.write( nitro[i] );
 
   // delay to allow response bytes to be received!
  delay(1200);
 
  // read in the received bytes
  for (byte i = 0; i < 11; i++) {
    values[i] = Serial2.read();
    // Serial.print(values[i], HEX);
   //  Serial.print(' ');
  }
  soilMoisture      = ((values[3] << 8) | values[4]);
  soilTemperature   = ((values[5] << 8) | values[6]);
  soilConductivity  = ((values[7] << 8) | values[8]);
  Serial.println(soilMoisture/10);
  Serial.println(soilTemperature/10);
  Serial.println(soilConductivity/10);
  

  return values[6];
}
 

 
