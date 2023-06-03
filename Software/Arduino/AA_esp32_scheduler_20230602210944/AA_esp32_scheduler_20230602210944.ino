#include <ESP32Time.h>

ESP32Time rtc;

int start_hr = 0;
int start_min = 0;
int run_time = 0;

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

void loop() {
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

  Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));

  struct tm timeinfo = rtc.getTimeStruct();

  if(rtc.getHour(true) == 15) {
    Serial.print("Motor RUNNING: ");
    Serial.println(String(60 - rtc.getMinute()) + " minutes remaining");
    delay(30000); 
  }

  else{
    delay(1000);
  }
}