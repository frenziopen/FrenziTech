//
//    FILE: PCF8575_isConnected.ino
//  AUTHOR: Rob Tillaart
//    DATE: 2021-01-03
// PUPROSE: demo device detection


#include "PCF8575.h"

// adjust addresses if needed
PCF8575 PCF(0x20);


void setup() {
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.print("PCF8575_LIB_VERSION:\t");
  Serial.println(PCF8575_LIB_VERSION);

  if (!PCF.begin()) {
    Serial.println("could not initialize...");
  }
  if (!PCF.isConnected()) {
    Serial.println("=> not connected");
  } else {
    Serial.println("=> connected!!");
  }
  /*
  PCF.write(0, LOW);
  PCF.write(1, LOW);*/
  PCF.write(2, LOW);
  PCF.write(3, LOW);
  PCF.write(4, LOW);
  PCF.write(5, LOW);
  PCF.write(6, LOW);
  PCF.write(7, LOW);
}


void loop() {
  
  Serial.println("ON");

  PCF.write(1, HIGH);
  PCF.write(0, LOW);
  delay(75);
  PCF.write(1, LOW);
  PCF.write(0, LOW);
  delay(2000);
  Serial.println("OFF");

  PCF.write(0, HIGH);
  PCF.write(1, LOW);
  delay(75);
  PCF.write(1, LOW);
  PCF.write(0, LOW);
  delay(2000);
}


// -- END OF FILE --
