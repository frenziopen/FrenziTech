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

struct route{
   uint16_t nodeID;
   int16_t rssi;
};

States_t state;
bool sleepMode = false;
int16_t Rssi, rxSize;
uint16_t myID = getID() >> 32;
const uint8_t nodecount = 4;
route routes[nodecount];
uint16_t destID;
int16_t testMsg;
int currentTime = 0;
int prevTime = 0;

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
  destID = 50087;
  testMsg = 9;
  // Initialize routing table
  for (int i=0; i<nodecount; i++){
    routes[i].nodeID = 0;
    routes[i].rssi = 0;
  }
  state = TX_ag;
}



void loop() {
 
  if (currentTime < (prevTime + 1000*30) {
    //cool stuff
    currentTime = millis();
  }
  else {
    Serial.println("Broadcasting packet...");
    destID = 1;
    testMsg = 1;
    state = TX_ag;
    prevTime = currentTime;
  }

  switch (state) {
    case TX_ag:

      // Send discovery packet to all nodes in range
      delay(1000);
      sprintf(txpacket, "%d", myID);
      sprintf(txpacket + strlen(txpacket), ",%d", destID);
      sprintf(txpacket + strlen(txpacket), ",%d", Rssi);
      sprintf(txpacket + strlen(txpacket), ",%d", testMsg);

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

//-------------------------------------------------SUBPROGRAMS-------------------------------------------------//

/*  FUNCTION: OnTxDone
    PURPOSE: Once message is sent, print completion message to Serial Monitor and switch to recieving mode.
    INPUTS: None
    LAST EDIT: < 07/11/2023
    FURTHER NOTES: None
*/
void OnTxDone(void) {
  Serial.print("TX done......");
  state = RX_ag;
}

/*  FUNCTION: OnTxTimeout
    PURPOSE: In the occurance that the radio is unable to send a packet within the timeout value,
    print a timeout message to Serial Monitor, and returns to transmitting mode
    INPUTS: None
    LAST EDIT: < 07/11/2023
    FURTHER NOTES: None
*/
void OnTxTimeout(void) {
  Radio.Sleep();
  Serial.print("TX Timeout......");
  state = TX_ag;
}

/*  FUNCTION: OnRxDone
    PURPOSE: Receive packet and update routing table 
             and relay packet to destination node
    INPUTS: (int) Message being recieved,
            (int) Size of said message,
            (int) Signal strength of message,
            (int) snr
    LAST EDIT: 07/11/2023
    FURTHER NOTES: Function that is very subject to change
*/
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  Rssi = rssi;
  rxSize = size;
  uint16_t sender_node_id;
  String rx_array[4] = {"", "", "" , ""};
  int count = 0;

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  Radio.Sleep();

  // Split rxpacket using , as separator   
  for (int j = 0; j < size; j++) {
    if (rxpacket[j] != ',') {
      rx_array[count].concat(rxpacket[j]);
    }
    else {
      count++;
    }   
  }
  Serial.println("\n");
  for(int i = 0; i < 4; i ++){
    Serial.println("\n");
    Serial.print(rx_array[i]);
  } 

  sender_node_id = rx_array[0].toInt();
  destID = rx_array[1].toInt();

  Serial.printf("\r\n----This is Node %d and its routing Table----", myID);
  bool match_found = false;
  for (int i = 0; i < nodecount; i++) {
    // Update routing table for all nodes
    if (routes[i].nodeID == sender_node_id) {
      routes[i].rssi = rssi;
      match_found = true;
      break;
    } 
  }
  // Add a new route entry
  if(!match_found) {
    for (int j=0; j < nodecount; j++){
      if(routes[j].nodeID == 0){
        routes[j].nodeID = sender_node_id;
        routes[j].rssi = rssi;
        break;
      }
    }
  }

  for (int k=0; k<nodecount; k++){
     Serial.printf("\r\nNode: %ld, RSSI: %d", routes[k].nodeID, routes[k].rssi);
  }
  // If packet received from Destination Node then send acknowledgement packet
  if (destID == myID) {
    Serial.printf("\r\nReached Destination - Message received %d ", rx_array[3].toInt());
  } else {
    // Check if the destination node is in the routing table
    for (int i = 0; i < nodecount; i++) {
      // Find a match node with destination
      if (routes[i].nodeID == destID) {
        if(routes[i].rssi != 0) {
          // Match found, relay the received packet
          Serial.printf("\r\nRelaying packet to destination node %d ", destID);
          testMsg = rx_array[3].toInt();
          state=TX_ag;
          return;
        }
      }
    }
    Serial.printf("\r\nreceived packet \"%s\" with Rssi %d , length %d - Destination not in range\r\n", rxpacket, Rssi, rxSize);
  }
  
  Serial.println("wait for next packet");

  state=RX_ag;
  
}
