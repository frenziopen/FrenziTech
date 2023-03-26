#include "Arduino.h"
#include "heltec.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_sleep.h>
#include <soc/rtc.h>
#include "WiFi.h"

#define VERSION    1.1
/* COMMENTS:
   V1.1 ADDED END_SESSION CMD, BECAUSE READING SENSOR IN BETWEEN CAUSIMG ERR
*/

#define GPI38      38
#define GPIO23     23
#define GPIO25     25

/*----------------------------------------------------------------------------
  GLOBAL Defines
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
#define BAND                  915E6                     //you can set band here directly,e.g. 868E6,915E6
#define uS_2MHZ_FACTOR        1                         // Conversion factor 2Mhz clock
#define uS_TO_S_FACTOR        1000000ULL/uS_2MHZ_FACTOR // Conversion factor for micro seconds to seconds 
#define TIME_TO_SLEEP         5/uS_2MHZ_FACTOR          // Time ESP32 will go to sleep (in seconds) 
#define RS485_DELAY           1000                      // ms
#define STARTUP_DELAY         100/uS_2MHZ_FACTOR        // initial delay after 80Mhz clock change
#define BLINK_DEL_100         100/uS_2MHZ_FACTOR
#define BLINK_DEL_10          10/uS_2MHZ_FACTOR

#define PING_CMD              0x05
#define SET_CLIENT_ADDR_CMD   0x06
#define GET_CLIENT_MAC_CMD    0x07
#define SEND_CLIENT_MAC_CMD   0x08
#define READ_DATA_CMD         0x09
#define WRITE_DATA_CMD        0x0A
#define END_SESSION_CMD       0x0B

#define SENSOR_BYTES          6
#define TOTAL_CLIENTS         10
#define DEBUG                 1
/*----------------------------------------------------------------------------
  GLOBAL Variables
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
unsigned int Counter = 0;
String DisplayString = "BISMILLAH \n CLIENT";
byte MacAdd[6];
byte ClientMacAdd[6 * TOTAL_CLIENTS];

byte ServerAddress   = 0xAA; // Fixed address for Server
byte ClientAddress   = 0x00; // Initially send 0 to client address
byte ClientAddressList [TOTAL_CLIENTS];
byte ClientRssiList    [TOTAL_CLIENTS];
byte ClientSnrList     [TOTAL_CLIENTS];

byte ReadCmd           = 0;
byte ReadServerAddress = 0;
byte ReadClientAddress = 0;
int  ReadRssi          = 0;
int  ReadSnr           = 0;

/*
   https://www.circuitschools.com/
   Interfacing Soil NPK Sensor with Arduino for measuring
   SoilSensorCmdgen, Phosphorus, and Potassium nutrients
*/

// RO to pin RX & DI to pin TX
// Address | Function_Code | Start Address (Hi) | Start Address (Lo) | Number of Points (Hi) | Number of Points (Lo) | Error Check (Lo) | Error Check (Hi)
// 0x01    | 0x03          | 0x00               | 0x00               | 0x00                  | 0x03
const byte SoilSensorCmd[]  = {0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x05, 0xcb}; // TX:01 03 00 00 00 01 84 0a
byte SMTSensorValues[SENSOR_BYTES] = {0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6};;
byte ClientSMTSensorValues[SENSOR_BYTES * TOTAL_CLIENTS];
float SoilMoisture;
float SoilTemperature;
float SoilConductivity;

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


/*----------------------------------------------------------------------------
  NAME :            setup
  DESCRIPTION :     Intialize the device
  INPUTS :          none
  OUTPUTS :         void
  PROCESS :
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
void setup() {
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
  delay(STARTUP_DELAY);                     // wait for a second
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, false /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  WiFi.macAddress(MacAdd);
  DisplayString = DisplayString + "\n CMAC:" + String(MacAdd[0], HEX) + ":" + String(MacAdd[1], HEX) + \
                  ":" + String(MacAdd[2], HEX) + ":" + String(MacAdd[3], HEX) + ":" + String(MacAdd[4], HEX) + \
                  ":" + String(MacAdd[5], HEX);
  DisplayString = DisplayString + "\n SMAC:" + WiFi.macAddress();

  // change frequency to 2MHz
  //rtc_clk_cpu_freq_set(RTC_CPU_FREQ_2M);
  OLEDisplayString (0, 10, DisplayString);
  BlinkLed(BLINK_DEL_100);
  LoRa.setTxPower(20, RF_PACONFIG_PASELECT_PABOOST);

  /*
    LoRa.setTxPower(txPower,RFOUT_pin);
    txPower -- 0 ~ 20
    RFOUT_pin could be RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
      - RF_PACONFIG_PASELECT_PABOOST -- LoRa single output via PABOOST, maximum output 20dBm
      - RF_PACONFIG_PASELECT_RFO     -- LoRa single output via RFO_HF / RFO_LF, maximum output 14dBm
  */

  gpio_wakeup_enable(GPIO_NUM_26, GPIO_INTR_HIGH_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  BlinkLed(BLINK_DEL_100);
  OLEDisplayString (0, 10, DisplayString);
  Serial.begin(115200);
  Serial2.begin(4800);
  pinMode(GPI38,  INPUT);
  LoRa.receive();
  //  esp_bluedroid_disable();
  //  esp_bt_controller_disable();
  //  esp_wifi_stop();
  //  esp_light_sleep_start();
}

/*----------------------------------------------------------------------------
  NAME :            loop
  DESCRIPTION :     Intialize the device
  INPUTS :          none
  OUTPUTS :         void
  PROCESS :
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
void loop() {

  esp_bluedroid_disable();
  esp_bt_controller_disable();
  esp_wifi_stop();
  esp_light_sleep_start();

  // receive back Ping
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    LoRaReceiveCmdResp(packetSize);
  }
  if (ReadCmd == PING_CMD)
  {
    // Send Ping
    LoRaSendCmd(PING_CMD);
  }
  /*  else if (ReadCmd == SET_CLIENT_ADDR_CMD)
    { // Send client address update
      Serial.println("SET_CLIENT_ADDR_CMD");

      if ((ServerAddress == ReadServerAddress) && (ClientAddress != ReadClientAddress)) {
        Serial.println("2. SET_CLIENT_ADDR_CMD");

        ClientAddress = ReadClientAddress;
      }
      // Send client address update
      LoRaSendCmd(SET_CLIENT_ADDR_CMD);
    }*/
  else if (ReadCmd == GET_CLIENT_MAC_CMD)
  {
    // Send client MAC update
    LoRaSendData(SEND_CLIENT_MAC_CMD, MacAdd, 6);
  }
  else if (ReadCmd == READ_DATA_CMD)
  {
    // Send client MTC OLD DATA
    LoRaSendData(WRITE_DATA_CMD, SMTSensorValues, SENSOR_BYTES);
  }
  else if (ReadCmd == END_SESSION_CMD)
  {
    // GET NEW MTC DATA
    ReadSoilMTC();
    // int aVin = analogRead(GPI38);
    // float dVin = floatMap(aVin, 0, 4095, 0, 3.3);// + random(900);
  }
  LoRa.receive();
}
/*----------------------------------------------------------------------------
  NAME :            blinkLed (int wait)
  DESCRIPTION :     Blink the LED at GPIO25
  INPUTS :          int wait, LED on-off delay in ms
  OUTPUTS :         void
  PROCESS :         Turn on the LED for wait ms and off for wait ms.
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
void BlinkLed (int wait)
{
  digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(wait);               // wait for a second
  digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
  delay(wait);
}

/*----------------------------------------------------------------------------
  NAME :            OLEDisplayString (int16_t XMove, int16_t YMove, String DisplayString)
  DESCRIPTION :     Blink the LED at GPIO25
  INPUTS :          int16_t xMove : x position of display
                    int16_t yMove : y position of display
                    String displayString : message to be displayed
  OUTPUTS :         void
  PROCESS :         Displays a message on OLED, it resets everything.
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
void OLEDisplayString (int16_t XMove, int16_t YMove, String DisplayString)
{
  Heltec.display -> clear();
  Heltec.display -> setFont(ArialMT_Plain_10); // ArialMT_Plain_10, ArialMT_Plain_16, ArialMT_Plain_24
  Heltec.display -> drawString(XMove, YMove, DisplayString); //"BISMILLAH");
  Heltec.display -> display();
  Serial.println(DisplayString);
  Serial.flush();
}



/*----------------------------------------------------------------------------
  NAME :            LoRaSendCmd(byte Command)
  DESCRIPTION :     Send a Command to a client
  INPUTS :          Command
  OUTPUTS :         void
  PROCESS :         Send Command, server and client addresses and wait for
                    reply.
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
void LoRaSendCmd(byte Command)
{
  //LoRa.setTxPower(20, RF_PACONFIG_PASELECT_PABOOST);
  LoRa.beginPacket();                   // start packet
  LoRa.write(Command);                  // seng Command
  LoRa.write(ServerAddress);            // send server address
  LoRa.write(ClientAddress);            // send client address
  LoRa.endPacket();                     // finish packet and send it
#ifdef DEBUG
  DisplayString =  "CLI TX cmd:" + String(Command) + \
                   " SA:" + String(ServerAddress) + " CA:" + String(ClientAddress) + "\n";
  OLEDisplayString (0, 0, DisplayString);
#endif
  LoRa.receive();                       // turn receive on
}


void LoRaSendData(byte Command, const uint8_t *buffer, int msgSize)
{
  //LoRa.setTxPower(20, RF_PACONFIG_PASELECT_PABOOST);
  int byteNo = 0;
  LoRa.beginPacket();                   // start packet
  LoRa.write(Command);                  // seng Command
  LoRa.write(ServerAddress);            // send server address
  LoRa.write(ClientAddress);            // send client address
  LoRa.write(msgSize);                  // add payload length
  for (byteNo = 0; byteNo < msgSize; byteNo++)
  {
    LoRa.write(buffer[byteNo]);
  }
  LoRa.endPacket();                     // finish packet and send it
#ifdef DEBUG
  DisplayString =  "TXdata:" + String(buffer[0], HEX) + ":" + String(buffer[1], HEX) + \
                   ":" + String(buffer[2], HEX) + ":" + String(buffer[3], HEX) + ":" + String(buffer[4], HEX) + \
                   ":" + String(buffer[5], HEX) + "\n" \
                   "Bytes:" + String(msgSize) + " cmd:" + String(Command) + \
                   " SA:" + String(ReadServerAddress) + " CA:" + String(ReadClientAddress) + "\n";
  OLEDisplayString (0, 0, DisplayString);
#endif
  LoRa.receive();                       // turn receive on
}
/*----------------------------------------------------------------------------
  NAME :            LoRaReceiveCmdResp(int PacketSize, byte Command)
  DESCRIPTION :     Receive Command response from client
  INPUTS :          PacketSize and return command code
  OUTPUTS :         void
  PROCESS :         Receive command, server and client addresses.
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
void LoRaReceiveCmdResp(int PacketSize)
{
  if (PacketSize == 0) return;            // if there's no packet, return 0
  ReadCmd           = LoRa.read();     // PING_CMD, SET_CLIENT_ADDR_CMD, READ_DATA_CMD
  ReadServerAddress = LoRa.read();     // server address returned
  ReadClientAddress = LoRa.read();     // recipient address
  ReadRssi          = LoRa.packetRssi();
  ReadSnr           = LoRa.packetSnr();
#ifdef DEBUG
  DisplayString =  "CLI CMDR pkt:" + String(PacketSize) + " cmd:" + String(ReadCmd) + \
                   "\nSA:" + String(ReadServerAddress) + " CA:" + String(ReadClientAddress) + "\n";
  OLEDisplayString (0, 0, DisplayString);
#endif
  LoRa.receive();                       // turn receive on
}


/*----------------------------------------------------------------------------
  NAME :            ReadSoilMTC()
  DESCRIPTION :     Receive Command response from client
  INPUTS :          PacketSize and return command code
  OUTPUTS :         void
  PROCESS :         Receive command, server and client addresses.
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/

void ReadSoilMTC() {
  // clear the receive buffer
  Serial2.flush();

  // write out the message
  for (uint8_t sCmd = 0; sCmd < sizeof(SoilSensorCmd); sCmd++ ) Serial2.write( SoilSensorCmd[sCmd] );

  // delay to allow response bytes to be received!
  unsigned long previous = millis();
  // this loop waits for the answer
  do {
    // Waits for the asnwer with time out
  } while  ((millis() - previous) < RS485_DELAY);

  // read in the received bytes
  for (byte rxData = 0; rxData < SENSOR_BYTES; rxData++) {
    if (rxData > 2 || rxData < 9) {
      //SMTSensorValues[rxData-3] = Serial2.read();
    }
    else {
      Serial2.read();
    }
    // Serial.print(SMTSensorValues[i], HEX);
    //  Serial.print(' ');
  }
  SoilMoisture      = ((SMTSensorValues[0] << 8) | SMTSensorValues[1]);
  SoilTemperature   = ((SMTSensorValues[2] << 8) | SMTSensorValues[3]);
  SoilConductivity  = ((SMTSensorValues[4] << 8) | SMTSensorValues[5]);
  DisplayString =  "SM:" + String(SoilMoisture) + "\nST:" + String(SoilTemperature) + "\nSC:" + String(SoilConductivity) + "\n";
  OLEDisplayString (0, 0, DisplayString);
  LoRa.receive();                       // turn receive on
}
