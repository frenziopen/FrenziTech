/*
 * HelTec Automation(TM) CubeCell software serial example
 *
 * Function summary:
 * 
 * - software read data from a UART device and print received data via normal serial prot
 * - baudrate defined in softwareSerial.begin(9600);
 * |-- Supported baudrate
 * |-- 57600
 * |-- 38400
 * |-- 19200
 * |-- 14400
 * |-- 9600
 * |-- 4800
 * |-- 2400
 * |-- 1200
 *
 * HelTec AutoMation, Chengdu, China.
 * 成都惠利特自动化科技有限公司
 * https://heltec.org
 * support@heltec.cn
 *
 * this project also release in GitHub:
 * 
 */

#include "softSerial.h"
// #include <stdlib.h>

#define TXD-485        GPIO5   // RS-485 transmit
#define RXD-485        GPIO6   // RS-485 receive
#define SLR_5V         GPIO8   // SOLENOID 5V CONTROL
#define SENSE_5V       GPIO9   // SENSOR 5V CONTROL
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


softSerial RS485Serial(GPIO5 /*TXD-485 TX pin*/, GPIO6/*RXD-485 RX pin*/);

void setup()
{
  pinMode(SLR_5V, OUTPUT);
  pinMode(SENSE_5V, OUTPUT);
  digitalWrite(SLR_5V, HIGH);
  digitalWrite(SENSE_5V, HIGH);
  delay(100);
	Serial.begin(115200);
	Serial.println("Normal serial init");
	//software serial init
	RS485Serial.begin(4800);
    delay(100);

}

void loop()
{
  nitrogen();
	delay(1000);
}

byte nitrogen() {
  // clear the receive buffer
  RS485Serial.flush();

  // write out the message
  for (uint8_t i = 0; i < sizeof(nitro); i++ ) RS485Serial.write( nitro[i] );

  // delay to allow response bytes to be received!
  delay(1200);

  // read in the received bytes
  for (byte i = 0; i < 11; i++) {
    values[i] = RS485Serial.read();
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

