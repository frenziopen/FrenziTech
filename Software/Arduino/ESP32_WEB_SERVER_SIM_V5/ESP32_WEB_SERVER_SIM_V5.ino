#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <HardwareSerial.h>
#include "time.h"
#include "Arduino.h"
#include "heltec.h"
#include <esp_sleep.h>
#include <soc/rtc.h>


#define VERSION 1.5
#define LTE_RX_GPIO16 0
#define LTE_TX_GPIO17 22
#define R485_RX2_GPIO16 16
#define R485_TX2_GPIO17 17
#define MOT_RLC1_GPO32 23//new board 32
#define MOT_RLC2_GPO33 25//new board 33

#define GPI38      38
#define GPIO23     23  //LED
#define GPIO25     25  //LED

#define SC1_GPO13 13
#define SLC2_GPO23 23
#define MOT_VM5_GPO2 2
#define LTE_VCC_GPO25 25
// ADC INPUTS
#define VI_ADC0_GPI36 36
#define VI_ADC1_GPI37 37
#define VM_ADC2_GPI38 38
#define TM_ADC0_GPI39 39

// 22, 23, 2, 17, 16x, 12x, 13, 32, 33
// 36, 37, 38, 39
#define BT_TX        12   // BT transmit
#define BT_RX        13   // BT receive


// RO to pin RX & DI to pin TX
// Address | Function_Code | Start Address (Hi) | Start Address (Lo) | Number of Points (Hi) | Number of Points (Lo) | Error Check (Lo) | Error Check (Hi)
// 0x01    | 0x03          | 0x00               | 0x00               | 0x00                  | 0x03
const byte nitro[]  = {0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x05, 0xcb}; // TX:01 03 00 00 00 01 84 0a
byte values[11];
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



uint32_t secs = 0;



#define BLINK_DEL_100 1500


/* Put your SSID & Password */
const char* ssid = "ESP32";     // Enter SSID here
const char* password = "1234";  //Enter Password here

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// These are the global variables
String motorStartTimeHr = "10";
String motorStartTimeMin = "10";
String motorRunTimeHr = "10";
String motorRunTimeMin = "10";
String motorRunInterval = "10";
String motorVoltage = "10";
String motorCurrent = "10";
String motorEffeciency = "10";
String soilMoisturePercent = "10";
String soilTemperature = "10";
String airTemperature = "10";
String currentTimeHr = "10";
String currentTimeMin = "10";
String programButton = "0";
String manualStartButton = "0";
String manualStopButton = "0";


// Control
bool MANUAL_START = 0;
bool MANUAL_STOP  = 0;
bool MOTOR_RUN    = 0;


time_t now;
char strftime_buf[64];
struct tm timeinfo;
HardwareSerial LTESerial(1);
#define DEBUG true
#define MODE_1A
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
/*
  <label for="motorStartTimeHr">Motor Start Time (H:M):  </label><input type="text" id="motorStartTimeHr" name="motorStartTimeHr"> <label for="motorStartTimeMin"> : </label><input type="text" id="motorStartTimeMin" name="motorStartTimeMin"> <br>
  <label for="motorRunTimeHr">Motor Run Time (H:M):     </label><input type="text" id="motorRunTimeHr" name="motorRunTimeHr"> <label for="motorRunTimeMin"> : </label><input type="text" id="motorRunTimeMin" name="motorRunTimeMin"> <br>
  <label for="motorRunInterval">Frequency (interval days) 1, 2, ... 8, 10: </label><input type="text" id="motorRunInterval" name="motorRunInterval"><br>
  <label for="motorVoltage">Motor voltage (Volts) 110, 220, 440: </label>
  <input type="radio" id="110" name="motorVoltage" value="100"> <label for="110">110V</label>
  <input type="radio" id="220" name="motorVoltage" value="220"> <label for="220">220V</label>
  <input type="radio" id="440" name="motorVoltage" value="440"> <label for="440">440V</label><br>
  <label for="motorCurrent">Motor current (Amps) 1, 2, 3, ... 100: </label><input type="text" id="motorCurrent" name="motorCurrent"><br>
  <label for="soilMoisturePercent">Soil moisture (%) 0-99: </label><input type="text" id="soilMoisturePercent" name="soilMoisturePercent"><br>
  <label for="soilTemperature">Soil temperature (F) 0-99: </label><input type="text" id="soilTemperature" name="soilTemperature"><br>

  const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
  <label for="airTemperature">Air temperature (F) 0-99: </label><input type="text" id="airTemperature" name="airTemperature"><br>
  <label for="motorCurrent">Motor current (Amps) 1, 2, 3, ... 100: </label><input type="text" id="motorCurrent" name="motorCurrent" min="1" max="100" value=@@L1@@><br>

  <input type="submit" value="Submit">
  </form>
  </body></html>)rawliteral";
*/

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  
  </head><body>
<style>
    input[type=number] {
      padding: 5px 10px;
      border-radius: 4px;
    }
    table {
      padding: 10px;
    }
    th, td {
            text-align: left;
            padding: 8px;
            font-family: Verdana,sans-serif;
    }
          
    tr:nth-child(even) {
            background-color: PowderBlue;
    }
    tr:nth-child(odd) {
            background-color: Whitesmoke;
    }
    input[type=button], input[type=submit], input[type=reset] {
       background-color: #04AA6D;
       border: none;
       color: white;
       padding: 16px 32px;
       text-decoration: none;
       margin: 4px 2px;
       cursor: pointer;
       width: 40%;
    }
    
</style>
<br><br>
<form action="/get">
<table border="0" cellpadding="10" align="center" >
<tr>
    <td>Motor Start Time (H:M): </td>
    <td><input type="number" id="motorStartTimeHr" name="motorStartTimeHr" min="0" max="23" value=@@MSTH@@> : <input type="number" id="motorStartTimeMin" name="motorStartTimeMin" min="0" max="59" value=@@MSTM@@> </td>
</tr>
<tr>
    <td>Motor Run Time (H:M): </td>
    <td><input type="number" id="motorRunTimeHr" name="motorRunTimeHr" min="1" max="999" value=@@MRTH@@> : <input type="number" id="motorRunTimeMin" name="motorRunTimeMin" min="0" max="59" value=@@MRTM@@> </td>
</tr>
<tr>
    <td>Frequency (interval days) 1, 2, ... 8, 10: </td>
    <td><input type="number" id="motorRunInterval" name="motorRunInterval" min="1" max="100" value=@@MRI@@></td>
</tr>
<tr>
    <td>Motor voltage (Volts) 110, 220, 440:</td>
    <td> <input type="radio" id="motorVoltage" name="motorVoltage" value="110" @@M110checked@@> 110V
         <input type="radio" id="motorVoltage" name="motorVoltage" value="220" @@M220checked@@> 220V
         <input type="radio" id="motorVoltage" name="motorVoltage" value="440" @@M440checked@@> 440V </td>
</tr>
<tr>
    <td>Motor current (Amps) 1, 2, 3, ... 100: </td>
    <td><input type="number" id="motorCurrent" name="motorCurrent" min="1" max="100" value=@@MC@@></td>
</tr>
<tr>
    <td>Motor effeciecy (%) 0-100: </td>
    <td><input type="number" id="motorEffeciency" name="motorEffeciency" min="1" max="100" value=@@ME@@></td>
</tr>
<tr>
    <td>Soil moisture (%) 0-100: </td>
    <td><input type="number" id="soilMoisturePercent" name="soilMoisturePercent" min="0" max="100" value=@@SMP@@></td> 
</tr>
<tr>
    <td>Soil temperature (&#8457) 0-100: </td>
    <td><input type="number" id="soilTemperature" name="soilTemperature" min="0" max="100" value=@@ST@@></td>
</tr>
<tr>
    <td>Air temperature (&#8457) 0-100: </td>
    <td><input type="number" id="airTemperature" name="airTemperature" min="0" max="100" value=@@AT@@></td>
</tr>
<tr>
    <td>Current Time (H:M): </td>
    <td><input type="number" id="currentTimeHr" name="currentTimeHr" min="0" max="23" value=@@CTH@@> : <input type="number" id="currentTimeMin" name="currentTimeMin" min="0" max="59" value=@@CTM@@> </td>
</tr>

<tr>
    <td colspan="2" align="center"><input type="submit" id="program" name="program" value="Program">&nbsp;&nbsp;<input type="reset" id="cancel" name="cancel" value="Cancel"></td>
</tr>
<tr>
    <td colspan="2" align="center"><input type="submit" id="manualStart" name="manualStart" value="Manual Start">&nbsp;&nbsp;<input type="submit" id="manualStop" name="manualStop" value="Stop" style="background-color:red;"></td>
</tr>
</table>
</form>
</body></html>)rawliteral";


//=======================================================================
//                    handles main page 192.168.1.1
//=======================================================================
/* Just a little test message.  Go to http://192.168.1.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  String s = index_html;
  s.replace("@@MSTH@@", motorStartTimeHr);
  s.replace("@@MSTM@@", motorStartTimeMin);
  s.replace("@@MRTH@@", motorRunTimeHr);
  s.replace("@@MRTM@@", motorRunTimeMin);
  s.replace("@@MRI@@", motorRunInterval);
  //    s.replace("@@MV@@", motorVoltage);
  if (motorVoltage) {
    if (motorVoltage == "110") {
      s.replace("@@M110checked@@", "checked");
    } else {
      s.replace("@@M110checked@@", "");
    }
    if (motorVoltage == "220") {
      s.replace("@@M220checked@@", "checked");
    } else {
      s.replace("@@M220checked@@", "");
    }
    if (motorVoltage == "440") {
      s.replace("@@M440checked@@", "checked");
    } else {
      s.replace("@@M440checked@@", "");
    }
  } else {
    s.replace("@@M110checked@@", "checked");
  }

  s.replace("@@MC@@", motorCurrent);
  s.replace("@@ME@@", motorEffeciency);
  s.replace("@@SMP@@", soilMoisturePercent);
  s.replace("@@ST@@", soilTemperature);
  s.replace("@@AT@@", airTemperature);
  s.replace("@@CTH@@", currentTimeHr);
  s.replace("@@CTM@@", currentTimeMin);
  s.replace("@@BTPR@@", programButton);
  s.replace("@@BTMSTART@@", manualStartButton);
  s.replace("@@BTMSTOP@@", manualStopButton);

  server.send(200, "text/html", s);
}
//===============================================================
// This routine is executed when you press submit
//===============================================================
void handleForm() {

  motorStartTimeHr = server.arg("motorStartTimeHr");
  motorStartTimeMin = server.arg("motorStartTimeMin");
  motorRunTimeHr = server.arg("motorRunTimeHr");
  motorRunTimeMin = server.arg("motorRunTimeMin");
  motorRunInterval = server.arg("motorRunInterval");
  motorVoltage = server.arg("motorVoltage");
  motorCurrent = server.arg("motorCurrent");
  motorEffeciency = server.arg("motorEffeciency");
  soilMoisturePercent = server.arg("soilMoisturePercent");
  soilTemperature = server.arg("soilTemperature");
  airTemperature = server.arg("airTemperature");
  currentTimeHr = server.arg("currentTimeHr");
  currentTimeMin = server.arg("currentTimeMin");
  if (server.arg("manualStart").length() > 0) {
    Serial.println("Manual start pressed - DETECTED");
    MANUAL_START = 1;
    MANUAL_STOP  = 0;
  }
  else if (server.arg("manualStop").length() > 0) {
    Serial.println("Manual stop pressed - DETECTED");
    MANUAL_STOP  = 1;
    MANUAL_START = 0;
  }

  Serial.print("motorCurrent:");
  Serial.println(motorCurrent);

  Serial.print("airTemperature:");
  Serial.println(airTemperature);

  //String s = "<a href='/'> Go Back </a>";
  //server.send(200, "text/html", s); //Send web page
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Updated Press Back Button");
}


//=======================================================================
//                    setup
//=======================================================================


int sec_timer = 0;
void setup() {
  Serial.begin(9600);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  server.on("/", handleRoot);     //Which routine to handle at root location
  server.on("/get", handleForm);  //form action is handled here
  server.begin();                 //Start server
  Serial.println("HTTP server started");
  delay(600);
  pinMode(VI_ADC0_GPI36, INPUT);
  pinMode(VI_ADC1_GPI37, INPUT);
  pinMode(VM_ADC2_GPI38, INPUT);
  pinMode(TM_ADC0_GPI39, INPUT);
  // put your setup code here, to run once:
  pinMode(MOT_RLC1_GPO32,  OUTPUT);
  pinMode(MOT_RLC2_GPO33,  OUTPUT);
  pinMode(LTE_VCC_GPO25,  OUTPUT);
  // disable relays
  digitalWrite(MOT_RLC1_GPO32, HIGH);
  digitalWrite(MOT_RLC2_GPO33, HIGH);

  int aVin = analogRead(VM_ADC2_GPI38);
  float dVin = floatMap(aVin, 0, 4095, 0, 3.3);

  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, false /*Serial Enable*/);
  Heltec.display -> clear();
  Heltec.display -> setFont(ArialMT_Plain_16); // ArialMT_Plain_10, ArialMT_Plain_16, ArialMT_Plain_24
  Heltec.display -> drawString(0, 10, "BISMILLAH");
  String batteryVin = ("BAT = " + String(dVin, 1) + " V");
  Heltec.display -> drawString(0, 30, String(batteryVin));
  Heltec.display -> display();
  delay(1500);
  Serial.println("Initialize...");
  Serial2.begin(4800);
  sec_timer = millis();
}

void loop() {
  server.handleClient();  //Handle client requests
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

  rms_current();

  if (millis() > sec_timer + 1000)
  {
    sec_timer = millis();
    //    Serial2.begin(4800);
    //    nitrogen();
    //    String temperature  = ("T = " + String(SsoilTemperature / 10, 1) + " C");
    Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, false /*Serial Enable*/);
    Heltec.display -> setFont(ArialMT_Plain_16);
    Heltec.display -> clear();
    String mstatus = MOTOR_RUN ? "Motor ON" : "Motor OFF";
    String rcurrent = "Req Mot I: " + motorCurrent + " A";
    String runcurrent = "Run Mot I: " + String(FinalRMSCurrent, 1) + " A";
    Heltec.display -> drawString(0, 15, String(mstatus));
    Heltec.display -> drawString(0, 30, String(rcurrent));
    Heltec.display -> drawString(0, 45, String(runcurrent));
    Heltec.display -> display();

  }
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
void BlinkLed(int wait) {
  digitalWrite(LED, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(wait);              // wait for a second
  digitalWrite(LED, LOW);   // turn the LED off by making the voltage LOW
  delay(wait);
}

//Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, false /*Serial Enable*/);
/*Heltec.display -> clear();
  Heltec.display -> setFont(ArialMT_Plain_16); // ArialMT_Plain_10, ArialMT_Plain_16, ArialMT_Plain_24
  String temperature  = ("T = " + String(SsoilTemperature / 10, 1) + " C");
  String humidity     = ("H = " + String(SsoilMoisture / 10, 1) + " %");
  String conductivity = ("C = " + String(SsoilConductivity / 10, 1) + " u");
  //}*/

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
