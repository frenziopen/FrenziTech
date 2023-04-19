#include <SPI.h>
#include <LoRa.h>

void setup() {
  Serial.begin(9600);
  while (!Serial);

  LoRa.setPins(5, 14, 2);

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("LoRa Master ready");
}

void loop() {
  String message = "Hello Slave!";
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  Serial.println("Message sent to slave: " + message);

  // Wait for response from slave
  while (LoRa.parsePacket() == 0) {
    delay(100);
  }

  // Receive response from slave
  String response = "";
  while (LoRa.available()) {
    response += (char)LoRa.read();
  }
  Serial.println("Message received from slave: " + response);

  // Send acknowledge to slave
  String ack = "Message received!";
  LoRa.beginPacket();
  LoRa.print(ack);
  LoRa.endPacket();

  Serial.println("Acknowledge sent to slave: " + ack);
}
