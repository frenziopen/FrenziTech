/*----------------------------------------------------------------------------
  NAME :            FrenziPump Timer
  COPYRIGHT FrenziTech 2023
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
#define VERSION V1 .0

#include "heltec.h"
#include "BluetoothSerial.h"  // Header File for Serial Bluetooth, will be added by default into Arduino
#include <rom/ets_sys.h>
#include <stdio.h>
#include <string.h>
#include <esp_sleep.h>
#include <soc/rtc.h>
#include <esp_bt.h>
#include <esp_bt_main.h>


#define GPIO25 25
#define MOT_RLC1_GPO32 32  //new board 32
#define MOT_RLC2_GPO33 33  //new board 33
#define BAND    915E6 //you can set band here directly,e.g. 868E6,915E6

BluetoothSerial BTSerial;  // Object for Bluetooth

// Control
bool MANUAL_START = 0;
bool MANUAL_STOP = 1;
bool MOTOR_RUN = 0;
bool CONFIG_PUMP = 0;
bool DEBUG = 1;
bool newMessage = 0;
// These are the global variables
//String motorStartTimeHr = "10";
//String motorStartTimeMin = "10";
//String motorRunTimeHr = "10";
//String motorRunTimeMin = "10";
//String motorRunInterval = "10";
//String motorVoltage = "10";
//String motorCurrent = "10";
//String motorEffeciency = "10";
//String soilMoisturePercent = "10";
//String soilTemperature = "10";
//String airTemperature = "10";
//String currentTimeHr = "10";
//String currentTimeMin = "10";
//String programButton = "0";
//String manualStartButton = "0";
//String manualStopButton = "0";

char BTConfig[100];
int ConfigByte = 0;
int counter = 0;
void setup() {
  Serial.begin(115200);                     // Start Serial monitor in 115200
  BTSerial.begin("Frenzimned Therapy BT");  // Name of your Bluetooth Signal
  Serial.println("Bluetooth Device is Ready to Pair");
  memset(BTConfig, '\0', 100);  // Initialize the string
  pinMode(GPIO25, OUTPUT);
  pinMode(MOT_RLC1_GPO32, OUTPUT);
  pinMode(MOT_RLC2_GPO33, OUTPUT);
  // power on pulse
  digitalWrite(GPIO25, HIGH);
  delay(500);
  digitalWrite(GPIO25, LOW);
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);

}

/*----------------------------------------------------------------------------
  NAME :            FrenziPump Timer
  COPYRIGHT FrenziTech 2023
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
void loop() {
  // put your main code here, to run repeatedly:
  BTScan();

  if (MANUAL_START) {
    // power on pulse
    digitalWrite(MOT_RLC1_GPO32, HIGH);
  }
  if (MANUAL_STOP) {
    digitalWrite(MOT_RLC1_GPO32, LOW);
  }
  if (CONFIG_PUMP){
    Serial.println(BTConfig);
    char msb = BTConfig[0]-'0';
    char lsb = BTConfig[1]-'0';
    int stHr = (msb) * 10 + (lsb) ;
    Serial.println(stHr);
    CONFIG_PUMP = 0;
  }
}
/*----------------------------------------------------------------------------
  NAME :            FrenziPump Timer
  COPYRIGHT FrenziTech 2023
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
//Serial.println("Bluetooth Device is Ready to Pair");
void BTScan() {
  String BTString = receiveBTData(DEBUG);
  
  if (newMessage == 1) {
    newMessage = 0;
    // check if the desired answer  is in the response of the module
    if (BTString == "PhoneConnect") {
      Serial.println("Bluetooth Device is connected ...");
    }
    Serial.println("Aadil");
    BTString.trim();
    Serial.println(BTString);
    BTSerial.println(BTString);
    if (BTString == "1") {
      MANUAL_START = 1;
      MANUAL_STOP = 0;
      BTSerial.println("OK");
      sendLoraData()
      BTSerial.flush();
    }
    if (BTString == "0") {
      MANUAL_START = 0;
      MANUAL_STOP = 1;
      BTSerial.println("OK");
      BTSerial.flush();
    }
    if (BTString == "ConfigStart") {
      CONFIG_PUMP = 1;
      ConfigByte = 0;
      BTSerial.println("OK");
      BTSerial.flush();
      while (newMessage == 0) {
        //BTConfig = receiveBTData(DEBUG);
        while (BTSerial.available()) {
          BTConfig[ConfigByte] = BTSerial.read();
          Serial.println(BTConfig[ConfigByte]);
          if (BTConfig[ConfigByte] == 'X') {
            newMessage = 1;
            Serial.println(BTConfig[ConfigByte]);
            Serial.println(ConfigByte);
          }
          ConfigByte++;
        }
      }
      newMessage = 0;
      BTSerial.println("OK");
      BTSerial.flush();
    }
  }
}

/*----------------------------------------------------------------------------
  NAME :            FrenziPump Timer
  COPYRIGHT FrenziTech 2023
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
String sendBTData(String command, const int timeout, boolean debug) {
  String response = "";
  BTSerial.println(command);
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (BTSerial.available()) {
      response = BTSerial.readString();
    }
  }
  if (debug) {
    Serial.println(response);
  }
  return response;
}

/*----------------------------------------------------------------------------
  NAME :            FrenziPump Timer
  COPYRIGHT FrenziTech 2023
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
String receiveBTData(boolean debug) {
  // Serial.println("Bluetooth Device is Ready to Pair");
  String response = "";
  while (BTSerial.available()) {
    char c = BTSerial.read();
    if (c != '\n') {
      response += c;
      newMessage = 1;
    }
  }
  if (debug & newMessage) {
    Serial.println(response);
  }
  return response;
}

void sendLoraData(bool motorSwitch) {
  Serial.print("Sending packet: ");
  Serial.println(counter);
  Serial.println(motorSwitch);
  // send packet
  LoRa.beginPacket();
/*
* LoRa.setTxPower(txPower,RFOUT_pin);
* txPower -- 0 ~ 20
* RFOUT_pin could be RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
*   - RF_PACONFIG_PASELECT_PABOOST -- LoRa single output via PABOOST, maximum output 20dBm
*   - RF_PACONFIG_PASELECT_RFO     -- LoRa single output via RFO_HF / RFO_LF, maximum output 14dBm
*/
  LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
  LoRa.print("hello ");
  LoRa.print(counter);
  LoRa.endPacket();
  
  counter++;
  digitalWrite(25, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(25, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}