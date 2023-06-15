/* Heltec Automation Receive communication test example
 *
 * Function:
 * 1. Receive the same frequency band lora signal program
 * 
 * 
 * this project also realess in GitHub:
 * https://github.com/HelTecAutomation/ASR650x-Arduino
 * */

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "PCF8575.h"

/*
 * set LoraWan_RGB to 1,the RGB active in loraWan
 * RGB red means sending;
 * RGB green means received done;
 */
#ifndef LoraWan_RGB
#define LoraWan_RGB 0
#endif

#define RF_FREQUENCY 915000000  // Hz

#define TX_OUTPUT_POWER 14  // dBm

#define LORA_BANDWIDTH 0         // [0: 125 kHz, \
                                 //  1: 250 kHz, \
                                 //  2: 500 kHz, \
                                 //  3: Reserved]
#define LORA_SPREADING_FACTOR 7  // [SF7..SF12]
#define LORA_CODINGRATE 1        // [1: 4/5, \
                                 //  2: 4/6, \
                                 //  3: 4/7, \
                                 //  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8   // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0    // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false


#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 30  // Define the payload size here

#define enable_pin GPIO5

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

int16_t txNumber;

int16_t rssi, rxSize;
int counter = 0;
bool lora_idle = true;

PCF8575 PCF(0x20);

void setup() {
  Serial.begin(115200);
  digitalWrite(enable_pin, HIGH);

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

  PCF.write(0, HIGH);
  PCF.write(1, LOW);
  delay(75);
  PCF.write(0, LOW);
  PCF.write(1, LOW);
  delay(425);
  PCF.write(2, HIGH);
  PCF.write(3, LOW);
  delay(75);
  PCF.write(2, LOW);
  PCF.write(3, LOW);
  delay(425);

  txNumber = 0;
  rssi = 0;

  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH, LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON, 0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  
  digitalWrite(enable_pin, LOW);
}



void loop() {
  if (lora_idle) {
    turnOffRGB();
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  rssi = rssi;
  rxSize = size;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  turnOnRGB(COLOR_RECEIVED, 0);
  Radio.Sleep();
  Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n", rxpacket, rssi, rxSize);
  if (counter % 2 == 0) {
    digitalWrite(enable_pin, HIGH);
    PCF.write(0, LOW);
    PCF.write(1, HIGH);
    delay(75);
    PCF.write(0, LOW);
    PCF.write(1, LOW);
    delay(2000);
    PCF.write(2, LOW);
    PCF.write(3, HIGH);
    delay(75);
    PCF.write(2, LOW);
    PCF.write(3, LOW);
    delay(75);
    digitalWrite(enable_pin, LOW);
    delay(500);
    counter++;
  }
  else {
    digitalWrite(enable_pin, HIGH);
    PCF.write(0, HIGH);
    PCF.write(1, LOW);
    delay(75);
    PCF.write(0, LOW);
    PCF.write(1, LOW);
    delay(2000);
    PCF.write(2, HIGH);
    PCF.write(3, LOW);
    delay(75);
    PCF.write(2, LOW);
    PCF.write(3, LOW);
    delay(75);
    digitalWrite(enable_pin, LOW);
    delay(500);
    counter++;
  }
  lora_idle = true;
}
