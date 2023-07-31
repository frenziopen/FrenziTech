#include "Arduino.h"

#define power GPIO8 
#define sol_F GPIO10
#define sol_R GPIO11

void setup() {
  pinMode(power, OUTPUT);
  pinMode(sol_F, OUTPUT);
  pinMode(sol_R, OUTPUT);

  digitalWrite(sol_F, LOW);
  digitalWrite(sol_R, LOW);
  digitalWrite(power, HIGH);
  delay(1000);

}

void loop() {
  digitalWrite(sol_F, HIGH);
  digitalWrite(sol_R, LOW);
  delay(1000);
  digitalWrite(sol_F, LOW);
  digitalWrite(sol_R, HIGH);
  delay(1000);
}
