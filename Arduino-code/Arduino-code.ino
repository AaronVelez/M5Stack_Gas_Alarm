/*
 Name:		Arduino_code.ino
 Created:	2/18/2021 12:12:37 PM
 Authors:	Daniela Cornejo y Aarón Vélez
*/


//////////////////////////////////////////////////////////////////
////// Libraries and its associated constants and variables //////
//////////////////////////////////////////////////////////////////

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
//#include <DFRobot_SHT3x.h>



//////////////////////////////////////////
////// User Constants and Variables //////
//////////////////////////////////////////

////// Time variables
unsigned long time_ms = 0;
time_t unix_t;	// UNIX time stamp
int s = -1;		//Seconds
int m = -1;		//Minutes
int h = -1;		//Hours
int dy = -1;	//Day
int mo = -1;	//Month
int yr = -1;	//Year

////// State machine Shift Registers
int LastSum = -1;			// Last minute that variables were added to the sum for later averaging
int SumNum = 0;				// Number of times a variable value has beed added to the sum for later averaging
int LastLog = -1;			// Last minute that variables were loged to the SD card
bool PayloadRdy = false;	// Payload ready to send to LoRa

////// Measured instantaneous variables
float O2Value = -1;		// Oxygene value read each second
float CO2Value = -1;    // Carbon dioxide value read each second
float Temp = -1;        // Air temperature read each minute
float RH = -1;          // Air RH value read each minute



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

}
 


//////////////////////////////////////////////////////////////////////////
// The loop function runs over and over again until power down or reset //
//////////////////////////////////////////////////////////////////////////
void loop() {
    M5.Lcd.print("Loop start");

    ////// State 2. Get current time
    time_ms = millis();
    // real time still needs to be added



    ////// State 3. Test if it is time to read gas sensor values (each second)
    if (true) { // logical test still need to be added
        O2Value = Oxygen.ReadOxygenData(COLLECT_NUMBER); //DFRobot_OxygenSensor Oxygen code
        M5.Lcd.print((String)"Oxygene: " + O2Value + " %vol");
        ////// Test if values are within safe limits
        if (true) { // logical test still need to be added
            // Set alarm global variable to TRUE
        }
    }

    
    ////// State 4. Test if alarm must be triggered
    if (true) { // logical test still need to be added
        // Triger alarm (sound, emails, flashes, etc)
    }
    

    ////// State 5. Test if it is time to read Temp and RH values AND record sensor values for 5-minute averages (each minute)
    if (true) { // logical test still need to be added
        // Read Temp and RH sensor values
        // add sensor value to sum variable
    }


    ////// State 6. Test if it is time to record sensor values for 5-minute averages (each minute)
    if (true) { // logical test still need to be added
        // calculate averages
        // record averages to sd card
    }


    ////// State 7. Test if there is data available to be sent to cloud
    if (true) { // logical test still need to be added
        // add code here
    }


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


