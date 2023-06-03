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
//#include <ESP32Time.h>

#define VERSION 1.5

// ADC INPUTS
#define VI_ADC0_GPI36 36
#define VI_ADC1_GPI37 37
#define VM_ADC2_GPI38 38
#define TM_ADC0_GPI39 39
#define R485_RX2_GPIO16 16
#define R485_TX2_GPIO17 17
//#define MOT_RLC1_GPO32 32  //new board 32
//#define MOT_RLC2_GPO33 33  //new board 33
#define MOT_RLC1_GPO32 23  //new board 32
#define MOT_RLC2_GPO33 25  //new board 33

// LTE Serial
// 22, 23, 2, 17, 16x, 12x, 13, 32, 33
// 36, 37, 38, 39
#define LTE_TX 12  // LTE transmit
#define LTE_RX 13  // LTE receive
HardwareSerial LTESerial(1);
BluetoothSerial BTSerial;  // Object for Bluetooth
//#define DEBUG true
#define MODE_1A
String from_usb = "";
String message = "";
char BTConfig[100];
int ConfigByte = 0;

// Control
bool MANUAL_START = 0;
bool MANUAL_STOP = 0;
bool MOTOR_RUN = 0;
bool CONFIG_PUMP = 0;
bool DEBUG = 1;
bool newMessage = 0;
float soilMoisture;
float soilTemperature;
float soilConductivity;
int tenMinDelay = 0;


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
/*-----------------------------------------------------------------------------
  // AC & DC Current Sensor with LCD By Solarduino

  // Note Summary
  // Note :  Safety is very important when dealing with electricity. We take no responsibilities while you do it at your own risk.
  // Note :  This AC & DC  Current Sensor Code is for HSTS016L Hall effect split core current transformer use.
  // Note :  The value shown in Serial Monitor / LCD Display is refreshed every second and measurement value is the average value of 4000 sample readings for nearly a second.
  // Note :  The current measured is the Root Mean Square (RMS) value.
  // Note :  The analog value per sample is squared and accumulated for every 4000 samples before being averaged. The averaged value is then getting square-rooted.
  // Note :  The unit provides reasonable accuracy and may not be comparable with other expensive branded and commercial product.
  // Note :  All credit shall be given to Solarduino.

  -----------------------------------------------------------------------------*/
/* 0- General */
int decimalPrecision = 2;                   // decimal places for all values shown in LED Display & Serial Monitor

/* 1- AC Current Measurement */
int currentAnalogInputPin = VI_ADC0_GPI36;  //A1;             // Which pin to measure Current Value (A0 is reserved for LCD Display Shield Button function)
int calibrationPin        = VI_ADC1_GPI37;         //A2;                    // Which pin to calibrate offset middle value
float manualOffset        = 0.00;           // Key in value to manually offset the initial value
float mVperAmpValue       = 6.25;           // If using "Hall-Effect" Current Transformer, key in value using this formula: mVperAmp = maximum voltage range (in milli volt) / current rating of CT
// For example, a 20A Hall-Effect Current Transformer rated at 20A, 2.5V +/- 0.625V, mVperAmp will be 625 mV / 20A = 31.25mV/A
// For example, a 50A Hall-Effect Current Transformer rated at 50A, 2.5V +/- 0.625V, mVperAmp will be 625 mV / 50A = 12.5 mV/A
// For example, a 100A Hall-Effect Current Transformer rated at 100A, 2.5V +/- 0.625V, mVperAmp will be 625 mV / 100A = 6.25 mV/A
float supplyVoltage      = 3300;            // Analog input pin maximum supply voltage, Arduino Uno or Mega is 5000mV while Arduino Nano or Node MCU is 3300mV
float offsetSampleRead   = 0;               /* to read the value of a sample for offset purpose later */
float currentSampleRead  = 0;               /* to read the value of a sample including currentOffset1 value*/
float currentLastSample  = 0;               /* to count time for each sample. Technically 1 milli second 1 sample is taken */
float currentSampleSum   = 0;               /* accumulation of sample readings */
float currentSampleCount = 0;               /* to count number of sample. */
float currentMean ;                         /* to calculate the average value from all samples, in analog values*/
float RMSCurrentMean ;                      /* square roof of currentMean, in analog values */
float FinalRMSCurrent ;                     /* the final RMS current reading*/
float FinalCurrent ;                        /* the final current reading*/

void setup() {
  Serial.begin(115200);  // Start Serial monitor in 115200
  LTESerial.begin(115200, SERIAL_8N1, LTE_RX, LTE_TX);


  memset(BTConfig, '\0', 100);  // Initialize the string

  pinMode(VI_ADC0_GPI36, INPUT);
  pinMode(VI_ADC1_GPI37, INPUT);
  pinMode(VM_ADC2_GPI38, INPUT);
  pinMode(TM_ADC0_GPI39, INPUT);

  pinMode(MOT_RLC1_GPO32, OUTPUT);
  pinMode(MOT_RLC2_GPO33, OUTPUT);  
    // disable relays
  digitalWrite(MOT_RLC1_GPO32, HIGH);
  digitalWrite(MOT_RLC2_GPO33, HIGH); 

  //delay(600);
  Serial.println(" Power ON ");
  // power on pulse
  //digitalWrite(GPIO25, HIGH);
  //delay(500);
  //digitalWrite(GPIO25, LOW);

  PowerOn();
  Serial.println(" Msg. ON ");
  // power on pulse
  sendData("AT+COPS=2\r\n", 3000, DEBUG);
  sendData("AT+CTZU=1\r\n", 3000, DEBUG);
  sendData("AT+COPS=0\r\n", 3000, DEBUG);
  sendData("AT+CCLK?\r\n", 3000, DEBUG);
  //ConfigureFTP(ftp_server,ftp_user_name,ftp_user_password);
  BTSerial.begin("Frenzimned Therapy BT");  // Name of your Bluetooth Signal
  Serial.println("Bluetooth Device is Ready to Pair");


/*delay(500);
  int aVin = analogRead(VM_ADC2_GPI38);
  float dVin = floatMap(aVin, 0, 4095, 0, 3.3);

  display.clear();
  display.setFont(ArialMT_Plain_16); // ArialMT_Plain_10, ArialMT_Plain_16, ArialMT_Plain_24
  display.drawString(0, 10, "BISMILLAH");
  String batteryVin = ("BAT = " + String(dVin, 1) + " V");
  display.drawString(0, 30, String(batteryVin));
  display.display();

  digitalWrite(GPIO25, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(100);                  // wait for a second
  digitalWrite(GPIO25, LOW);   // turn the LED off by making the voltage LOW
  delay(100);
  Serial.println(" Lora ON ");
*/
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
rms_current();
  if (MANUAL_START) {
    // power on pulse
    digitalWrite(MOT_RLC1_GPO32, LOW);
  }
  if (MANUAL_STOP) {
    digitalWrite(MOT_RLC1_GPO32, HIGH);
  }
  if (CONFIG_PUMP) {
    Serial.println(BTConfig);
    char msb = BTConfig[0] - '0';
    char lsb = BTConfig[1] - '0';
    int stHr = (msb)*10 + (lsb);
    Serial.println(stHr);
    CONFIG_PUMP = 0;
  }

  tenMinDelay = tenMinDelay + 1;
  if (  (tenMinDelay > 1000) && (MANUAL_STOP == 1)) {
    sendData("AT+CCLK?\r\n", 3000, DEBUG);
    tenMinDelay = 0;
    // put your main code here, to run repeatedly:
    // nitrogen();
    // power on pulse
    //digitalWrite(GPIO25, HIGH);
    //delay(500);
    //digitalWrite(GPIO25, LOW);
    String http_str = "AT+HTTPPARA=\"URL\",\"https://frenzitech.frenzivendi.com/sensor_service.php?soil_temp=" + String(soilTemperature / 10, 1) + \
                      "&soil_moisture=" + String(soilMoisture / 10, 1) + "&soil_ece=" + String(FinalRMSCurrent, 1) + "\"\r\n";
    //String http_str = "AT+HTTPPARA=\"URL\",\"https://frenzitech.frenzivendi.com/sensor_service.php?soil_temp=" + String(soilTemperature / 10, 1) + \
    //                  "&soil_moisture=" + String(soilMoisture / 10, 1) + "&soil_ece=" + String(soilConductivity / 10, 1) + "\"\r\n";
    Serial.println(http_str);
    sendData("AT+HTTPINIT\r\n", 2000, DEBUG);
    sendData(http_str, 2000, DEBUG);
    sendData("AT+HTTPACTION=0\r\n", 3000, DEBUG);
    sendData("AT+HTTPTERM\r\n", 3000, DEBUG);
    sendData("AT+CCLK?\r\n", 3000, DEBUG);
  }


}




int rms_current()                                                                                                   /*codes to run again and again */
{
  /* 1- AC & DC Current Measurement US 60Hz = 16666.66 uSec*/
  if (micros() >= currentLastSample + 100)                                                            /* every 0.1 milli second taking 1 reading */
  {
    int Vin1          = analogRead(currentAnalogInputPin);
    int Vin2          = analogRead(calibrationPin);
    currentSampleRead = Vin1 - Vin2;                                                                   /* read the sample value including offset value*/
    currentSampleSum = currentSampleSum + sq(currentSampleRead) ;                                      /* accumulate total analog values for each sample readings*/
    currentSampleCount = currentSampleCount + 1;                                                       /* to count and move on to the next following count */
    currentLastSample = micros();                                                                      /* to reset the time again so that next cycle can start again*/
  }

  if (currentSampleCount > 166)                                                                       /* after 166 count or 16.6 milli seconds (0.166 second), do this following codes*/
  {
    currentMean = currentSampleSum / currentSampleCount;                                              /* average accumulated analog values*/
    RMSCurrentMean = sqrt(currentMean);                                                               /* square root of the average value*/
    FinalRMSCurrent = (((RMSCurrentMean / 4095) * supplyVoltage) / mVperAmpValue) - manualOffset;
    /* calculate the final RMS current*/
    if (FinalRMSCurrent <= (625 / mVperAmpValue / 100))                                               /* if the current detected is less than or up to 1%, set current value to 0A*/
    {
      FinalRMSCurrent = 0;
    }
    // Serial.println(FinalRMSCurrent);
    currentSampleSum = 0;                                                                             /* to reset accumulate sample values for the next cycle */
    currentSampleCount = 0;                                                                           /* to reset number of sample for the next cycle */
    return 1;
  }
  return 0;
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
float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
/**************************Power on Sim7x00**************************/
/*----------------------------------------------------------------------------
  NAME :            FrenziPump Timer
  COPYRIGHT FrenziTech 2023
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
void PowerOn() {
  uint8_t answer = 0;
  // checks if the module is started
  answer = sendATcommand("AT", "OK", 2000);
  if (answer == 0) {
    // waits for an answer from the module
    while (answer == 0) {  // Send AT every two seconds and wait for the answer
      answer = sendATcommand("AT", "OK", 2000);
      Serial.print("Starting up...\n");
      delay(1000);
    }
  }
  delay(5000);
  while ((sendATcommand("AT+CREG?", "+CREG: 0,1", 500) || sendATcommand("AT+CREG?", "+CREG: 0,5", 500)) == 0)
    delay(500);
}


/**************************SMS sending and receiving message **************************/
/*----------------------------------------------------------------------------
  NAME :            FrenziPump Timer
  COPYRIGHT FrenziTech 2023
  AUTHOR :          Aamir Farooqui
  DATE:             08/11/22
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
//SMS sending short message
bool SendingShortMessage(const char* PhoneNumber, const char* Message) {
  uint8_t answer = 0;
  char aux_string[30];
  Serial.print("Setting SMS mode...\n");
  sendATcommand("AT+CMGF=1", "OK", 1000);  // sets the SMS mode to text
  Serial.print("Sending Short Message\n");
  sprintf(aux_string, "AT+CMGS=\"%s\"", PhoneNumber);
  answer = sendATcommand(aux_string, ">", 3000);  // send the SMS number
  if (answer == 1) {
    LTESerial.println(Message);
    LTESerial.write(0x1A);
    answer = sendATcommand("", "OK", 20000);
    if (answer == 1) {
      Serial.print("Sent successfully \n");
      return true;
    } else {
      Serial.print("error \n");
      return false;
    }
  } else {
    //     Serial.print(answer);
    Serial.print(" error.\n");
    return false;
  }
}
String sendData(String command, const int timeout, boolean debug)
{
  String response = "";
  LTESerial.println(command);

  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (LTESerial.available())
    {
      char c = LTESerial.read();
      response += c;
    }
  }
  if (debug)
  {
    Serial.print(response);
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
uint8_t sendATcommand(const char* ATcommand, const char* expected_answer, unsigned int timeout) {
  uint8_t x = 0, answer = 0;
  char response[100];
  unsigned long previous;
  memset(response, '\0', 100);  // Initialize the string
  delay(100);
  while (LTESerial.available() > 0) LTESerial.read();  // Clean the input buffer
  LTESerial.println(ATcommand);                        // Send the AT command
  x = 0;
  previous = millis();
  // this loop waits for the answer
  do {
    if (LTESerial.available() != 0) {
      // if there are data in the UART input buffer, reads it and checks for the asnwer
      response[x] = LTESerial.read();
      //            Serial.print(response[x]);
      x++;
      // check if the desired answer  is in the response of the module
      if (strstr(response, expected_answer) != NULL) {
        answer = 1;
      }
    }
    // Waits for the asnwer with time out
  } while ((answer == 0) && ((millis() - previous) < timeout));
  //    Serial.print("\n");
  return answer;
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
    if (BTString == "StartPump") {
      MANUAL_START = 1;
      MANUAL_STOP = 0;
      BTSerial.println("OK");
      BTSerial.flush();
    }
    if (BTString == "StopPump") {
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
      char c = BTSerial.read();
      response += c;
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
    response += c;
    newMessage = 1;
  }
  if (debug & newMessage) {
    Serial.println(response);
  }
  return response;
}
