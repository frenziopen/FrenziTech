#define in1 GPIO2
#define in2 GPIO3

void setup() {
  Serial.begin(9600);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

void loop() {
  directionControl();
}

void directionControl() {
  // Turn on motor
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  Serial.println("On");
  delay(30);

  // Turn off motors
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  delay(1000);
  
  // Now change motor directions
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  Serial.println("Off");
  delay(30);
  
  // Turn off motors
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  delay(1000);
}