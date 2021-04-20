/*
 Name:		Arduino_code.ino
 Created:	2/18/2021 12:12:37 PM
 Authors:	Daniela Cornejo y Aarón Vélez
*/

time_t t_DataBucket = 0;             // Last time Data was sent to bucket (in UNIX time format) 
const int DataBucket_frq = 150;       // Data bucket update frequency in seconds


//////////////////////////////////////////////////////////////////
////// Libraries and its associated constants and variables //////
//////////////////////////////////////////////////////////////////

////// Board library
#include <M5Stack.h>


////// Credentials is a user library containing paswords, IDs and credentials
#include "credentials.h"
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWD;
const char iot_user[] = IoT_USER;
const char iot_device[] = IoT_DEVICE;
const char iot_credential[] = IoT_CREDENTIAL;


////// Comunication libraries
#include <Wire.h>
#include <WiFi.h>
const char* ssid = "SSID";
const char* password = "PASSWORD";
#include <WiFiUdp.h>
WiFiUDP ntpUDP;

#define _DEBUG_
#define _DISABLE_TLS_
#include <ThingerESP32.h>
ThingerESP32 thing(iot_user, iot_device, iot_credential);




////// SD fat library used in order to use exFAT file system
////// Adapted to Mg stack acording to https://github.com/ArminPP/sdFAT-M5Stack
#include <SdFat.h>
#define SPI_SPEED SD_SCK_MHZ(12)
#define SD_CONFIG SdSpiConfig(TFCARD_CS_PIN, SHARED_SPI, SPI_SPEED) // TFCARD_CS_PIN is defined in M5Stack Config.h (Pin 4)
/// <summary>
/// IMPORTANT
/// in file SdFat\src\SdFatConfig.h at Line 100 set to cero for NON dedicated SPI:
/// #define ENABLE_DEDICATED_SPI 0
/// </summary>
SdExFat sd;
ExFile LogFile;
ExFile root;
char line[250];
String str = "";
int position = 0;


////// Time libraries
#include <TimeLib.h>
#include <NTPClient.h>
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", 0, 1000); // For details, see https://github.com/arduino-libraries/NTPClient
// Time zone library
#include <Timezone.h>
//  Central Time Zone (Mexico City)
TimeChangeRule mxCDT = { "CDT", First, Sun, Apr, 2, -300 };
TimeChangeRule mxCST = { "CST", Last, Sun, Oct, 2, -360 };
Timezone mxCT(mxCDT, mxCST);


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


////// CO2 Sensor Input
const int CO2In = G35;


//////////////////////////////////////////
////// User Constants and Variables //////
//////////////////////////////////////////

////// Station IDs & Constants
const int StaNum = 4;
String StaType = F("Gas-Environmental Alarm");
String StaName = F("Photosynthesis Lab Alarm");
String Firmware = F("v1.0.0");
//const float VRef = 3.3;
const float CO2cal = 2.5;   // Calibrated coeficient to transform voltage to ppm
const bool debug = true;


////// Log File & Headers
const char* FileName[] = { "2020.txt", "2021.txt", "2022.txt", "2023.txt", "2024.txt", "2025.txt", "2026.txt", "2027.txt", "2028.txt", "2029.txt",
            "2030.txt", "2031.txt", "2032.txt", "2033.txt", "2034.txt", "2035.txt", "2036.txt", "2037.txt", "2038.txt", "2039.txt",
            "2040.txt", "2041.txt", "2042.txt", "2043.txt", "2044.txt", "2045.txt", "2046.txt", "2047.txt", "2048.txt", "2049.txt",
            "2050.txt" };
const String Headers = F("UNIX_t\tyear\tmonth\tday\thour\tminute\tsecond\t\
O2%\tCO2ppm\t\
AirTemp\tAirRH\t\
SentIoT");
const int HeaderN = 11;	// Number of items in header (columns); it is cero indexed
String LogString = "";


////// Time variables
unsigned long time_ms = 0;
time_t unix_t;	    // RT UNIX time stamp
time_t SD_unix_t;    // Recorded UNIX time stamp
int s = -1;		    //Seconds
int m = -1;		    //Minutes
int h = -1;		    //Hours
int dy = -1;	    //Day
int mo = -1;	    //Month
int yr = -1;	    //Year
int yrIoT = -1;


////// State machine Shift Registers
int LastSec = -1;           // Last second that gas sensor values where measured
int LastSum = -1;			// Last minute that variables were added to the sum for later averaging
int SumNum = 0;				// Number of times a variable value has beed added to the sum for later averaging
int LastLog = -1;			// Last minute that variables were loged to the SD card
bool PayloadRdy = false;	// Payload ready to send to LoRa


////// Alarm variables
bool O2low = false;
bool O2high = false;
bool CO2high = false;

////// Measured instantaneous variables
float O2Value = -1;		// Oxygene value read each second
float CO2raw = -1;    // Carbon dioxide value read each second
float CO2Volt = -1;    // Carbon dioxide value read each second
float CO2ppm = -1;
float Temp = -1;        // Air temperature read each minute
float RH = -1;          // Air RH value read each minute


////// Variables to store sum for eventual averaging
float O2ValueSum = 0;
float CO2ppmSum = 0;
float TempSum = 0;
float RHSum = 0;


////// Values to be logged. They will be the average over the last 5 minutes
float O2ValueAvg = 0;
float CO2ppmAvg = 0;
float TempAvg = 0;
float RHAvg = 0;


//////////////////////////////////////////////////////////////////////////
// the setup function runs once when you press reset or power the board //
//////////////////////////////////////////////////////////////////////////
void setup() {
    ////// Initialize and setup M5Stack
    M5.begin();
    M5.Power.begin();
    M5.Lcd.println(F("M5 started"));
    Serial.println(F("M5 started"));
    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);
    M5.Lcd.print(F("Connecting to internet"));

    while (WiFi.status() != WL_CONNECTED) {
        M5.Lcd.println(WiFi.status());
        delay(1000);
        M5.Lcd.print(".");
    }




    ////// Configure IoT
    thing.add_wifi(ssid, password);
    thing["RT_Oxygene"] >> [](pson& out) { out = O2Value; };
    thing["RT_Carbon_Dioxide"] >> [](pson& out) { out = CO2ppm; };
    thing["RT_Temp"] >> [](pson& out) { out = Temp; };
    thing["RT_RH"] >> [](pson& out) { out = RH; };
    thing["Alarm_O2low"] >> [](pson& out) { out = O2low; };
    thing["Alarm_O2high"] >> [](pson& out) { out = O2high; };
    thing["Alarm_CO2high"] >> [](pson& out) { out = CO2high; };

    thing["Avg_Data"] >> [](pson& out) {
        out["Time_Stamp"] = SD_unix_t;
        out["Oxygene"] = O2ValueAvg;
        out["Carbon_Dioxide"] = CO2ppmAvg;
        out["Temperature"] = TempAvg;
        out["Relative_Humidity"] = RHAvg;
    };



    ////// Initialize SD card
    Serial.print(F("Start SD:"));
    Serial.println(sd.begin(SD_CONFIG));
    // Reserve RAM memory for large and dynamic String object
    // used in SD file write/read
    // (prevents heap RAM framgentation)
    LogString.reserve(HeaderN * 7);
    str.reserve(HeaderN * 7);


    //////// Start NTP client engine
    M5.Lcd.println(F("Starting NTP client engine"));
    timeClient.begin();


    /////// Start oxygene sensor
    //M5.Lcd.print(F("Starting Oxygen sensor"));
    //Oxygen.begin(Oxygen_IICAddress);


    ////// Configure ADC for reading CO2 sensor
    analogSetClockDiv(128);     // Default is 1, higher value should result in lower sampling speed (better performance)
    analogReadResolution(12);   // ADC read resolution
    analogSetWidth(12);         // ADC sampling resolution
    analogSetAttenuation(ADC_11db);  // ADC attenuation, with 11 dB the range is 150 to 2450 mV
    // Adjust CO2 sensor reading if resolution or attenuation is changed


    // Start SHT31 Temp and RH sensor
    //while (sht3x.begin() != 0) {
        //M5.Lcd.println(F("Failed to Initialize the chip, please confirm the wire connection"));
        //delay(0);
    //}
    //M5.Lcd.print(F("Chip serial number"));
    //M5.Lcd.println(sht3x.readSerialNumber());
    //if (!sht3x.softReset()) {
        //M5.Lcd.print(F("Failed to Initialize the chip...."));
    //}
}



//////////////////////////////////////////////////////////////////////////
// The loop function runs over and over again until power down or reset //
//////////////////////////////////////////////////////////////////////////
void loop() {
    if (debug) {
        M5.Lcd.print(F("Loop start"));
        //Serial.println(F("Loop start serial"));
    }
    ////// State 0. Keep the Iot engine runing
    thing.handle();


    ////// State 1. Get current time
    if (WiFi.isConnected()) { // get UTC unix timestamp from internet time via NTC
        timeClient.update();    // It does not force-update NTC time (see NTPClient declaration for actual udpate interval)
        unix_t = timeClient.getEpochTime();
        unix_t = mxCT.toLocal(unix_t); // Conver to local time
        setTime(unix_t);   // set system time to given unix timestamp
        time_ms = millis();
        M5.Lcd.println(time_ms);
    }
    else { // If no internet connection, estimate unixtime from last update and enlapsed miliseconds
        if (debug) { M5.Lcd.println(F("No internet")); }
        if (millis() - time_ms > 1000) {
            unix_t = unix_t + round(((millis() - time_ms) / 1000));
            setTime(unix_t);   // set system time to given unix timestamp
            time_ms = millis();
        }
    }
    s = second(unix_t);
    m = minute(unix_t);
    h = hour(unix_t);
    dy = day(unix_t);
    mo = month(unix_t);
    yr = year(unix_t);
    if (debug) { M5.Lcd.println((String)"Time: " + h + ":" + m + ":" + s); }


    ////// State 2. Test if it is time to read gas sensor values (each second)
    /*
    if (s != LastSec) { // logical test still need to be added
        O2Value = Oxygen.ReadOxygenData(COLLECT_NUMBER); //DFRobot_OxygenSensor Oxygen code
        CO2raw = analogRead(CO2In);
        CO2Volt = (CO2raw * 2450) / 4096;   // With attenuation set to 11dB, 12 bit resolution maps to 2450 mV (see ADC setup)
        CO2ppm = (CO2Volt - 400) * CO2cal;
        if (debug) {
            M5.Lcd.println();
            M5.Lcd.println((String)"Oxygene: " + O2Value + " %vol");
            M5.Lcd.println((String)"CO2: " + CO2ppm + " ppm");
            M5.Lcd.println();
        }
        LastSec = s;
    }
    */
    M5.Lcd.print(F("Update Variables"));
    O2Value = (21.5 * random(80, 120)) / 100;
    CO2ppm = (425 * random(80, 120)) / 100;
    Temp = (25.75 * random(80, 120)) / 100;
    RH = (70.26 * random(80, 120)) / 100;

    ////// State 3. Test gas limits
    // Low oxygene
    if (O2Value < 19) { O2low = true; }
    else { O2low = false; }
    // High oxygene
    if (O2Value > 23) { O2high = true; }
    else { O2low = false; }
    // High carbon dioxide
    if (CO2ppm > 2000) { CO2high = true; }
    else { CO2high = false; }


    ////// State 4 trigger alarm



    ////// State 5. Test if it is time to read Temp and RH values
    ////// AND record sensor values for 5-minute averages (each minute)
    ////// AND update screen
    /*
    if (m != LastSum) {
        Temp = sht3x.getTemperatureC();
        RH = sht3x.getHumidityRH();
        if (debug) {
            M5.Lcd.println();
            M5.Lcd.println((String)"Temp: " + Temp + " °C");
            M5.Lcd.println((String)"RH: " + RH + " %");
            M5.Lcd.println();
        }
        // Add new values to sum
        O2ValueSum += O2Value;
        CO2ppmSum += CO2ppm;
        TempSum += Temp;
        RHSum += RH;

        // Update Shift registers
        LastSum = m;
        SumNum += 1;
    }
    */

    ////// State 6. Test if it is time to compute  averages and record in SD card (each 5 minutes)

    if (((m % 5) == 0) && (m != LastLog)) {
        // Calculate averages
        /*
        O2ValueAvg = O2ValueSum / SumNum;
        CO2ppmAvg = CO2ppmSum / SumNum;
        TempAvg = TempSum / SumNum;
        RHAvg = RHSum / SumNum;
        */

        // Open Year LogFile (create if not available)
        Serial.print(F("Year: "));
        Serial.println(yr);
        Serial.println(!sd.exists(FileName[yr - 2020]));
        if (!sd.exists(FileName[yr - 2020])) {
            Serial.println(F("If true"));
            LogFile.open((FileName[yr - 2020]), O_RDWR | O_CREAT); // Create file

            // Add Metadata
            LogFile.println("Start position of last line send to IoT:\t1");
            // Tabs added to prevent line ending with 0. Line ending with 0 indicates that line needs to be sent to IoT.
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("Metadata:"));
            LogFile.println((String)"Station Number\t" + StaNum + "\t\t\t");
            LogFile.println((String)"Station Name\t" + StaName + "\t\t\t");
            LogFile.println((String)"Station Type\t" + StaType + "\t\t\t");
            LogFile.println((String)"Firmware\t" + Firmware + "\t\t\t");
            LogFile.println((String)"CO2 sensor calibration\t" + CO2cal + "\t\t\t");
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));

            LogFile.println(Headers); // Add Headers
        }
        else {
            Serial.println(F("Else true"));
            Serial.print(F("open success: "));
            Serial.println(LogFile.open(FileName[yr - 2020], O_RDWR)); // Open file
            LogFile.seekEnd(); // Set position to end of file
        }


        // Log to SD card
        LogString = (String)unix_t + "\t" + yr + "\t" + mo + "\t" + dy + "\t" + h + "\t" + m + "\t" + s + "\t" +
            String(O2Value, 4) + "\t" + String(CO2ppm, 4) + "\t" +
            String(Temp, 4) + "\t" + String(RH, 4) + "\t" +
            "0";
        LogFile.println(LogString); // Prints Log string to SD card file "LogFile.txt"
        LogFile.close(); // Close SD card file to save changes


        // Reset Shift Registers
        LastLog = m;

        O2ValueSum = 0;
        CO2ppmSum = 0;
        TempSum = 0;
        RHSum = 0;

        SumNum = 0;
    }


    ////// State 7. Test if there is data available to be sent to IoT cloud

    if (PayloadRdy == false) {
        Serial.println(F("Start scanning files"));
        Serial.println(root.open("/"));	// Open root directory
        Serial.print(F("Current pos: "));
        Serial.println(root.curPosition());
        Serial.print(F("Rewind root: "));
        //Serial.println(root.seekSet(0));
        root.rewind();	// Rewind root directory
        Serial.print(F("open next: "));

        Serial.println(LogFile.openNext(&root, O_RDWR));    // NEEDED IN M5; I DO NOT KNOW WHY....

        Serial.print(F("Current pos: "));
        Serial.println(root.curPosition());
        Serial.print(F("Available bytes in root: "));
        Serial.println(root.available());

        Serial.println(F("While start"));
        while (LogFile.openNext(&root, O_RDWR)) {
            LogFile.printName(&Serial);
            LogFile.rewind();
            LogFile.fgets(line, sizeof(line));     // Get first line
            str = String(line);
            Serial.print(F("raw line: "));
            Serial.println(str);
            str = str.substring(str.indexOf("\t"), str.indexOf("\r"));
            if (str == "Done") {	// Skips file if year data is all sent to IoT
                LogFile.close();
                continue;
            }
            position = str.toInt();	// Sets file position to start of last line sent to IoT
            LogFile.seekSet(position);	// Set position to last line sent to LoRa
            // Read each line until a line not sent to IoT is found
            while (LogFile.available()) {
                position = LogFile.curPosition();  // START position of current line
                int len = LogFile.fgets(line, sizeof(line));
                if (line[len - 2] == '0') {
                    Serial.println("Loteria!!!");
                    str = String(line); // str is the payload, next state test if there is internet connection to send payload to IoT
                    Serial.println(str);
                    PayloadRdy = true;
                    break;
                }
            }
        }
        Serial.print(F("close root: "));
        Serial.println(root.close());
        Serial.print(F("close LogFile: "));
        Serial.println(LogFile.close());
        Serial.println(F("State 7 end"));
    }


    ////// State 8. Test if there is Internet and a Payload to sent SD data to IoT
    Serial.print(F("State 8 exec test: "));
    Serial.println(WiFi.isConnected() &&
        thing.is_connected() &&
        PayloadRdy &&
        unix_t - t_DataBucket > DataBucket_frq);
    if (WiFi.isConnected() &&
        thing.is_connected() &&
        PayloadRdy &&
        unix_t - t_DataBucket > DataBucket_frq) {
        t_DataBucket = unix_t; // Record Data Bucket update TRY; even if it is not succesfful
        // extract data from payload string (str)
        for (int i = 0; i < HeaderN; i++) {
            String buffer = str.substring(0, str.indexOf('\t'));
            if (i < 2 || i > 6) { 	// Do not send to IoT human-readable date/time (columns 1 to 6); get data year from column 2
                if (i == 0) {   // UNIX Time
                    SD_unix_t = buffer.toInt();
                }
                else if (i == 1) {
                    yrIoT = buffer.toInt();
                }
                else if (i == 7) {  // O2
                    O2ValueAvg = buffer.toFloat();
                }
                else if (i == 8) {  // CO2
                    CO2ppmAvg = buffer.toFloat();
                }
                else if (i == 9) {  // Temp
                    TempAvg = buffer.toFloat();
                }
                else if (i == 10) { // RH
                    RHAvg = buffer.toFloat();
                }


            }
            str = str.substring(str.indexOf('\t') + 1);
        }
        // send data to IoT. If succsessful, rewrite line in log File
        // thing.write_bucket("Data_Gas_Alarm_PhotoLab", "Avg_Data", confirm_write = true)
        // thing.stream("Avg_Data")
        if (thing.write_bucket("Data_Gas_Alarm_PhotoLab", "Avg_Data", true)) {
            LogFile.open(FileName[yrIoT - 2020], O_RDWR); // Open file containing the data just sent to IoT
            str = String(line);                     // Recover complete payload from original line
            str.setCharAt(str.length() - 2, '1');   // Replace 0 with 1, last characters are always "\r\n"
            LogFile.seekSet(position);              // Set position to start of line to be rewritten
            LogFile.println(str.substring(0, str.length() - 1));    // Remove last character ('\n') to prevent an empty line below rewritten line
            LogFile.close();
            PayloadRdy = false;
        }
    }


}
