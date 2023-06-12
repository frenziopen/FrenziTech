/*----------------------------------------------------------------------------
  NAME :            FrenziPump Timer
  COPYRIGHT FrenziMed 2023
  AUTHOR :          Aamir Farooqui
  DATE:             05/11/23
  MODIFIED BY :
  COMMENTS:
  VERSION:          V1.0
  -----------------------------------------------------------------------------*/
#include "BluetoothSerial.h"  // Header File for Serial Bluetooth, will be added by default into Arduino
#include <rom/ets_sys.h>
#include <ESP32Time.h>
#include <esp_sleep.h>
#include <soc/rtc.h>
#include "esp32-hal-cpu.h"

#define VERSION 1.5
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 3           /* Time ESP32 will go to sleep (in seconds) */
#define DEBUG true
#define MODE_1A
// LTE Serial
// 22, 23, 2, 17, 16x, 12x, 13, 32, 33
// 36, 37, 38, 39
#define LTE_TX 12  // LTE transmit
#define LTE_RX 13  // LTE receive
HardwareSerial LTESerial(1);
BluetoothSerial ESP_BT;  // Object for Bluetooth
ESP32Time rtc;

String outgoing;  // outgoing message
RTC_DATA_ATTR char Data;
bool ON_OFF = 0;
int MIN = 0;
int DELAY_IN_MIN = 30;  // Initiallize Timer value

// Control
bool MANUAL_START = 0;
bool MANUAL_STOP = 0;
bool MOTOR_RUN = 0;



String from_usb = "";
String message = "";


// ADC INPUTS
#define VI_ADC0_GPI36 36
#define VI_ADC1_GPI37 37
#define VM_ADC2_GPI38 38
#define TM_ADC0_GPI39 39
#define R485_RX2_GPIO16 16
#define R485_TX2_GPIO17 17
#define MOT_RLC1_GPO32 23  //new board 32
#define MOT_RLC2_GPO33 25  //new board 33

RTC_DATA_ATTR bool first_rst = false;
void setup() {
//  if (first_rst == false) {
    //rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
    first_rst = true;
    setCpuFrequencyMhz(RTC_CPU_FREQ_80M);
    delay(100);  // wait for a second
    // put your setup code here, to run once:
    Serial.begin(115200);
   // LTESerial.begin(115200, SERIAL_8N1, LTE_RX, LTE_TX);
    ESP_BT.begin("FrenziTech BT");  // Name of your Bluetooth Signal
    Serial.println("Bluetooth Device is Ready to Pair");
    rtc.setTime(11, 10, 15, 3, 6, 2023);  // 17th Jan 2021 15:24:30
 // }
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.print("first_rst = ");
  Serial.println(first_rst);
  Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));
  Serial.flush();
  struct tm timeinfo = rtc.getTimeStruct();
  esp_light_sleep_start();
}


int start_hr = 0;
int start_min = 0;
int run_time = 0;
/*
void setup() {
  Serial.begin(115200);
  byte buffer[50];  
  Serial.println("---------Set Time---------");
  Serial.print("Current Year: ");
  while(Serial.available() == 0){
    //do nothing
  }
  int yr = Serial.readBytes(buffer, 4) - '0';
  for (byte i = 0; i < 5; i = i + 1) {
    Serial.print(buffer[i]-'0');
  }

  Serial.print("Current Month: ");
  while(Serial.available() == 0){}
  int mt = int(Serial.read());
  Serial.println(mt);

  Serial.print("Current Day: ");
  while(Serial.available() == 0){}
  int dy = int(Serial.read());
  Serial.println(dy);

  Serial.print("Current Hour: ");
  while(Serial.available() == 0){}
  int hr = int(Serial.read());
  Serial.println(hr);

  Serial.print("Current Minute: ");
  while(Serial.available() == 0){}
  int mn = int(Serial.read());
  Serial.println(mn);

  Serial.print("Current Second (approx.): ");
  while(Serial.available() == 0){}
  int sc = int(Serial.read());
  Serial.println(sc);

  Serial.println("---------Thank You---------");

  Serial.println("---------Set Starting Time and Duration---------");
  Serial.print("Input Starting Hour (24-hr format): ");
  while(Serial.available() == 0){}
  int start_hr = int(Serial.read());
  Serial.println(start_hr);

  Serial.print("Input Starting Minute: ");
  while(Serial.available() == 0){}
  int start_min = int(Serial.read());
  Serial.println(start_min);

  Serial.print("Input Run Time (in minutes): ");
  while(Serial.available() == 0){}
  int run_time = int(Serial.read());;
  Serial.println(String(run_time) + " minutes");
  
  Serial.println("---------Thank You---------");

  rtc.setTime(sc, mn, hr, dy, mt, yr);
}
*/
void loop() {
  Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));
  Serial.flush();
  while (ESP_BT.available())  // Check if we receive anything from Bluetooth
  {
    Data = ESP_BT.read();            // Read "Char"
    int BluetoothData = Data - '0';  // Convert char into integer
    Serial.println("======================================================");
    Serial.print("Received:");
    Serial.println(BluetoothData);
    Serial.println("======================================================");
    delay(500);
    if (BluetoothData == 0)  // if data recieve is 0, turn off motor
    {
      ON_OFF = 0;
      ESP_BT.println("LED turned OFF");
    }
    if (BluetoothData == 1)  // if data recieve is 1,LED turn on
    {
      ON_OFF = 1;
      ESP_BT.println("LED turned ON");
    }
    if (BluetoothData == 2)  // 25% Duty Cycle, change intensity of led
    {
      ESP_BT.println("LED 25% Duty cycle");
    }
  }
  if(ON_OFF == 1){
  esp_light_sleep_start();
  }

  //  Serial.println(rtc.getTime());          //  (String) 15:24:38
  //  Serial.println(rtc.getDate());          //  (String) Sun, Jan 17 2021
  //  Serial.println(rtc.getDate(true));      //  (String) Sunday, January 17 2021
  //  Serial.println(rtc.getDateTime());      //  (String) Sun, Jan 17 2021 15:24:38
  //  Serial.println(rtc.getDateTime(true));  //  (String) Sunday, January 17 2021 15:24:38
  //  Serial.println(rtc.getTimeDate());      //  (String) 15:24:38 Sun, Jan 17 2021
  //  Serial.println(rtc.getTimeDate(true));  //  (String) 15:24:38 Sunday, January 17 2021
  //
  //  Serial.println(rtc.getMicros());        //  (long)    723546
  //  Serial.println(rtc.getMillis());        //  (long)    723
  //  Serial.println(rtc.getEpoch());         //  (long)    1609459200
  //  Serial.println(rtc.getSecond());        //  (int)     38    (0-59)
  //  Serial.println(rtc.getMinute());        //  (int)     24    (0-59)
  //  Serial.println(rtc.getHour());          //  (int)     3     (0-12)
  //  Serial.println(rtc.getHour(true));      //  (int)     15    (0-23)
  //  Serial.println(rtc.getAmPm());          //  (String)  pm
  //  Serial.println(rtc.getAmPm(true));      //  (String)  PM
  //  Serial.println(rtc.getDay());           //  (int)     17    (1-31)
  //  Serial.println(rtc.getDayofWeek());     //  (int)     0     (0-6)
  //  Serial.println(rtc.getDayofYear());     //  (int)     16    (0-365)
  //  Serial.println(rtc.getMonth());         //  (int)     0     (0-11)
  //  Serial.println(rtc.getYear());          //  (int)     2021

  /*
  Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));

  struct tm timeinfo = rtc.getTimeStruct();
  esp_light_sleep_start();

  if(rtc.getHour(true) == 15) {
    Serial.print("Motor RUNNING: ");
    Serial.println(String(60 - rtc.getMinute()) + " minutes remaining");
    delay(30000); 
  }

  else{
    delay(1000);
  }*/
}