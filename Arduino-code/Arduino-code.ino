/*
 Name:		Arduino_code.ino
 Created:	2/18/2021 12:12:37 PM
 Authors:	Daniela Cornejo y Aarón Vélez
*/

////// General libraries
#include <M5Stack.h>
#include <Wire.h>
#include <WiFi.h> // VisualMicro compiler needs this to find the libraries M5Stack needs



////// Library for Oxygen Sensor
#include <DFRobot_OxygenSensor.h>
#define COLLECT_NUMBER    10             // collect number, the collection range is 1-100.
#define Oxygen_IICAddress ADDRESS_3
/*   iic slave Address, The default is ADDRESS_3.
       ADDRESS_0               0x70      // iic device address.
       ADDRESS_1               0x71
       ADDRESS_2               0x72
       ADDRESS_3               0x73
*/
DFRobot_OxygenSensor Oxygen; // Decalre the class for the Oxygen sensor 



////// Library for SHT31 Temperature and Humidity Sensor
#include <DFRobot_SHT3x.h>
DFRobot_SHT3x   sht3x;


//////////////////////////////////////////////////////////////////////////
// the setup function runs once when you press reset or power the board //
//////////////////////////////////////////////////////////////////////////
void setup() {
    // Initialize M5Stack and setup power
    M5.begin();
    M5.Power.begin();
    M5.Lcd.print("M5 started");

    //Serial.begin(115200); // initalize serial comunication with computer
    //Serial.println(F("Serial communication started"));

    Oxygen.begin(Oxygen_IICAddress);
    //Serial.println(F("Oxygen sensor started"));
    M5.Lcd.print("Oxygen started");

   //SHT31 Temperature and Humidity Sensor
   while (sht3x.begin() != 0) {
    M5.Lcd.println("Failed to Initialize the chip, please confirm the wire connection");
    delay(0);
  }

  M5.Lcd.print("Chip serial number");
  M5.Lcd.println(sht3x.readSerialNumber());
  
   if(!sht3x.softReset()){
     M5.Lcd.print("Failed to Initialize the chip....");
   }
}
 


//////////////////////////////////////////////////////////////////////////
// The loop function runs over and over again until power down or reset //
//////////////////////////////////////////////////////////////////////////
void loop() {
    M5.Lcd.print("Loop start");
    float oxygenData = Oxygen.ReadOxygenData(COLLECT_NUMBER); //DFRobot_OxygenSensor Oxygen code
    //Serial.print(" Oxygen concentration is ");
    //Serial.print(oxygenData);
    //Serial.println(" %vol");
    delay(500);
    M5.Lcd.print((String) "Oxygene: " + oxygenData + " %vol");
    delay(500);
 
 //loop for SHT31 Temperature and Humidity Sensor
  M5.Lcd.print(sht3x.getTemperatureC());
  M5.Lcd.print(" C/ ");
  M5.Lcd.print(sht3x.getHumidityRH());
  M5.Lcd.print(" %RH");
  delay(115200);


  /*
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
  */

}


