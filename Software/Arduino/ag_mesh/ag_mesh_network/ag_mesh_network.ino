/* AG Mesh Network
 *
 * Function:
 * 1. Ping Pong communication in two CubeCell device.
 * 
 * Description:
 * 1. Only hardware layer communicate, no LoRaWAN protocol support;
 * 2. Download the same code into two CubeCell devices, then they will begin Ping Pong test each other;
 * 3. This example is for CubeCell hardware basic test.
 
 * */

#include "LoRaWan_APP.h"
//#include "Arduino.h"

/*
 * set LoraWan_RGB to 1,the RGB active in loraWan
 * RGB red means sending;
 * RGB green means received done;
 */
#ifndef LoraWan_RGB
#define LoraWan_RGB 0
#endif

#define RF_FREQUENCY 915000000  // Hz

#define TX_OUTPUT_POWER 20  // dBm

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
#define BUFFER_SIZE 50  // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
void OnTxDone(void);
void OnTxTimeout(void);
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

typedef enum {
  LOWPOWER,
  RX_ag,
  TX_ag
} States_t;

States_t state;
bool sleepMode = false;
int16_t Rssi, rxSize;
int myID = 0x02;
const uint8_t nodecount = 4;
int16_t routes[nodecount][4] = { { 1, 0 }, { 2, 255 }, {3, 0}, {4, 0} };
uint8_t destID;

void setup() {
  Serial.begin(115200);

  //Mcu.begin();

  Rssi = 0;

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  state = TX_ag;
}



void loop() {
  destID = 0x02;
  switch (state) {
    case TX_ag:

      // Send discovery packet to all nodes in range
      delay(1000);
      sprintf(txpacket, "%d", myID);
      sprintf(txpacket + strlen(txpacket), ",%d", destID);
      sprintf(txpacket + strlen(txpacket), ",%d", Rssi);

      //turnOnRGB(COLOR_SEND, 0);

      Serial.printf("\r\nsending packet \"%s\" , length %d\r\n", txpacket, strlen(txpacket));

      Radio.Send((uint8_t *)txpacket, strlen(txpacket));
      state = LOWPOWER;
      break;
    case RX_ag:
      Serial.println("into RX mode");
      Radio.Rx(0);
      state = LOWPOWER;
      break;
    case LOWPOWER:
      //lowPowerHandler();
      break;
    default:
      break;
  }
  Radio.IrqProcess();
}

void OnTxDone(void) {
  Serial.print("TX done......");
  //turnOnRGB(0, 0);
  state = RX_ag;
}

void OnTxTimeout(void) {
  Radio.Sleep();
  Serial.print("TX Timeout......");
  state = TX_ag;
}
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  Rssi = rssi;
  rxSize = size;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  //turnOnRGB(COLOR_RECEIVED,0);
  Radio.Sleep();

  for (int j = 0; j < size; j++) {
    Serial.printf("\r\n character %d = %c ", j, rxpacket[j]);
  }
  Serial.printf("\r\n----This is Node %d and its routing Table----", myID);
  for (int i = 0; i < nodecount; i++) {
    // Update routing table for all nodes
    if (routes[i][0] == rxpacket[0] - '0') {
      routes[i][1] = Rssi;
    }
    Serial.printf("\r\nNode: %d, RSSI: %d", routes[i][0], routes[i][1]);
  }
  // If packet received from Destination Node then send acknowledgement packet
  if ((rxpacket[2] - '0') == myID) {
    Serial.printf("\r\nReached Destination - Send Acknowledgment back to node %c", rxpacket[0]);
  }

  Serial.printf("\r\nreceived packet \"%s\" with Rssi %d , length %d\r\n", rxpacket, Rssi, rxSize);
  Serial.println("wait for next packet");

  state=RX_ag;
}