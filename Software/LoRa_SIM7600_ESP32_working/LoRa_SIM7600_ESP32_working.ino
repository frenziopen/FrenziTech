#include "heltec.h"

#include <stdio.h>
#include <string.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_sleep.h>


#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6


String outgoing;              // outgoing message
//AADIL
//byte localAddress = 0xBB;     // address of this device
//byte destination = 0xFD;      // destination to send to
//ALEENA
byte localAddress = 0xFD;     // address of this device
byte destination = 0xBB;      // destination to send to


//#include <soc/rtc.h>
//void setup() {
//    rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
//}
// 22, 23, 2, 17, 16x, 12x, 13, 32, 33
// 36, 37, 38, 39
#define BT_TX        12   // BT transmit
#define BT_RX        13   // BT receive


#define GPI38      38
#define GPIO23     23
#define GPIO25     25

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


#define DEBUG true
#define MODE_1A
HardwareSerial LTESerial(1);

String from_usb = "";
String message = "";

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

#define GPIO25     25
#define GPI38      38

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
/**********************MAIN****************************/
/***********************FTP upload and download***************************/
char ftp_user_name[] = "frenzitech@frenzivendi.com";
char ftp_user_password[] = "Ttest1234";
char ftp_server[] = "ftp.frenzivendi.com";
char download_file_name[] = "index.htm";
char upload_file_name[] = "index.htm";

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
char phone_number[] = "14085040698";      //********** change it to the phone number you want to call
char text_message[] = "AADIL IS KUKU";      //

void setup() {
  Serial.println("Press 1 ");

  // put your setup code here, to run once:
  Serial.begin(115200);
  LTESerial.begin(115200, SERIAL_8N1, BT_RX, BT_TX);
  Serial2.begin(4800);

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

  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, false /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  digitalWrite(GPIO25, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);                       // wait for a second
  digitalWrite(GPIO25, LOW);    // turn the LED off by making the voltage LOW
  delay(100);  
  Serial.println(" Lora ON ");


}
int tenMinDelay = 0;

void loop() {
  Serial.println("Press 1 ");
/*
  esp_sleep_enable_timer_wakeup(60000000);
  esp_bluedroid_disable();
  esp_bt_controller_disable();
  esp_wifi_stop();
  esp_light_sleep_start();

  */

  
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

byte nitrogen() {
  // clear the receive buffer
  Serial2.flush();

  // write out the message
  for (uint8_t i = 0; i < sizeof(nitro); i++ ) Serial2.write( nitro[i] );

  // delay to allow response bytes to be received!
  delay(1200);

  // read in the received bytes
  for (byte i = 0; i < 11; i++) {
    values[i] = Serial2.read();
    // Serial.print(values[i], HEX);
    //  Serial.print(' ');
  }
  soilMoisture      = ((values[3] << 8) | values[4]);
  soilTemperature   = ((values[5] << 8) | values[6]);
  soilConductivity  = ((values[7] << 8) | values[8]);
  Serial.println(soilMoisture / 10);
  Serial.println(soilTemperature / 10);
  Serial.println(soilConductivity / 10);
  return values[6];
}
