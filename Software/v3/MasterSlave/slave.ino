#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>
char json_data[128];
String node, state, message;
int actuator1;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  LoRa.setPins(5, 14, 2);

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("LoRa Slave ready");
}

/*----------------------------------------------------------------------------
  NAME :            breakJsonPacket()
  DESCRIPTION :     break a JSON packet to read data
  INPUTS :          Command
  OUTPUTS :         void
  PROCESS :         Read sender name, destination name, state for the destination
                    to send or not send, actuators control.
  AUTHOR :          Syed Mohiuddin Zia
  DATE:             19/04/2023
  MODIFIED BY :
  COMMENTS:
  VERSION:          V3.0
  -----------------------------------------------------------------------------*/
void breakJsonPacket()
{
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {Serial.println("JSON parsing failed.");return;}
    actuator1 = doc["actuator1"].as<int>();
    Serial.println("sensor1 value: " + String(sensor1));
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    message="";
    while (LoRa.available()) {
      message += (char)LoRa.read();
    }
    Serial.println("Message received from master: " + message);
    breakJsonPacket();
    // Send response to master
    String response = "Hello Master!";
    LoRa.beginPacket();
    LoRa.print(response);
    LoRa.endPacket();

    Serial.println("Response sent to master: " + response);

    // Wait for acknowledge from master
    while (LoRa.parsePacket() == 0) {
      delay(100);
    }

    // Receive acknowledge from master
    String ack = "";
    while (LoRa.available()) {
      ack += (char)LoRa.read();
    }
    Serial.println("Acknowledge received from master: " + ack);
  }
}
