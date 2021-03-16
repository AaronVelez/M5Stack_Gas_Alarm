/*
 Name:		Arduino_code.ino
 Created:	2/18/2021 12:12:37 PM
 Authors:	Daniela Cornejo y Aarón Vélez
*/

////// Library for M5Stack
#include <M5Stack.h>



////// Library for Oxygen Sensor
#include "DFRobot_OxygenSensor.h
#define COLLECT_NUMBER    10             // collect number, the collection range is 1-100.
#define Oxygen_IICAddress ADDRESS_3
/*   iic slave Address, The default is ADDRESS_3.
       ADDRESS_0               0x70      // iic device address.
       ADDRESS_1               0x71
       ADDRESS_2               0x72
       ADDRESS_3               0x73
*/



/////// Library for CO2 Sensor
#include "DFRobot_BME280.h"
#include "Wire.h"
#define SEA_LEVEL_PRESSURE    1015.0f
int sensorIn = A0;



////// Library for Environmental Sensor Temperature, Humidity, Barometer
typedef DFRobot_BME280_IIC    BME; 
BME   bme(&Wire, 0x77);   // select TwoWire peripheral and set sensor address

#define SEA_LEVEL_PRESSURE    1015.0f

// show last sensor operate status
void printLastOperateStatus(BME::eStatus_t eStatus)
{
  switch(eStatus) {
  case BME::eStatusOK:    Serial.println("everything ok"); break;
  case BME::eStatusErr:   Serial.println("unknow error"); break;
  case BME::eStatusErrDeviceNotDetected:    Serial.println("device not detected"); break;
  case BME::eStatusErrParameter:    Serial.println("parameter error"); break;
  default: Serial.println("unknow status"); break;
  }
}



//////////////////////////////////////////////////////////////////////////
// the setup function runs once when you press reset or power the board //
//////////////////////////////////////////////////////////////////////////
void setup() {
Serial.begin(9600); //DFRobot_OxygenSensor Oxygen code
  while(!Oxygen.begin(Oxygen_IICAddress)) {
    Serial.println("I2c device number error !");
    delay(1000);
  }
  Serial.println("I2c connect success !");
}
{
 Serial.begin(9600); //CO2 Sensor code 
  // Set the default voltage of the reference voltage
  analogReference(DEFAULT);
}
 
{
  Serial.begin(115200); //Environmental Sensor Temperature, Humidity, Barometer
  bme.reset();
  Serial.println("bme read data test");
  while(bme.begin() != BME::eStatusOK) {
    Serial.println("bme begin faild");
    printLastOperateStatus(bme.lastOperateStatus);
    delay(2000);
  }
  Serial.println("bme begin success");
  delay(100);
}



//////////////////////////////////////////////////////////////////////////
// The loop function runs over and over again until power down or reset //
//////////////////////////////////////////////////////////////////////////
void loop() {
  float oxygenData = Oxygen.ReadOxygenData(COLLECT_NUMBER); //DFRobot_OxygenSensor Oxygen code
  Serial.print(" Oxygen concentration is ");
  Serial.print(oxygenData);
  Serial.println(" %vol");
  delay(1000);
}

{
 //Read voltage for the CO2 sensor
  int sensorValue = analogRead(sensorIn);

  // The analog signal is converted to a voltage
  float voltage = sensorValue*(5000/1024.0);
  if(voltage == 0)
  {
    Serial.println("Fault");
  }
  else if(voltage < 400)
  {
    Serial.println("preheating");
  }
  else
  {
    int voltage_diference=voltage-400;
    float concentration=voltage_diference*50.0/16.0;
    // Print Voltage
    Serial.print("voltage:");
    Serial.print(voltage);
    Serial.println("mv");
    //Print CO2 concentration
    Serial.print(concentration);
    Serial.println("ppm");
  }
  delay(100); // This will show the ppm on the air of CO2
}

{
  float   temp = bme.getTemperature(); //This is for Environmental Sensor Temperature, Humidity, Barometer
  uint32_t    press = bme.getPressure();
  float   alti = bme.calAltitude(SEA_LEVEL_PRESSURE, press);
  float   humi = bme.getHumidity();

  Serial.println();
  Serial.println("======== start print ========");
  Serial.print("temperature (unit Celsius): "); Serial.println(temp);
  Serial.print("pressure (unit pa):         "); Serial.println(press);
  Serial.print("altitude (unit meter):      "); Serial.println(alti);
  Serial.print("humidity (unit percent):    "); Serial.println(humi);
  Serial.println("========  end print  ========");

  delay(1000);
}
}
