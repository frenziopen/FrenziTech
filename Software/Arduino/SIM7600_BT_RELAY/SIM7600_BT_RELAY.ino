/*----------------------------------------------------------------------------
  NAME :            FrenziPump Timer
  COPYRIGHT FrenziMed 2023
  AUTHOR :          Aamir Farooqui
  DATE:             05/11/23
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
#include "heltec.h"
#include "BluetoothSerial.h"              // Header File for Serial Bluetooth, will be added by default into Arduino
#include <rom/ets_sys.h>
#include <ESP32Time.h>

#define VERSION 1.5

#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6
String outgoing;              // outgoing message
//AADIL
//byte localAddress = 0xBB;     // address of this device
//byte destination = 0xFD;      // destination to send to
//ALEENA
byte localAddress = 0xFD;     // address of this device
byte destination = 0xBB;      // destination to send to

// LTE Serial
// 22, 23, 2, 17, 16x, 12x, 13, 32, 33
// 36, 37, 38, 39
#define LTE_TX        12   // LTE transmit
#define LTE_RX        13   // LTE receive
HardwareSerial LTESerial(1);

BluetoothSerial ESP_BT;                   // Object for Bluetooth
char Data;
char BluetoothData  = 0;                  // Initiallize BT value 0
bool ON_OFF = 0;
int  MIN = 0;
int  DELAY_IN_MIN = 30;                   // Initiallize Timer value

#define GPI38      38
#define GPIO23     23
#define GPIO25     25

// ADC INPUTS
#define VI_ADC0_GPI36 36
#define VI_ADC1_GPI37 37
#define VM_ADC2_GPI38 38
#define TM_ADC0_GPI39 39
#define R485_RX2_GPIO16 16
#define R485_TX2_GPIO17 17
#define MOT_RLC1_GPO32 23//new board 32
#define MOT_RLC2_GPO33 25//new board 33
float SsoilMoisture;
float SsoilTemperature;
float SsoilConductivity;

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


// Control
bool MANUAL_START = 0;
bool MANUAL_STOP  = 0;
bool MOTOR_RUN    = 0;


#define DEBUG true
#define MODE_1A
String from_usb = "";
String message = "";
/**********************MAIN****************************/
/***********************FTP upload and download***************************/
char ftp_user_name[] = "frenzitech@frenzivendi.com";
char ftp_user_password[] = "Ttest1234";
char ftp_server[] = "ftp.frenzivendi.com";
char download_file_name[] = "index.htm";
char upload_file_name[] = "index.htm";
int tenMinDelay = 0;


char phone_number[] = "14085040698";      //********** change it to the phone number you want to call
char text_message[] = "AADIL IS KUKU";      //

/*
   https://www.circuitschools.com/
   Interfacing Soil NPK Sensor with Arduino for measuring
   Nitrogen, Phosphorus, and Potassium nutrients
*/

// RO to pin RX & DI to pin TX
// Address | Function_Code | Start Address (Hi) | Start Address (Lo) | Number of Points (Hi) | Number of Points (Lo) | Error Check (Lo) | Error Check (Hi)
// 0x01    | 0x03          | 0x00               | 0x00               | 0x00                  | 0x03
const byte nitro[]  = {0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x05, 0xcb}; // TX:01 03 00 00 00 01 84 0a
byte values[11];
float soilMoisture;
float soilTemperature;
float soilConductivity;



uint8_t sendATcommand(const char* ATcommand, const char* expected_answer, unsigned int timeout) {

  uint8_t x = 0,  answer = 0;
  char response[100];
  unsigned long previous;

  memset(response, '\0', 100);    // Initialize the string

  delay(100);

  while ( LTESerial.available() > 0) LTESerial.read();   // Clean the input buffer

  LTESerial.println(ATcommand);    // Send the AT command


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
      if (strstr(response, expected_answer) != NULL)
      {
        answer = 1;
      }
    }
    // Waits for the asnwer with time out
  } while ((answer == 0) && ((millis() - previous) < timeout));

  //    Serial.print("\n");

  return answer;
}



/**************************Power on Sim7x00**************************/
void PowerOn() {
  uint8_t answer = 0;


  // checks if the module is started
  answer = sendATcommand("AT", "OK", 2000);
  if (answer == 0)
  {
    // waits for an answer from the module
    while (answer == 0) {     // Send AT every two seconds and wait for the answer
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
//SMS sending short message
bool SendingShortMessage(const char* PhoneNumber, const char* Message) {
  uint8_t answer = 0;
  char aux_string[30];

  Serial.print("Setting SMS mode...\n");
  sendATcommand("AT+CMGF=1", "OK", 1000);    // sets the SMS mode to text
  Serial.print("Sending Short Message\n");

  sprintf(aux_string, "AT+CMGS=\"%s\"", PhoneNumber);

  answer = sendATcommand(aux_string, ">", 3000);    // send the SMS number
  if (answer == 1)
  {
    LTESerial.println(Message);
    LTESerial.write(0x1A);
    answer = sendATcommand("", "OK", 20000);
    if (answer == 1)
    {
      Serial.print("Sent successfully \n");
      return true;
    }
    else
    {
      Serial.print("error \n");
      return false;
    }
  }
  else
  {
    //     Serial.print(answer);
    Serial.print(" error.\n");
    return false;
  }
}

/**************************FTP download file to Module EFS , uploading EFS file to FTP**************************/
void ConfigureFTP(const char* FTPServer, const char* FTPUserName, const char* FTPPassWord) {
  char aux_str[50];

  // sets the paremeters for the FTP server
  sendATcommand("AT+CFTPPORT=21", "OK", 2000);
  sendATcommand("AT+CFTPMODE=1", "OK", 2000);
  sendATcommand("AT+CFTPTYPE=A", "OK", 2000);

  //  Serial.print(aux_str,"AT+CFTPSERV=\"%s\"", FTPServer);

  sprintf(aux_str, "AT+CFTPSERV=\"%s\"", FTPServer);
  sendATcommand(aux_str, "OK", 2000);

  sprintf(aux_str, "AT+CFTPUN=\"%s\"", FTPUserName);
  sendATcommand(aux_str, "OK", 2000);
  sprintf(aux_str, "AT+CFTPPW=\"%s\"", FTPPassWord);
  sendATcommand(aux_str, "OK", 2000);
}

void UploadToFTP(const char* FileName) {
  char aux_str[50];

  Serial.print("Upload file to FTP...\n");
  sprintf(aux_str, "AT+CFTPPUTFILE=\"%s\",0", FileName);
  sendATcommand(aux_str, "OK", 2000);
}

void DownloadFromFTP(const char* FileName) {
  char aux_str[50];

  Serial.print("Download file from FTP...\n");
  sprintf(aux_str, "AT+CFTPGETFILE=\"%s\",0", FileName);
  sendATcommand(aux_str, "OK", 2000);
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
ESP32Time rtc;
void setup() {


  // put your setup code here, to run once:
  Serial.begin(115200);
//  LTESerial.begin(115200, SERIAL_8N1, LTE_RX, LTE_TX);
  ESP_BT.begin("FrenziTech BT");   // Name of your Bluetooth Signal
  Serial.println("Bluetooth Device is Ready to Pair");
  /*
  Serial2.begin(4800);
  Serial.println("Press 1 ");
  rtc.setTime(30, 24, 15, 17, 1, 2021);  // 17th Jan 2021 15:24:30

  delay(600);
  Serial.println(" Power ON ");
  pinMode(GPI38,  INPUT);
  pinMode(GPIO25, OUTPUT);
  // power on pulse
  digitalWrite(GPIO25, HIGH);
  delay(500);
  digitalWrite(GPIO25, LOW);
  PowerOn();
  Serial.println(" Msg. ON ");
  // power on pulse
  digitalWrite(GPIO25, HIGH);
  delay(500);
  digitalWrite(GPIO25, LOW);
  sendData("AT+COPS=2\r\n", 3000, DEBUG);
  sendData("AT+CTZU=1\r\n", 3000, DEBUG);
  sendData("AT+COPS=0\r\n", 3000, DEBUG);
  sendData("AT+CCLK?\r\n", 3000, DEBUG);
  //ConfigureFTP(ftp_server,ftp_user_name,ftp_user_password);
  delay(500);
*/
  ///Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, false /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  digitalWrite(GPIO25, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);                       // wait for a second
  digitalWrite(GPIO25, LOW);    // turn the LED off by making the voltage LOW
  delay(100);
  Serial.println(" Lora ON ");


}

void loop() {
  Serial.println("Press 1 ");
  while (ESP_BT.available())                // Check if we receive anything from Bluetooth
  {
    Data = ESP_BT.read();                   // Read "Char"
    int BluetoothData = Data - '0';         // Convert char into integer
    Serial.println("======================================================");
    Serial.print("Received:");
    Serial.println(BluetoothData);
    Serial.println("======================================================");
    delay(500);
    if (BluetoothData == 0)                 // if data recieve is 0, turn off motor
    {
      ON_OFF = 0;
      ESP_BT.println("LED turned OFF");

    }
    if (BluetoothData == 1)                 // if data recieve is 1,LED turn on
    {
      ON_OFF = 1;
      ESP_BT.println("LED turned ON");
    }
    if (BluetoothData == 2)                 // 25% Duty Cycle, change intensity of led
    {
      ESP_BT.println("LED 25% Duty cycle");
    }

  }

  if (MANUAL_STOP) {
    Serial.print("MANUAL_STOP pressed: ");
    Serial.println(MANUAL_STOP);
    digitalWrite(MOT_RLC1_GPO32, LOW);
    digitalWrite(MOT_RLC2_GPO33, HIGH);
    MANUAL_STOP = 0;
    MOTOR_RUN = 0;
    delay (1000);
  }
  if (MANUAL_START) {
    Serial.print("MANUAL_START pressed: ");
    Serial.println(MANUAL_START);
    digitalWrite(MOT_RLC2_GPO33, LOW);
    digitalWrite(MOT_RLC1_GPO32, HIGH);
    MANUAL_START = 0;
    MOTOR_RUN = 1;
    delay (1000);
  }
  digitalWrite(MOT_RLC1_GPO32, HIGH);
  digitalWrite(MOT_RLC2_GPO33, HIGH);

 // rms_current();

  /*
    esp_sleep_enable_timer_wakeup(60000000);
    esp_bluedroid_disable();
    esp_bt_controller_disable();
    esp_wifi_stop();
    esp_light_sleep_start();

  */
/*
  tenMinDelay = tenMinDelay + 1;

  if (  tenMinDelay > 10) {
    sendData("AT+CCLK?\r\n", 3000, DEBUG);
    tenMinDelay = 0;
    // put your main code here, to run repeatedly:
    nitrogen();
    // power on pulse
    digitalWrite(GPIO25, HIGH);
    delay(500);
    digitalWrite(GPIO25, LOW);
    String http_str = "AT+HTTPPARA=\"URL\",\"https://frenzitech.frenzivendi.com/sensor_service.php?soil_temp=" + String(soilTemperature / 10, 1) + \
                      "&soil_moisture=" + String(soilMoisture / 10, 1) + "&soil_ece=" + String(soilConductivity / 10, 1) + "\"\r\n";

    //Serial.println(http_str);
    sendData("AT+HTTPINIT\r\n", 2000, DEBUG);
    sendData(http_str, 2000, DEBUG);
    sendData("AT+HTTPACTION=0\r\n", 3000, DEBUG);
    sendData("AT+HTTPTERM\r\n", 3000, DEBUG);
    sendData("AT+CCLK?\r\n", 3000, DEBUG);
  }
  */
  /*
    Serial.println("Press 1 ");
    int aVin = analogRead(GPI38);
    float dVin = floatMap(aVin, 0, 4095, 0, 3.3);// + random(900);
    char sendsms = 0;
    while (Serial.available())
    {
    sendsms = Serial.read();
    Serial.println(sendsms);
    }

    if (sendsms != 0)
    {
    Serial.println(" sendsms ");
    //       sendData("AT+HTTPTERM\r\n", 3000, DEBUG);
    //   SendingShortMessage(phone_number, text_message);
    //       delay(5000);

    //  Serial.println("Uploading file to ");//\"%s\"...\n", ftp_server);
    //  UploadToFTP(upload_file_name);
    //  Serial.println(ftp_server);
    //-----------HTTP---------------------
    //String http_str = "AT+HTTPPARA=\"URL\",\"https://frenzitech.frenzivendi.com/sensor_service.php?sensor_temp=" + String(dVin) + "\"\r\n";
    //    String temperature = ("T = " + String(soilTemperature/10, 1) + " C");
    //  String humidity = ("H = " + String(soilMoisture/10, 1) + " %");
    //  String conductivity = ("C = " + String(soilConductivity/10, 1) + " u");

    String http_str = "AT+HTTPPARA=\"URL\",\"https://frenzitech.frenzivendi.com/sensor_service.php?soil_temp=" + String(soilTemperature / 10, 1) + \
                      "&soil_moisture=" + String(soilMoisture / 10, 1) + "&soil_ece=" + String(soilConductivity / 10, 1) + "\"\r\n";

    Serial.println(http_str);

    sendData("AT+HTTPINIT\r\n", 2000, DEBUG);
    sendData(http_str, 2000, DEBUG);
    sendData("AT+HTTPACTION=0\r\n", 3000, DEBUG);
    sendData("AT+HTTPTERM\r\n", 3000, DEBUG);
    sendData("AT+CCLK?\r\n", 3000, DEBUG);


    }
    delay(500);*/
}

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
byte nitrogen() {
  // clear the receive buffer
  Serial2.flush();

  // write out the message
  for (uint8_t i = 0; i < sizeof(nitro); i++ ) Serial2.write( nitro[i] );

  // delay to allow response bytes to be received!
  delay(120);

  // read in the received bytes
  for (byte i = 0; i < 11; i++) {
    values[i] = Serial2.read();
    // Serial.print(values[i], HEX);
    //  Serial.print(' ');
  }
  SsoilMoisture      = ((values[3] << 8) | values[4]);
  SsoilTemperature   = ((values[5] << 8) | values[6]);
  SsoilConductivity  = ((values[7] << 8) | values[8]);
  Serial.println(SsoilMoisture / 10);
  Serial.println(SsoilTemperature / 10);
  Serial.println(SsoilConductivity / 10);
  return values[6];
}






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




